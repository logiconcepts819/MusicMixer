#include "Crossfader.h"
#include "fademaps/FadeMap.h"
#include "../AudioFile.h"
#include "../AudioBlock.h"
#include "../AudioSink.h"
#include "../RequestQueue.h"
#include "../stretch/AudioStretchInfo.h"
#include "../stretch/AudioStretcher.h"
#include "CrossfadeCalculator.h"
#include "DJCrossfadeCalculatorOld.h"
#include <cassert>
#include <string>
#include <iostream>

const size_t Crossfader::m_DefaultXfadeBufferSize = 512;

void Crossfader::SubmitStretchInfoAndReset(bool fadeOut, std::shared_ptr<AudioStretchInfo> & stretchInfo)
{
	(fadeOut ? m_Stretcher1 : m_Stretcher2)->SubmitAudioStretchInfo(stretchInfo);
	stretchInfo = std::make_shared<AudioStretchInfo>();
}

void Crossfader::setCrossfadeTime(double crossfadeTime)
{
	m_CrossfadeTime = crossfadeTime;
	if (m_Initialized)
	{
		xfadeCalc->setCrossfadeTime(crossfadeTime);
	}
}

void Crossfader::setUsingOptimisticTempoAdaptation(bool enable)
{
	m_UseOptimisticTempoAdaptation = enable;
	if (m_Initialized && xfadeCalc->IsDJCalculator())
	{
		xfadeCalc->AsDJCalculator()->setUsingOptimisticTempoAdaptation(
				enable);
	}
}

std::shared_ptr<AudioBlock> Crossfader::ChunkAndSendToStretcher(std::unique_ptr<AudioStretcher> & stretcher, AudioBlock * block)
{
	// VERY CHALLENGING CRAFTSMANSHIP HERE!!!

	bool done = false;
	bool fadeOut = stretcher == m_Stretcher1;
	AudioFile * & file = fadeOut ? m_File1 : m_File2;
	std::shared_ptr<AudioStretchInfo> stretchInfo =
			std::make_shared<AudioStretchInfo>();
	AudioBlock * theBlock = block;
	std::shared_ptr<AudioBlock> nextBlock;
	bool haveRefPos = false;
	bool haveCompRefPos = false;

	// The reference position is used to indicate where the track should start
	// playing.  The complementary reference position is used to indicate where
	// the track should start playing according to the reference position in
	// the other track (i.e., it is used for synchronization purposes).
	double refPercent = 0.0;
	double compRefPercent;
	double sr = (double) AudioSink::Instance().getSampleRate();
	double dt = m_XfadeBufferSize / sr;
	double relTime = 0.0;
	double fadePercent = 0.0;

	// Step 1:  synchronize the two tracks
	if (fadeOut)
	{
		double fadeOutPercent = xfadeCalc->GetFadeOutPercentage();
		double fadeInPercent = xfadeCalc->GetFadeInPercentage();
		double origRefPercent = 0.0;
		if (fadeInPercent < 1.0 - CrossfadeCalculator::GetEpsilon())
		{
			{
				std::unique_lock<std::mutex> lck(m_ExtraEndInfoMutex);
				m_ExtraEndInfoCond.wait(lck,
						[this]{ return m_ExtraEndInfoAvailable; });
				refPercent = origRefPercent = m_PercentPadding;
			}
			refPercent *= (1.0 - fadeOutPercent) / (1.0 - fadeInPercent);
		}
		refPercent += fadeOutPercent;
		compRefPercent = fadeInPercent + origRefPercent;

		bool percentDone = false;
		double timePadding = relTime;
		bool calculationDone = false;
		while (!percentDone && !calculationDone)
		{
			double dp = xfadeCalc->ComputePercentageChangeForTrack(fadeOut,
					fadePercent, relTime, dt, relTime, percentDone);
			double theDt = dt;
			double theDp = dp;
			if (fadePercent + dp >= refPercent)
			{
				calculationDone = true;
				theDp = refPercent - fadePercent;
				theDt = xfadeCalc->ComputeOriginalTimeChangeForTrack(
						fadeOut, fadePercent, theDp);
			}
			timePadding -= theDt;
			fadePercent += theDp;
		}
		done = done || percentDone;
	}
	else
	{
		double desiredSeek = xfadeCalc->GetTimeAtStartOfFadeIn();
		assert(file->seek(desiredSeek));
		nextBlock = file->getNextAudioBlock();
		done = file->isFileDone();
		theBlock = nextBlock->getNumSamples() == 0 ? nullptr
				: nextBlock.get();
		double filePos = file->getPosition();

		// At least in libav, it seems to be almost always the
		// case that filePos >= desiredSeek.
		bool percentDone = false;
		double timePadding = filePos > desiredSeek
				? filePos - desiredSeek : 0.0;
		while (!percentDone && timePadding > 0.0)
		{
			double theDt = std::min(timePadding, dt);
			double dp = xfadeCalc->ComputePercentageChangeForTrack(
					fadeOut, fadePercent, relTime, theDt,
					relTime, percentDone);
			timePadding -= dt;
			fadePercent += dp;
		}
		done = done || percentDone;

		{
			std::lock_guard<std::mutex> lck(m_ExtraEndInfoMutex);
			m_ExtraEndInfoAvailable = true;
			m_PercentPadding = fadePercent;
			m_ExtraEndInfoCond.notify_all();
		}

		refPercent = xfadeCalc->GetFadeInPercentage() + fadePercent;
		compRefPercent = xfadeCalc->GetFadeOutPercentage();
	}

	bool setCompRes = compRefPercent > refPercent;
	if (!fadeOut)
	{
		std::lock_guard<std::mutex> lck(m_File2HasCompRefPosLock);
		m_File2HasCompRefPosAvailable = true;
		m_File2HasCompRefPos = setCompRes;
		m_File2HasCompRefPosCond.notify_all();
	}

	// Step 2:  Shove audio data into the stretcher
	size_t offset = 0;
	size_t blocksPlacedInBuffer = 0;
	double volPercent = 0.0;
	bool haveLastBlock = false;
	bool transferredAtLeastOneBlock = false;
	while (!done)
	{
		if (theBlock == nullptr)
		{
			nextBlock = file->getNextAudioBlock();
			done = file->isFileDone();
			theBlock = nextBlock->getNumSamples() == 0 ? nullptr
					: nextBlock.get();
		}

		if (theBlock != nullptr)
		{
			if (blocksPlacedInBuffer == 0)
			{
				// Set up new stretch information

				double dp = xfadeCalc->ComputePercentageChangeForTrack(fadeOut,
						fadePercent, relTime, dt, relTime, haveLastBlock);
				if (!haveRefPos && fadePercent + dp >= refPercent)
				{
					double refDp = refPercent - fadePercent;
					haveRefPos = true;
					stretchInfo->SetUsingRefPos(true);
					double refDt =
							xfadeCalc->ComputeOriginalTimeChangeForTrack(
							fadeOut, fadePercent, refDp);
					size_t refPos = (size_t)
							(refDt * AudioSink::Instance().getSampleRate());
					stretchInfo->SetReferencePos(refPos);
				}
				if (!haveCompRefPos && setCompRes && fadePercent + dp >= compRefPercent)
				{
					double refDp = compRefPercent - fadePercent;
					haveCompRefPos = true;
					stretchInfo->SetUsingComplementaryRefPos(true);
					double refDt =
							xfadeCalc->ComputeOriginalTimeChangeForTrack(
							fadeOut, fadePercent, refDp);
					size_t refPos = (size_t)
							(refDt * AudioSink::Instance().getSampleRate());
					stretchInfo->SetComplementaryReferencePos(refPos);
				}

				double timeRatio = xfadeCalc->ComputeStretchFactorForTrack(
						fadeOut, fadePercent);
				stretchInfo->SetTimeRatio(timeRatio);
				volPercent = xfadeCalc->ComputeVolumeForTrack(fadeOut,
						fadePercent);
				volPercent = m_FadeMap->MapCrossfadeVolume(volPercent);
				fadePercent += dp;
			}

			size_t count = theBlock->getNumSamples() - offset;
			size_t bufCount = m_XfadeBufferSize - blocksPlacedInBuffer;
			if (count >= bufCount)
			{
				count = bufCount;
				blocksPlacedInBuffer = 0;
				stretchInfo->AppendSamples(*theBlock, volPercent, offset,
						offset + count);
				done = haveLastBlock;
				stretchInfo->SetLastOne(done);
				SubmitStretchInfoAndReset(fadeOut, stretchInfo);
				transferredAtLeastOneBlock = true;
			}
			else
			{
				stretchInfo->AppendSamples(*theBlock, volPercent, offset,
						offset + count);
				blocksPlacedInBuffer += count;
			}

			offset += count;
			if (offset >= theBlock->getNumSamples())
			{
				offset = 0;
				theBlock = nullptr;
			}
		}
	}

	// Step 3:  Handle leftovers from step 2
	if (blocksPlacedInBuffer != 0 || !transferredAtLeastOneBlock)
	{
		// If we reach this branch, we hit EOF before the end of the crossfade
		// interval.

		stretchInfo->SetLastOne(true);
		SubmitStretchInfoAndReset(fadeOut, stretchInfo);
	}

	return theBlock == nullptr ? nullptr : theBlock->split(offset);
}

/*
static void PrintBuffer(const std::string & buffDesc, const std::shared_ptr<AudioBlock> & block)
{
	std::cout << "Resulting buffer " << buffDesc << std::endl;
	if (block != nullptr)
	{
		for (size_t k = 0; k != block->getNumSamples(); ++k)
		{
			std::cout << '\t';
			for (int m = 0; m != AudioSink::Instance().getNumChannels(); ++m)
			{
				std::cout << block->getSampleAtPosition(m, k) << ' ';
			}
			std::cout << '\n';
		}
	}
	else
	{
		std::cout << "\t<null>" << std::endl;
	}
}
*/

void Crossfader::PlaybackCrossfadeMix()
{
	// Crossfade requires processing the two buffers in lockstep.
	// We do it using the following steps:
	// 1. Throw away all blocks before the reference position.
	// 2. Sync up complementary reference position with the reference
	//    position of the other track.
	// 3. Proceed until one or both of the tracks run out.
	// 4. All operation resumes as usual for the next track.

	std::shared_ptr<AudioBlock> blk1, blk2;

	bool haveRefPos = false;
	bool haveCompRefPos;
	bool eos = false;
	bool clickRemove = true;

	{
		std::unique_lock<std::mutex> lck(m_File2HasCompRefPosLock);
		m_File2HasCompRefPosCond.wait(lck,
				[this] { return m_File2HasCompRefPosAvailable; });
		haveCompRefPos = !m_File2HasCompRefPos;
	}

	size_t compRefPos;

	// Skip over frames until we hit a reference point
	while (!eos && !haveRefPos)
	{
		size_t refPos;
		blk1 = m_Stretcher1->getStretchedAudioAsBlock(
				m_XfadeBufferSize, refPos, compRefPos, eos);
		std::shared_ptr<AudioBlock> nextBlock;
		if (refPos != std::string::npos)
		{
			haveRefPos = true;
			nextBlock = blk1->split(refPos);
			if (compRefPos != std::string::npos)
			{
				haveCompRefPos = true;
				blk1 = nextBlock->split(compRefPos - refPos);
				if (nextBlock->getNumSamples() != 0) // Don't be wasteful
				{
					nextBlock->setRemoveClick(clickRemove);
					clickRemove = false;
					AudioSink::Instance().SubmitAudioBlock(nextBlock);
				}
			}
			else if (!haveCompRefPos && blk1->getNumSamples() != 0)
			{
				blk1->setRemoveClick(clickRemove);
				clickRemove = false;
				AudioSink::Instance().SubmitAudioBlock(blk1);
			}
		}
	}

	// Play frames from single track until we hit a complementary reference
	// point
	while (!eos && !haveCompRefPos)
	{
		size_t refPos;
		blk1 = m_Stretcher1->getStretchedAudioAsBlock(
				m_XfadeBufferSize, refPos, compRefPos, eos);
		std::shared_ptr<AudioBlock> nextBlock;
		if (compRefPos != std::string::npos)
		{
			haveCompRefPos = true;
			nextBlock = blk1->split(compRefPos);
		}
		if (blk1->getNumSamples() != 0) // Don't be wasteful
		{
			blk1->setRemoveClick(clickRemove);
			clickRemove = false;
			AudioSink::Instance().SubmitAudioBlock(blk1);
		}
		blk1 = nextBlock;
	}
	// blk1 now contains the audio data that is needed for proper
	// synchronization

	// Sync buffers so that the beats align properly when the frame
	// times overlap
	if (!eos)
	{
		// Skip over frames until we hit a reference point
		haveRefPos = false;
		if (blk1 == nullptr)
		{
			// Prevent a segfault if blk1 has a nullptr
			blk1 = std::make_shared<AudioBlock>();
			blk1->initializeChannels();
		}
		bool embeddedOverlap = false;
		while (!eos && !haveRefPos)
		{
			size_t refPos;
			blk2 = m_Stretcher2->getStretchedAudioAsBlock(
					m_XfadeBufferSize, refPos, compRefPos, eos);
			std::shared_ptr<AudioBlock> nextBlock;
			if (refPos != std::string::npos)
			{
				haveRefPos = true;
				std::shared_ptr<AudioBlock> overlap = blk2->overlap(
						blk1, refPos, embeddedOverlap);

				// End result: blk2 is discarded, overlap is used for
				//             playback, and blk1 is used for
				//             synchronization
				if (overlap->getNumSamples() != 0) // Don't be wasteful
				{
					overlap->setRemoveClick(clickRemove);
					clickRemove = false;
					AudioSink::Instance().SubmitAudioBlock(overlap);
				}
			}
		}

		// Now it's time for some real buffer synchronization.
		if (!eos)
		{
			size_t refPos;
			if (embeddedOverlap)
			{
				// overlap is completely embedded within the frame belonging
				// to the next track
				blk1 = m_Stretcher1->getStretchedAudioAsBlock(
						blk2->getNumSamples(), refPos, compRefPos, eos);
				blk1->merge(blk2); // Only need a simple merge here
				if (blk1->getNumSamples() != 0)
				{
					AudioSink::Instance().SubmitAudioBlock(blk1);
				}
			}
			else
			{
				// overlap touches or exceeds the right edge of the frame
				// belonging to the next track
				blk2 = m_Stretcher2->getStretchedAudioAsBlock(
						blk1->getNumSamples(), refPos, compRefPos, eos);
				blk2->merge(blk1); // Only need a simple merge here
				if (blk2->getNumSamples() != 0)
				{
					AudioSink::Instance().SubmitAudioBlock(blk2);
				}
			}

			// At this stage, it's go time!
			// We just keep blending until one of the streams hits
			// the EOS.  Then, we just insert the samples individually
			// if the non-EOS stream corresponds to the next track.  This
			// ensures that we have no discontinuities between the end of
			// the crossfade and the start of the next track.  Once we're
			// all done, we are in the next track.
			bool eos2 = false;
			while (!eos && !eos2)
			{
				blk1 = m_Stretcher1->getStretchedAudioAsBlock(
						m_XfadeBufferSize, refPos, compRefPos, eos);
				blk2 = m_Stretcher2->getStretchedAudioAsBlock(
						m_XfadeBufferSize, refPos, compRefPos, eos2);
				blk1->merge(blk2);
				if (blk1->getNumSamples() != 0)
				{
					blk1->setRemoveClick(clickRemove);
					clickRemove = false;
					AudioSink::Instance().SubmitAudioBlock(blk1);
				}
			}
			while (!eos2)
			{
				blk2 = m_Stretcher2->getStretchedAudioAsBlock(
						m_XfadeBufferSize, refPos, compRefPos, eos2);
				if (blk1->getNumSamples() != 0)
				{
					blk1->setRemoveClick(clickRemove);
					clickRemove = false;
					AudioSink::Instance().SubmitAudioBlock(blk2);
				}
			}
		}
	}
}

void Crossfader::PlaybackThread(Crossfader * xfader)
{
	xfader->PlaybackCrossfadeMix();
}

void Crossfader::LaunchCrossfade1Thread(Crossfader * xfader, std::shared_ptr<AudioBlock> startingFadeOutBlock)
{
	xfader->ChunkAndSendToStretcher(xfader->m_Stretcher1, startingFadeOutBlock.get());
}

void Crossfader::LaunchCrossfade2Thread(Crossfader * xfader)
{
	xfader->m_XfadeLeftover = xfader->ChunkAndSendToStretcher(xfader->m_Stretcher2);
}

FadeMap & Crossfader::GetFadeMap()
{
	return *m_FadeMap;
}

const FadeMap & Crossfader::GetFadeMap() const
{
	return *m_FadeMap;
}

void Crossfader::SetFadeMap(FadeMap & fadeMap)
{
	m_FadeMap = &fadeMap;
}

Crossfader::Crossfader(AudioFile & file1, AudioFile & file2, FadeMap & fadeMap)
: m_StretchBufReadPos(0), m_File1(&file1), m_File2(&file2),
  m_FadeMap(&fadeMap), m_Initialized(false), m_Ineligible(false),
  m_AllowDJCrossfade(false), m_AllowCrossfade(false),
  m_CrossfadeTime(RequestQueue::GetDefaultXfadeDuration()),
  m_UseOptimisticTempoAdaptation(false), m_SampleCounter(0),
  m_XfadeBufferSize(m_DefaultXfadeBufferSize), m_File2HasCompRefPos(false),
  m_File2HasCompRefPosAvailable(false), m_PercentPadding(0.0),
  m_ExtraEndInfoAvailable(false)
{
}

Crossfader::~Crossfader()
{
}

void Crossfader::InitializeCrossfade()
{
	if (!m_Initialized && !m_Ineligible)
	{
		xfadeCalc.reset(new DJCrossfadeCalculatorOld(*m_File1, *m_File2));
		if (m_AllowDJCrossfade && xfadeCalc->CheckCrossfadeCondition())
		{
			xfadeCalc->AsDJCalculator()->setUsingOptimisticTempoAdaptation(
					m_UseOptimisticTempoAdaptation);
			m_Initialized = true;
		}
		else
		{
			xfadeCalc.reset(new CrossfadeCalculator(*m_File1, *m_File2));
			if (m_AllowCrossfade && xfadeCalc->CheckCrossfadeCondition())
			{
				xfadeCalc->setCrossfadeTime(m_CrossfadeTime);
				m_Initialized = true;
			}
			else
			{
				xfadeCalc.reset();
				m_Ineligible = true;
			}
		}
		if (m_Initialized)
		{
			m_Stretcher1.reset(new AudioStretcher);
			m_Stretcher2.reset(new AudioStretcher);
		}
	}
}

bool Crossfader::ReadyToCrossfade(double & backDelta) const
{
	bool rv = false;
	if (m_Initialized)
	{
		backDelta = m_File1->getPosition() - xfadeCalc->GetTimeAtStartOfFadeOut();
		rv = backDelta >= 0.0;
	}
	return rv;
}

void Crossfader::StartCrossfade(std::shared_ptr<AudioBlock> & startingFadeOutBlock)
{
	if (m_Initialized)
	{
		m_File2HasCompRefPosAvailable = false;
		m_XfadeThread1.reset(new std::thread(LaunchCrossfade1Thread, this, startingFadeOutBlock));
		m_XfadeThread2.reset(new std::thread(LaunchCrossfade2Thread, this));
		m_PlaybackThread.reset(new std::thread(PlaybackThread, this));
	}
}

void Crossfader::WaitOnThreadsAndGiveXfadeLeftover(std::shared_ptr<AudioBlock> & leftover)
{
	if (m_XfadeThread1 != nullptr)
	{
		m_XfadeThread1->join();
		m_XfadeThread1.reset();
	}
	if (m_XfadeThread2 != nullptr)
	{
		m_XfadeThread2->join();
		m_XfadeThread2.reset();
	}
	if (m_PlaybackThread != nullptr)
	{
		m_PlaybackThread->join();
		m_PlaybackThread.reset();
	}
	leftover = m_XfadeLeftover;
	m_XfadeLeftover.reset();
}
