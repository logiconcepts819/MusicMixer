#include "AudioStretcher.h"
#include "AudioStretchInfo.h"
#include "../AudioBlock.h"
#include "../AudioSink.h"
#include <algorithm>
#include <string>
#include <cstring>

// Implementation based off of Mixxx's enginebufferscalerubberband

const size_t AudioStretcher::m_RubberbandBlockSize = 256;

AudioStretcher::AudioStretcher()
: m_Rotate(true), m_BufPos(0), m_BufLen(0), m_NumSIFrames(0),
  m_RefPos(std::string::npos), m_RelRefPos(std::string::npos),
  m_RefPosStretch(1.0), m_CompRefPos(std::string::npos),
  m_RelCompRefPos(std::string::npos), m_EOS(false),
  m_Latency(0), m_ConsumedLatency(0), m_PastInitialLatency(false),
  m_FinalLatency(0)
{
	m_RBBuf.resize(AudioBufUtil::MaxAudioBufLen);
	m_RBProcBuf = AudioBufUtil::NewAudioBuffer(AudioBufUtil::MaxAudioBufLen);
	InitRubberband();
}

AudioStretcher::~AudioStretcher()
{
	AudioBufUtil::FreeAudioBuffer(m_RBProcBuf);
}

void AudioStretcher::InitRubberband()
{
	m_RBS.reset(new RubberBand::RubberBandStretcher(
			AudioSink::Instance().getSampleRate(),
			AudioSink::Instance().getNumChannels(),
			RubberBand::RubberBandStretcher::OptionProcessRealTime));
	m_RBS->setMaxProcessSize(m_RubberbandBlockSize);
	// Minimize memory reallocations during audio playback by setting
	// the time ratio to a modestly large value and then going back to
	// unity
	m_RBS->setTimeRatio(2.0);
	m_RBS->setTimeRatio(1.0);
}

void AudioStretcher::SubmitAudioStretchInfo(const std::shared_ptr<AudioStretchInfo> & stretchInfo)
{
	std::lock_guard<std::mutex> lck(m_AudioStretchInfoQueueMutex);
	m_AudioStretchInfoQueue.push(stretchInfo);
	m_AudioStretchInfoQueueCond.notify_all();
}

std::shared_ptr<AudioStretchInfo> AudioStretcher::ObtainAudioStretchInfo()
{
	std::unique_lock<std::mutex> lck(m_AudioStretchInfoQueueMutex);
	m_AudioStretchInfoQueueCond.wait(lck,
			[this]{ return !m_AudioStretchInfoQueue.empty(); });
	std::shared_ptr<AudioStretchInfo> item = m_AudioStretchInfoQueue.front();
	m_AudioStretchInfoQueue.pop();
	return item;
}

size_t AudioStretcher::GetFromRubberBand(float * buffer, size_t frames)
{
	size_t framesAvailable = (size_t) m_RBS->available();
	size_t framesToRead = std::min(framesAvailable, frames);
	size_t framesReceived = m_RBS->retrieve(
			(float * const *) m_RBProcBuf.data(), framesToRead);
	size_t numChannels = AudioSink::Instance().getNumChannels();
	for (size_t ch = 0; ch < numChannels; ++ch)
	{
		for (size_t k = 0; k < framesReceived; ++k)
		{
			buffer[ch + k * numChannels] = m_RBProcBuf.at(ch)[k];
		}
	}
	return framesReceived;
}

void AudioStretcher::PutIntoRubberBand(const float * buffer, size_t frames, bool flush)
{
	size_t numChannels = AudioSink::Instance().getNumChannels();
	for (size_t ch = 0; ch < numChannels; ++ch)
	{
		for (size_t k = 0; k < frames; ++k)
		{
			m_RBProcBuf.at(ch)[k] = buffer[ch + k * numChannels];
		}
	}
	m_RBS->process((const float * const *) m_RBProcBuf.data(), frames, flush);
}

size_t AudioStretcher::ComputeRefPos(size_t stretchedSize, size_t & coarseRefPos, size_t & relRefPos)
{
	size_t refPos = std::string::npos;
	if (coarseRefPos != std::string::npos)
	{
		size_t theRefPos = coarseRefPos + m_RefPosStretch * relRefPos;
		if (theRefPos >= stretchedSize)
		{
			refPos = std::string::npos;
			coarseRefPos = 0;
			relRefPos = (size_t)
					((theRefPos - stretchedSize) / m_RefPosStretch);
		}
		else
		{
			refPos = theRefPos;
			coarseRefPos = std::string::npos;
		}
	}
	return refPos;
}

const std::vector<float> & AudioStretcher::getStretchedAudio(size_t requestedStretchedSize, size_t & actualStretchedSize, size_t & refPos, size_t & compRefPos, bool & eos)
{
	float * readBuf = m_RBBuf.data();
	size_t numChannels = AudioSink::Instance().getNumChannels();
	size_t remainingCount = requestedStretchedSize / numChannels;
	actualStretchedSize = 0;
	eos = false;
	refPos = std::string::npos;
	while (remainingCount > 0)
	{
		if (m_EOS)
		{
			remainingCount = std::min(remainingCount, m_FinalLatency);
		}
		size_t framesReturned = GetFromRubberBand(readBuf, remainingCount);
		if (m_Latency != 0 && !m_PastInitialLatency)
		{
			m_ConsumedLatency += framesReturned;
			if (m_ConsumedLatency >= m_Latency)
			{
				m_PastInitialLatency = true;
				size_t leftover = std::min(framesReturned,
						m_ConsumedLatency - m_Latency);
				framesReturned -= leftover;
				memmove(readBuf, readBuf + leftover * numChannels,
						framesReturned * numChannels * sizeof(float));
			}
			else
			{
				framesReturned = 0;
			}
		}
		if (m_Latency == 0 || m_PastInitialLatency)
		{
			remainingCount -= framesReturned;
			actualStretchedSize += framesReturned;
			readBuf += framesReturned * numChannels;
		}

		if (m_EOS)
		{
			if (framesReturned == 0 || (m_FinalLatency -= framesReturned) == 0)
			{
				eos = true;
				m_RBS->reset();
				remainingCount = 0;
			}
		}
		else
		{
			size_t numFramesRequired = m_RBS->getSamplesRequired();
			if (numFramesRequired == 0)
			{
				int available = m_RBS->available();
				if (available == 0)
				{
					numFramesRequired = m_RubberbandBlockSize;
				}
			}

			if (remainingCount > 0 && numFramesRequired > 0)
			{
				if (m_Rotate)
				{
					m_StretchInfo = ObtainAudioStretchInfo();
					m_RefPosStretch = m_StretchInfo->GetTimeRatio();
					m_NumSIFrames =
							m_StretchInfo->GetBuffer().size() / numChannels;
					m_RBS->setTimeRatio(m_StretchInfo->GetTimeRatio());
					if (m_Latency == 0 || m_PastInitialLatency)
					{
						m_Latency = m_RBS->getLatency();
						if (m_PastInitialLatency)
						{
							m_FinalLatency = m_Latency;
						}
					}
					else
					{
						m_Latency = m_ConsumedLatency +
								((m_ConsumedLatency - m_Latency)
								* m_RBS->getLatency()) / m_Latency;
					}
					m_BufPos = 0;
					m_BufLen = std::min(m_NumSIFrames, numFramesRequired);
					if (m_RefPos == std::string::npos &&
							m_StretchInfo->IsUsingRefPos())
					{
						m_RefPos = actualStretchedSize;
						m_RelRefPos = m_StretchInfo->GetReferencePos();
					}
					if (m_CompRefPos == std::string::npos &&
							m_StretchInfo->IsUsingComplementaryRefPos())
					{
						m_CompRefPos = actualStretchedSize;
						m_RelCompRefPos =
								m_StretchInfo->GetComplementaryReferencePos();
					}
				}
				else
				{
					m_BufPos += m_BufLen;
					m_BufLen = m_BufPos > m_NumSIFrames ? 0 :
						std::min(m_NumSIFrames - m_BufPos, numFramesRequired);
				}
				m_Rotate = m_BufPos + m_BufLen >= m_NumSIFrames;
				m_EOS = m_Rotate && m_StretchInfo->IsLastOne();
				PutIntoRubberBand(
					m_StretchInfo->GetBuffer().data() + m_BufPos * numChannels,
					m_BufLen, m_EOS);
			}
		}
	}

	if (!m_EOS)
	{
		refPos = ComputeRefPos(actualStretchedSize, m_RefPos, m_RelRefPos);
		compRefPos = ComputeRefPos(actualStretchedSize, m_CompRefPos,
				m_RelCompRefPos);
	}

	return m_RBBuf;
}

std::shared_ptr<AudioBlock> AudioStretcher::getStretchedAudioAsBlock(size_t requestedStretchedSize, size_t & refPos, size_t & compRefPos, bool & eos)
{
	size_t actualStretchedSize = 0;
	const std::vector<float> & stretchedBlock = getStretchedAudio(
			requestedStretchedSize, actualStretchedSize, refPos, compRefPos,
			eos);
	std::shared_ptr<AudioBlock> blk = std::make_shared<AudioBlock>();
	std::vector<float> channelBuffer;
	channelBuffer.resize(actualStretchedSize);
	size_t numChannels = AudioSink::Instance().getNumChannels();
	for (size_t ch = 0; ch < numChannels; ++ch)
	{
		size_t idxMaj = 0;
		for (size_t k = 0; k < actualStretchedSize; ++k)
		{
			channelBuffer.at(k) = stretchedBlock.at(ch + idxMaj);
			idxMaj += numChannels;
		}
		blk->pushChannelData(channelBuffer.data(), channelBuffer.size());
	}
	return blk;
}
