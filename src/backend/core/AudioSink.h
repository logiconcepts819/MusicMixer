#ifndef SRC_CORE_AUDIOSINK_H_
#define SRC_CORE_AUDIOSINK_H_

#include "AudioBlock.h"
#include "filters/Filter.h"
#include "filters/HoldBackQueue.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <portaudio.h>

class AudioSink {
	std::mutex m_BlockQueueMutex;
	std::queue<std::shared_ptr<AudioBlock> > m_BlockQueue;
	std::condition_variable m_BlockQueueBelowCapacityCond;
	std::condition_variable m_BlockQueueWaitingForWaiters;

	HoldBackQueue m_HeldBackBlocks;
	std::shared_ptr<AudioBlock> m_HeldBackNextBlock;
	std::shared_ptr<AudioBlock> m_DequeuedBlock;
	std::unique_ptr<Filter> m_ClickRemovalFilter;

	static const int m_DefaultSampleRate;
	static const int m_DefaultNumChannels;
	static const int m_DefaultMinPlaybackBufSize;
	static const int m_DefaultBlockQueueCapacity;
	static const int m_PaFramesPerBuffer;

	int m_NumBlockQueueWaiters;
	int m_SampleRate;
	int m_NumChannels;
	size_t m_MinPlaybackBufSize;
	size_t m_Capacity;
	bool m_Buffering;
	PaStream * m_Stream;
	bool m_SinkRunning;

	static int PaCallback(const void * inputBuffer, void * outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo * timeInfo,
            PaStreamCallbackFlags statusFlags, void * userData);
	void DoPaCallback(float * outputBuffer, unsigned long framesPerBuffer);
	void FlushBlockQueue();

	void ApplyFilter(Filter & filter,
			const std::shared_ptr<AudioBlock> & block,
			const std::shared_ptr<AudioBlock> & nextBlock);

	// The constructor is private, since we can only have one instance of an
	// AudioSink
	AudioSink(int sampleRate, int numChannels, int minPlaybackBufSize,
			int blockQueueCapacity);
public:
	virtual ~AudioSink();

	// Only a single static instance of AudioSink is allowed
	static AudioSink & Instance();

	Filter & getClickRemovalFilter();
	const Filter & getClickRemovalFilter() const;
	void takeClickRemovalFilter(std::unique_ptr<Filter> && filter);

	// Accessor may be called at any time
	int getSampleRate() const { return m_SampleRate; }
	// Mutator must not be called while sink is running
	void setSampleRate(int sampleRate) { m_SampleRate = sampleRate; }

	// Accessor may be called at any time
	int getNumChannels() const { return m_NumChannels; }
	// Mutator must not be called while sink is running
	void setNumChannels(int numChannels) { m_NumChannels = numChannels; }

	// Accessor may be called at any time
	int getMinPlaybackBufSize() const { return m_MinPlaybackBufSize; }
	// Mutator must not be called while sink is running
	void setMinPlaybackBufSize(int playbackBufSize)
		{ m_MinPlaybackBufSize = playbackBufSize; }

	// Accessor may be called at any time
	int getBlockQueueCapacity() const { return m_Capacity; }
	// Mutator must not be called while sink is running
	void setBlockQueueCapacity(int capacity) { m_Capacity = capacity; }

	// WARNING:  This is not thread-safe!
	bool StartSink();
	// WARNING:  This is not thread-safe!
	bool StopSink();

	// This IS thread safe.
	void SubmitAudioBlock(const std::shared_ptr<AudioBlock> & block);
};

#endif /* SRC_CORE_AUDIOSINK_H_ */
