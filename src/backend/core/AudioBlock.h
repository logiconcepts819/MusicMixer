#ifndef SRC_CORE_AUDIOBLOCK_H_
#define SRC_CORE_AUDIOBLOCK_H_

#include <memory>
#include <vector>
#include <array>
#include <cstdint>

class AudioBlock {
	// First dimension: channel number, second dimension: buffer position
	std::vector<std::vector<float> > m_Samples;
	size_t m_ReadPos;
	bool m_RemoveClick;
public:
	AudioBlock();
	virtual ~AudioBlock();

	void initializeChannels();

	size_t getReadPosition() const { return m_ReadPos; }
	void setReadPosition(size_t readPos) { m_ReadPos = readPos; }
	void incrementReadPosition() { ++m_ReadPos; }
	bool isClickRemovalSet() const { return m_RemoveClick; }
	void setRemoveClick(bool enable) { m_RemoveClick = enable; }

	float getSample(int channel) const
	{
		return m_Samples.at(channel).at(m_ReadPos);
	}

	float getSample(int channel, ssize_t offset) const
	{
		return m_Samples.at(channel).at(m_ReadPos + offset);
	}

	float getSampleAtPosition(int channel, size_t position) const
	{
		return m_Samples.at(channel).at(position);
	}

	void setSample(int channel, float sample)
	{
		m_Samples.at(channel).at(m_ReadPos) = sample;
	}

	void setSample(int channel, ssize_t offset, float sample)
	{
		m_Samples.at(channel).at(m_ReadPos + offset) = sample;
	}

	void setSampleAtPosition(int channel, size_t position, float sample)
	{
		m_Samples.at(channel).at(position) = sample;
	}

	size_t getNumSamples() const
	{
		return m_Samples.empty() ? 0 : m_Samples.at(0).size();
	}

	bool atEnd() const
	{
		return m_Samples.empty() || m_Samples.at(0).size() <= m_ReadPos;
	}

	void pushChannelData(const float * data, int count)
	{
		m_Samples.push_back(std::vector<float>(data, data + count));
	}

	void pushChannelData(const int16_t * data, int count)
	{
		std::vector<float> theData;
		for (int k = 0; k < count; ++k)
		{
			theData.push_back(((float) data[k]) / ((float) INT16_MAX));
		}
		m_Samples.push_back(std::move(theData));
	}

	void addChannelData(int channel, float data)
	{
		m_Samples.at(channel).push_back(data);
	}

	void setChannelData(int channel, const float * data, int count)
	{
		m_Samples.at(channel).assign(data, data + count);
	}

	void setChannelData(int channel, const int16_t * data, int count)
	{
		std::vector<float> theData;
		for (int k = 0; k < count; ++k)
		{
			theData.push_back(((float) data[k]) / ((float) INT16_MAX));
		}
		m_Samples.at(channel) = std::move(theData);
	}

	std::shared_ptr<AudioBlock> split(size_t position);
	std::shared_ptr<AudioBlock> splitBackwards(size_t position);
	void merge(const std::shared_ptr<AudioBlock> & source);
	void clear();
	void set(const std::shared_ptr<AudioBlock> & source);
	void swap(std::shared_ptr<AudioBlock> & source);
	void append(const std::shared_ptr<AudioBlock> & source);
	std::shared_ptr<AudioBlock> overlap(std::shared_ptr<AudioBlock> & otherBlock, ssize_t otherBlockOffset, bool & isOverlapEmbedded);
};

#endif /* SRC_CORE_AUDIOBLOCK_H_ */
