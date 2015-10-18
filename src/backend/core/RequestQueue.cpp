//#define TEST_AUDIO_SINK 1

#include "RequestQueue.h"
#include "AudioFile.h"
#include "AudioBlock.h"
#include "AudioSink.h"
#include "xfade/Crossfader.h"
#include "xfade/fademaps/LinearFadeMap.h"
#ifdef TEST_AUDIO_SINK
#include <cmath>
#include <cassert>
#endif

using namespace std;

const double RequestQueue::m_DefaultXfadeDuration = 5.0;

double RequestQueue::GetDefaultXfadeDuration()
{
	return m_DefaultXfadeDuration;
}

void RequestQueue::ProcessRequests(RequestQueue * reqQueue)
{
	reqQueue->DoProcessRequests();
}

RequestQueue::RequestQueue()
: m_TerminateThread(false), m_ThreadRunning(false),
  m_EnableNormalXfade(false), m_EnableDJXFade(false),
  m_XfadeDuration(m_DefaultXfadeDuration), m_UseOptimisticTempoAdaptation(true)
{
	// TODO Auto-generated constructor stub
	// Set default fade map to linear fade map
	m_FadeMap.reset(new LinearFadeMap);
}

RequestQueue::~RequestQueue() {
	// TODO Auto-generated destructor stub
}

RequestQueue & RequestQueue::Instance()
{
	static RequestQueue inst;
	return inst;
}

FadeMap & RequestQueue::GetFadeMap()
{
	return *m_FadeMap.get();
}

const FadeMap & RequestQueue::GetFadeMap() const
{
	return *m_FadeMap.get();
}

void RequestQueue::TakeFadeMap(std::unique_ptr<FadeMap> && fadeMap)
{
	m_FadeMap = std::move(fadeMap);
}

void RequestQueue::Play(const string & filename)
{
	// Costs less (to the people) here.  Less rude.
	lock_guard<mutex> lck(m_ThreadMutex);
	m_DEQueue.push_back(make_shared<AudioRequest>(filename));
	m_DEQueueHasDataCond.notify_all();
}

void RequestQueue::PlayNext(const string & filename)
{
	// Costs more (to the people) here.  More rude.
	lock_guard<mutex> lck(m_ThreadMutex);
	m_DEQueue.push_front(make_shared<AudioRequest>(filename));
	m_DEQueueHasDataCond.notify_all();
}

void RequestQueue::StartRequestProcessor()
{
	lock_guard<mutex> lck(m_ThreadMutex);
	if (!m_ThreadRunning)
	{
		m_ThreadRunning = true;
		m_RequestThread.reset(new thread(ProcessRequests, this));
	}
}

void RequestQueue::StopRequestProcessor()
{
	{
		lock_guard<mutex> lck(m_ThreadMutex);
		m_TerminateThread = true;
		m_DEQueueHasDataCond.notify_all();
	}
	m_RequestThread->join();
}

void RequestQueue::StopRequestProcessorAndSink()
{
	{
		lock_guard<mutex> lck(m_ThreadMutex);
		m_TerminateThread = true;
	}
	AudioSink::Instance().StopSink();
	{
		lock_guard<mutex> lck(m_ThreadMutex);
		m_DEQueueHasDataCond.notify_all();
	}
	m_RequestThread->join();
	m_DEQueue.clear();
}

void RequestQueue::ProcessNextRequest()
{
	shared_ptr<AudioRequest> request;
	bool terminateThread = false;
	if (m_NextAudioFile == nullptr)
	{
		{
			unique_lock<mutex> lck(m_ThreadMutex);
			if (!m_TerminateThread && m_DEQueue.empty())
			{
				m_DEQueueHasNoDataCond.notify_all();
				m_DEQueueHasDataCond.wait(lck,
					[this] { return !m_DEQueue.empty() || m_TerminateThread; });
			}
			terminateThread = m_TerminateThread;
			if (!terminateThread)
			{
				request = move(m_DEQueue.front());
				m_DEQueue.pop_front();
			}
		}
		if (!terminateThread)
		{
			m_AudioFile.reset(new AudioFile(
					AudioSink::Instance().getNumChannels(),
					AudioSink::Instance().getSampleRate()));
			m_AudioFile->setFilename(request->getFilename(), true);
		}
	}
	if (!terminateThread)
	{
		// TODO Implement sweepers and commercial breaks
		//
		// The idea for crossfade:
		// 1. When we approach the end of the song, we stop adding stuff to the
		//    sink queue, and starting adding the remaining stuff to the
		//    crossfade queue.  Then, we break from this thread and proceed as
		//    if we are opening the next audio file.
		// 2. When we open the other file, we read from the crossfade queue and
		//    start blending that with the blocks read from the other file.
		//    Those blended blocks then go to the sink queue.  Once the
		//    crossfade queue is depleted, we are entirely in the next song.
		//
		// Crossfade styles will be implemented in priorities:
		// 1. If DJ-style crossfade is enabled, and BPM information is
		//    available for both tracks, DJ-style crossfade will take place.
		// 2. If DJ-style crossfade is disabled, or DJ-style crossfade is
		//    enabled but BPM information is unavailable for either track,
		//    then we have either of the following two scenarios:
		//    a. If normal crossfade is enabled and both tracks are
		//       long enough for the desired crossfade duration, then normal
		//       crossfade will take place.
		//    b. Otherwise, the two tracks remain disjoint.
		// 3. If both DJ-style and normal crossfade are disabled, then the two
		//    tracks remain disjoint (ditto).

		bool isCrossfading = false;
		bool removeClicks = true;
		if (m_Crossfader != nullptr || m_AudioFile->OpenFile())
		{
			OnMetadataLoaded(m_NextAudioFile == nullptr ? *m_AudioFile
			                                            : *m_NextAudioFile);
			if (m_Crossfader != nullptr)
			{
				std::shared_ptr<AudioBlock> leftover;
				m_Crossfader->WaitOnThreadsAndGiveXfadeLeftover(leftover);
				if (leftover != nullptr)
				{
					leftover->setRemoveClick(removeClicks);
					removeClicks = false;
					AudioSink::Instance().SubmitAudioBlock(
							leftover);
				}
				m_Crossfader.reset();
				m_AudioFile.swap(m_NextAudioFile);
				m_NextAudioFile.reset();
			}

			bool crossfadeFailed = false;
			AudioRequest * frontRequest = nullptr;
			while (!terminateThread && !isCrossfading && !m_AudioFile->isFileDone())
			{
				std::string filename;

				// Obtain next audio block
				shared_ptr<AudioBlock> blk = m_AudioFile->getNextAudioBlock();
				OnPositionUpdate(*m_AudioFile);

				// Check crossfade conditions and do the crossfade at the
				// right time

				// LOTS OF LOCK CONTENTION HERE!
				double backDelta = 0.0;
				{
					unique_lock<mutex> lck(m_ThreadMutex);
					AudioRequest * newFrontRequest = nullptr;
					if (!m_DEQueue.empty())
					{
						newFrontRequest = m_DEQueue.front().get();
					}
					if (newFrontRequest != frontRequest)
					{
						frontRequest = newFrontRequest;
						if (newFrontRequest != nullptr)
						{
							m_NextAudioFile.reset(new AudioFile(
									AudioSink::Instance().getNumChannels(),
									AudioSink::Instance().getSampleRate()));
							m_NextAudioFile->setFilename(
									frontRequest->getFilename(), true);
							m_Crossfader.reset(new Crossfader(*m_AudioFile,
									*m_NextAudioFile, *m_FadeMap));
							m_Crossfader->setAllowingCrossfade(m_EnableNormalXfade);
							m_Crossfader->setAllowingDJCrossfade(m_EnableDJXFade);
							m_Crossfader->setCrossfadeTime(m_XfadeDuration);
							m_Crossfader->InitializeCrossfade();
							crossfadeFailed = !m_Crossfader->isEligible();
							if (crossfadeFailed)
							{
								m_Crossfader.reset();
								m_NextAudioFile.reset();
							}
						}
					}

					if (m_Crossfader != nullptr)
					{
						isCrossfading = m_Crossfader->ReadyToCrossfade(backDelta);
						if (isCrossfading)
						{
							m_DEQueue.pop_front();
						}
					}
				}

				if (isCrossfading && !crossfadeFailed)
				{
					crossfadeFailed = !m_NextAudioFile->OpenFile();
					if (crossfadeFailed)
					{
						m_Crossfader.reset();
						m_NextAudioFile.reset();
					}
					else
					{
						size_t numSamples = blk->getNumSamples();
						size_t sampDepletion = backDelta * AudioSink::Instance().getSampleRate();
						shared_ptr<AudioBlock> fadeOutBlk = blk->split(
								numSamples > sampDepletion ?
								numSamples - sampDepletion : 0);
						m_Crossfader->StartCrossfade(fadeOutBlk);
					}
				}

				blk->setRemoveClick(removeClicks);
				removeClicks = false;
				AudioSink::Instance().SubmitAudioBlock(std::move(blk));
			}
		}
	}
}

void RequestQueue::DoProcessRequests()
{
	m_ThreadMutex.lock();
	while (!m_TerminateThread)
	{
		m_ThreadMutex.unlock();
		ProcessNextRequest();
		m_ThreadMutex.lock();
	}
	m_TerminateThread = false;
	m_ThreadRunning = false;
	m_ThreadMutex.unlock();
}

void RequestQueue::WaitForEmptyQueue()
{
	unique_lock<mutex> lck(m_ThreadMutex);
	m_DEQueueHasNoDataCond.wait(lck, [this] { return m_DEQueue.empty(); });
}
