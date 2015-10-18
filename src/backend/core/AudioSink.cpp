#include "AudioSink.h"
#include "filters/CubicInterpFilter.h"
#include <cassert>
#include <chrono>

/* Dry run allows us to debug the high-level code more quickly */
//#define DRY_RUN 1

using namespace std;

const int AudioSink::m_DefaultSampleRate = 44100;
const int AudioSink::m_DefaultNumChannels = 2;

// Low-latency defaults
const int AudioSink::m_DefaultMinPlaybackBufSize = 32; // alternatively, 64
const int AudioSink::m_DefaultBlockQueueCapacity = 64; // alternatively, 128

const int AudioSink::m_PaFramesPerBuffer = 256;

int AudioSink::PaCallback(const void * inputBuffer, void * outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags, void * userData)
{
	AudioSink * sink = (AudioSink *) userData;
	(void) timeInfo;
	(void) statusFlags;
	(void) inputBuffer;
	sink->DoPaCallback((float *) outputBuffer, framesPerBuffer);
	return 0;
}

AudioSink::AudioSink(int sampleRate, int numChannels, int minPlaybackBufSize,
		int blockQueueCapacity)
: m_ClickRemovalFilter(new CubicInterpFilter), m_NumBlockQueueWaiters(0),
  m_SampleRate(sampleRate), m_NumChannels(numChannels),
  m_MinPlaybackBufSize(minPlaybackBufSize), m_Capacity(blockQueueCapacity),
  m_Buffering(false), m_Stream(nullptr), m_SinkRunning(false) {
}

AudioSink::~AudioSink() {
	// TODO Auto-generated destructor stub
}

AudioSink & AudioSink::Instance()
{
	static AudioSink inst(m_DefaultSampleRate, m_DefaultNumChannels,
			m_DefaultMinPlaybackBufSize, m_DefaultBlockQueueCapacity);
	return inst;
}

Filter & AudioSink::getClickRemovalFilter()
{
	return *m_ClickRemovalFilter;
}

const Filter & AudioSink::getClickRemovalFilter() const
{
	return *m_ClickRemovalFilter;
}

void AudioSink::takeClickRemovalFilter(std::unique_ptr<Filter> && filter)
{
	m_ClickRemovalFilter = std::move(filter);
}

bool AudioSink::StartSink()
{
	PaError err = paStreamIsNotStopped;
	if (m_Stream == nullptr)
	{
		err = Pa_Initialize();
		if (err == paNoError)
		{
			err = Pa_OpenDefaultStream(&m_Stream, 0, m_NumChannels, paFloat32,
					m_SampleRate, m_PaFramesPerBuffer, &PaCallback, this);
			if (err == paNoError)
			{
				m_Buffering = true;
				err = Pa_StartStream(m_Stream);
				if (err != paNoError)
				{
					Pa_CloseStream(m_Stream);
				}
			}
			else
			{
				Pa_Terminate();
			}
		}
		if (err == paNoError)
		{
			m_SinkRunning = true;
		}
	}
	return err == paNoError;
}

bool AudioSink::StopSink()
{
	PaError err = paStreamIsStopped;
	if (m_Stream != nullptr)
	{
		{
			unique_lock<mutex> lck(m_BlockQueueMutex);
			m_SinkRunning = false;
			m_BlockQueueBelowCapacityCond.notify_all();
			m_BlockQueueWaitingForWaiters.notify_all();
		}
		Pa_StopStream(m_Stream);
		m_DequeuedBlock.reset();
		FlushBlockQueue();
		Pa_CloseStream(m_Stream);
		err = Pa_Terminate();
	}
	return err == paNoError;
}

void AudioSink::SubmitAudioBlock(const shared_ptr<AudioBlock> & block)
{
	// PRODUCER
	if (block != nullptr && block->getNumSamples() != 0)
	{
		unique_lock<mutex> lck(m_BlockQueueMutex);
		if (!m_HeldBackBlocks.IsEmpty())
		{
			int readyFlags;
			if (m_HeldBackNextBlock)
			{
				m_HeldBackNextBlock->append(block);
				readyFlags = m_ClickRemovalFilter->GetFilterReadyFlags(
						m_HeldBackBlocks, *m_HeldBackNextBlock, 1);
			}
			else
			{
				readyFlags = m_ClickRemovalFilter->GetFilterReadyFlags(
						m_HeldBackBlocks, *block, 1);
			}

			bool updateQueue = false;
			if (readyFlags == 0)
			{
				assert(m_HeldBackNextBlock == nullptr);
				m_HeldBackBlocks.Add(block);
			}
			else if (readyFlags == Filter::GetLeftBlockReadyFlag())
			{
				if (m_HeldBackNextBlock == nullptr &&
						block->isClickRemovalSet())
				{
					m_HeldBackNextBlock = block;
				}
			}
			else if (m_HeldBackNextBlock != nullptr)
			{
				assert(readyFlags == Filter::GetBothBlocksReadyFlag());
				updateQueue = true;
			}
			else if (readyFlags == Filter::GetBothBlocksReadyFlag() &&
					block->isClickRemovalSet())
			{
				updateQueue = true;
				m_HeldBackNextBlock = block;
			}

			if (!updateQueue && m_HeldBackNextBlock == nullptr)
			{
				updateQueue =
					m_ClickRemovalFilter->IsQueueReadyAfterQueueFetchAndAdd(
							m_HeldBackBlocks, *block, 1);
				m_HeldBackBlocks.Add(block);
			}

			if (updateQueue)
			{
				if (m_HeldBackNextBlock != nullptr)
				{
					assert(m_HeldBackNextBlock->isClickRemovalSet());
					ApplyFilter(*m_ClickRemovalFilter,
							m_HeldBackBlocks.CreateBlock(),
							m_HeldBackNextBlock);
					m_HeldBackBlocks.Add(m_HeldBackNextBlock);
					m_HeldBackNextBlock.reset();
				}

				std::shared_ptr<AudioBlock> oldBlk = m_HeldBackBlocks.Remove();
#ifndef DRY_RUN
				// Wait while queue is full
				m_NumBlockQueueWaiters++;
				m_BlockQueueBelowCapacityCond.wait(lck,
						[this]{ return !m_SinkRunning || m_BlockQueue.size() < m_Capacity; });
				m_NumBlockQueueWaiters--;
				// Add item to back of queue
				if (m_SinkRunning)
				{
					m_BlockQueue.push(oldBlk);
					m_BlockQueueWaitingForWaiters.notify_all();
				}
#endif
			}
		}
		else
		{
			m_HeldBackBlocks.Add(block);
		}
	}
}

void AudioSink::DoPaCallback(float * outputBuffer,
		unsigned long framesPerBuffer)
{
	// Here, we model the playback as a stream of samples (similar to what is
	// used for Internet radio)
	for (unsigned long k = 0; k < framesPerBuffer; ++k)
	{
		// CONSUMER
		bool haveNoBlock =
				m_DequeuedBlock == nullptr || m_DequeuedBlock->atEnd();
		if (haveNoBlock)
		{
			lock_guard<mutex> lck(m_BlockQueueMutex);
			bool processNext = true;
			while (processNext)
			{
				processNext = false;
				if (!m_Buffering || m_BlockQueue.size() >= m_MinPlaybackBufSize)
				{
					if (m_BlockQueue.empty())
					{
						// Either no more audio or an underrun in the buffer
						m_Buffering = true;
					}
					else
					{
						m_Buffering = false;
						m_DequeuedBlock = m_BlockQueue.front();
						haveNoBlock = processNext = m_DequeuedBlock->atEnd();
						m_BlockQueue.pop();
						m_BlockQueueBelowCapacityCond.notify_all();
					}
				}
			}
		}
		if (haveNoBlock)
		{
			// Either no more audio or an underrun in the buffer
			for (int ch = 0; ch < m_NumChannels; ++ch)
			{
				*outputBuffer++ = 0.0f;
			}
		}
		else
		{
			for (int ch = 0; ch < m_NumChannels; ++ch)
			{
				*outputBuffer++ = m_DequeuedBlock->getSample(ch);
			}
			m_DequeuedBlock->incrementReadPosition();
		}
	}
}

void AudioSink::FlushBlockQueue()
{
	unique_lock<mutex> lck(m_BlockQueueMutex);
	int numBlockQueueWaiters = m_NumBlockQueueWaiters;
	bool done = false;
	do
	{
		while (!m_BlockQueue.empty())
		{
			m_BlockQueue.pop();
		}
		done = numBlockQueueWaiters <= 0;
		if (!done)
		{
			m_BlockQueueBelowCapacityCond.notify_all();
			m_BlockQueueWaitingForWaiters.wait(
				lck, [this, numBlockQueueWaiters] {
					return !m_SinkRunning || numBlockQueueWaiters > m_NumBlockQueueWaiters; });
			numBlockQueueWaiters = m_NumBlockQueueWaiters;
		}
	}
	while (!done);
}

void AudioSink::ApplyFilter(Filter & filter,
			const std::shared_ptr<AudioBlock> & block,
			const std::shared_ptr<AudioBlock> & nextBlock)
{
	double timeInterval = filter.GetTimeInterval();
	size_t numSamples = (size_t) (timeInterval * m_SampleRate);
	size_t leftNumSamples = numSamples >> 1;
	size_t rightNumSamples = (numSamples + 1) >> 1;
	size_t leftBlockSize = block->getNumSamples();
	//size_t rightBlockSize = nextBlock->getNumSamples();

	for (size_t ch = 0; ch < (size_t) getNumChannels(); ++ch)
	{
		CubicInterpFilter * cif = dynamic_cast<CubicInterpFilter*>(&filter);
		if (cif != nullptr && m_ClickRemovalFilter.get() == cif)
		{
			// Set up click removal
			cif->Reset();
			cif->GetLeftBlockInformation(*block, ch);
			cif->GetRightBlockInformation(*nextBlock, ch);
		}
		size_t leftStart = leftBlockSize - leftNumSamples;
		for (size_t k = leftStart; k < leftBlockSize; ++k)
		{
			float t = (k - leftStart) / ((double) numSamples);
			block->setSampleAtPosition(ch, k,
					filter.ProcessSample(
							block->getSampleAtPosition(ch, k),
							t));
		}
		for (size_t k = 0; k < rightNumSamples; ++k)
		{
			float t = (k + leftNumSamples) / ((double) numSamples);
			nextBlock->setSampleAtPosition(ch, k, filter.ProcessSample(
							nextBlock->getSampleAtPosition(ch, k), t));
		}
	}
}
