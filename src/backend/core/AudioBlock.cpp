#include "AudioBlock.h"
#include "AudioSink.h"
#include <algorithm>
#include <iostream>

AudioBlock::AudioBlock() : m_ReadPos(0), m_RemoveClick(false) {
	// TODO Auto-generated constructor stub

}

AudioBlock::~AudioBlock() {
	// TODO Auto-generated destructor stub
}

void AudioBlock::initializeChannels()
{
	m_Samples.resize(AudioSink::Instance().getNumChannels());
}

std::shared_ptr<AudioBlock> AudioBlock::split(size_t position)
{
	std::shared_ptr<AudioBlock> newBlock = std::make_shared<AudioBlock>();
	if (!m_Samples.empty())
	{
		std::vector<float> newChannelData;
		for (size_t k = 0; k < m_Samples.size(); ++k)
		{
			newBlock->pushChannelData(m_Samples.at(k).data() + position,
					m_Samples.at(k).size() - position);
			m_Samples.at(k).resize(position);
		}
		if (m_ReadPos >= position)
		{
			newBlock->m_ReadPos = m_ReadPos - position;
			m_ReadPos = position;
		}
	}
	return newBlock;
}

std::shared_ptr<AudioBlock> AudioBlock::splitBackwards(size_t position)
{
	// This function is less efficient than split(), so it should be used
	// sparingly.
	std::shared_ptr<AudioBlock> newBlock = std::make_shared<AudioBlock>();
	if (!m_Samples.empty())
	{
		std::vector<float> newChannelData;
		for (size_t k = 0; k < m_Samples.size(); ++k)
		{
			newBlock->pushChannelData(m_Samples.at(k).data(), position);
			std::rotate(m_Samples.at(k).begin(),
					m_Samples.at(k).begin() + position,
					m_Samples.at(k).end());
			m_Samples.at(k).resize(m_Samples.at(k).size() - position);
		}
		std::swap(m_ReadPos, newBlock->m_ReadPos);
		std::swap(m_RemoveClick, newBlock->m_RemoveClick);
		if (newBlock->m_ReadPos >= position)
		{
			m_ReadPos = newBlock->m_ReadPos - position;
			newBlock->m_ReadPos = position;
		}
	}
	return newBlock;
}

void AudioBlock::merge(const std::shared_ptr<AudioBlock> & source)
{
	if (source != nullptr)
	{
		size_t numChannels = AudioSink::Instance().getNumChannels();
		size_t thisSize = m_Samples.at(0).size();
		size_t sourceSize = source->m_Samples.at(0).size();

		size_t smallerSize = std::min(thisSize, sourceSize);
		for (size_t ch = 0; ch < numChannels; ++ch)
		{
			if (source->m_Samples.size() > ch)
			{
				for (size_t k = 0; k < smallerSize; ++k)
				{
					m_Samples.at(ch).at(k) += source->m_Samples.at(ch).at(k);
				}
			}
		}

		if (sourceSize > thisSize)
		{
			for (size_t ch = 0; ch < numChannels; ++ch)
			{
				if (source->m_Samples.size() > ch)
				{
					m_Samples.at(ch).insert(m_Samples.at(ch).end(),
							source->m_Samples.at(ch).begin() + thisSize,
							source->m_Samples.at(ch).end());
				}
			}
		}
	}
}

void AudioBlock::append(const std::shared_ptr<AudioBlock> & source)
{
	if (source != nullptr)
	{
		size_t numChannels = AudioSink::Instance().getNumChannels();
		for (size_t ch = 0; ch < numChannels; ++ch)
		{
			if (source->m_Samples.size() > ch)
			{
				m_Samples.at(ch).insert(m_Samples.at(ch).end(),
						source->m_Samples.at(ch).begin(),
						source->m_Samples.at(ch).end());
			}
		}
	}
}

void AudioBlock::clear()
{
	m_ReadPos = 0;
	m_RemoveClick = false;
	for (size_t ch = 0; ch < m_Samples.size(); ++ch)
	{
		m_Samples.at(ch).clear();
	}
}

void AudioBlock::set(const std::shared_ptr<AudioBlock> & source)
{
	if (source == nullptr)
	{
		clear();
	}
	else
	{
		size_t numChannels = AudioSink::Instance().getNumChannels();
		m_ReadPos = source->m_ReadPos;
		m_RemoveClick = source->m_RemoveClick;
		for (size_t ch = 0; ch < numChannels; ++ch)
		{
			if (source->m_Samples.size() > ch)
			{
				m_Samples.at(ch).clear();
				m_Samples.at(ch).insert(m_Samples.at(ch).end(),
						source->m_Samples.at(ch).begin(),
						source->m_Samples.at(ch).end());
			}
		}
	}
}

void AudioBlock::swap(std::shared_ptr<AudioBlock> & source)
{
	if (source == nullptr)
	{
		source = std::make_shared<AudioBlock>();
	}
	size_t numChannels = AudioSink::Instance().getNumChannels();
	std::swap(m_ReadPos, source->m_ReadPos);
	std::swap(m_RemoveClick, source->m_RemoveClick);
	for (size_t ch = 0; ch < numChannels; ++ch)
	{
		if (source->m_Samples.size() > ch)
		{
			m_Samples.at(ch).swap(source->m_Samples.at(ch));
		}
	}
}

std::shared_ptr<AudioBlock> AudioBlock::overlap(std::shared_ptr<AudioBlock> & otherBlock, ssize_t otherBlockOffset, bool & isOverlapEmbedded)
{
	// Assuming this block and other block overlap, we have 4 cases here:
	//
	// [this block]        => [this block] [returned overlap] [other block]
	//       [other block]
	//
	// OR (similar to the above)
	//
	//       [this block] => [other block] [returned overlap] [this block]
	// [other block]
	//
	// OR
	//
	// [t h i s   b l o c k] => [this block] [returned overlap] [this block]
	//     [other block]
	//
	// OR (similar to the above)
	//
	//      [this block]       => [other block] [returned overlap] [other block]
	// [o t h e r   b l o c k]
	//

	std::shared_ptr<AudioBlock> overlap;
	size_t thisEnd;
	if (otherBlockOffset < 0)
	{
		thisEnd = (size_t) (-otherBlockOffset);
		swap(otherBlock);
	}
	else
	{
		thisEnd = (size_t) otherBlockOffset;
	}

	size_t thisSize = getNumSamples();
	if (thisSize > thisEnd)
	{
		// The two blocks have overlap

		size_t otherSize = otherBlock->getNumSamples();
		isOverlapEmbedded = thisSize + thisEnd > otherSize;
		if (isOverlapEmbedded)
		{
			// We have case 3 (or case 4)
			overlap = split(thisEnd);
			std::shared_ptr<AudioBlock> end = overlap->split(otherSize);
			overlap->merge(otherBlock);
			otherBlock->set(end);
		}
		else
		{
			// We have case 1 (or case 2)
			overlap = split(thisEnd);
			overlap->merge(otherBlock->splitBackwards(thisSize - thisEnd));
		}
	}

	return overlap;
}
