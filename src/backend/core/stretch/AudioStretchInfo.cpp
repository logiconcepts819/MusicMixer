#include "AudioStretchInfo.h"
#include "../AudioSink.h"
#include <cstring>

AudioStretchInfo::AudioStretchInfo()
: m_LastOne(false), m_TimeRatio(1.0), m_UseRefPos(false), m_ReferencePos(0),
  m_UseComplementaryRefPos(false), m_ComplementaryReferencePos(0)
{
}

AudioStretchInfo::~AudioStretchInfo()
{
}

void AudioStretchInfo::AppendSamples(const AudioBlock & block, float scale,
	size_t start, size_t end)
{
	size_t numChannels = AudioSink::Instance().getNumChannels();
	size_t theEnd = end == std::string::npos ? block.getNumSamples() : end;
	size_t addCount = theEnd - start;
	size_t sampleSize = m_Buffer.size() / numChannels;

	size_t majIdx = numChannels * sampleSize;
	m_Buffer.resize(majIdx + numChannels * addCount);
	for (size_t k = 0; k < addCount; ++k)
	{
		for (size_t ch = 0; ch != numChannels; ++ch)
		{
			m_Buffer.at(ch + majIdx) = scale * block.getSampleAtPosition(ch, start + k);
		}
		majIdx += numChannels;
	}
}

void AudioStretchInfo::AppendSample(const float * sample, float scale)
{
	size_t numChannels = AudioSink::Instance().getNumChannels();
	size_t sampleSize = m_Buffer.size() / numChannels;

	size_t majIdx = numChannels * sampleSize;
	m_Buffer.resize(majIdx + numChannels);
	for (size_t ch = 0; ch != (size_t) AudioSink::Instance().getNumChannels(); ++ch)
	{
		size_t interleavedIndex = ch + majIdx;
		m_Buffer.at(interleavedIndex) = scale * sample[ch];
	}
}

std::shared_ptr<AudioBlock> AudioStretchInfo::GenerateBlock() const
{
	std::shared_ptr<AudioBlock> blk = std::make_shared<AudioBlock>();
	size_t numChannels = AudioSink::Instance().getNumChannels();
	size_t sampleSize = m_Buffer.size() / numChannels;

	size_t majIdx = 0;
	blk->initializeChannels();
	for (size_t k = 0; k < sampleSize; ++k)
	{
		for (size_t ch = 0; ch != numChannels; ++ch)
		{
			blk->addChannelData(ch, m_Buffer.at(ch + majIdx));
		}
		majIdx += numChannels;
	}

	return blk;
}
