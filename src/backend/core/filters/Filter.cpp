#include "Filter.h"
#include "HoldBackQueue.h"
#include "../AudioBlock.h"
#include "../AudioSink.h"

const float Filter::m_DefaultTimeInterval = 0.001f;
const size_t Filter::m_MinNumSamples = 6;
const int Filter::m_LeftBlockReady = 1;
const int Filter::m_RightBlockReady = 2;

Filter::Filter() : m_TimeInterval(m_DefaultTimeInterval)
{
	// TODO Auto-generated constructor stub
}

Filter::~Filter() {
	// TODO Auto-generated destructor stub
}

float Filter::GetMinTimeInterval() const
{
	return m_MinNumSamples / ((float) AudioSink::Instance().getSampleRate());
}

void Filter::SetTimeInterval(float timeInterval)
{
	m_TimeInterval = std::max(timeInterval, GetMinTimeInterval());
}

int Filter::GetLeftBlockReadyFlag()
{
	return m_LeftBlockReady;
}

int Filter::GetRightBlockReadyFlag()
{
	return m_RightBlockReady;
}

int Filter::GetBothBlocksReadyFlag()
{
	return GetLeftBlockReadyFlag() | GetRightBlockReadyFlag();
}

int Filter::GetFilterReadyFlagsAux(size_t queueSize, const AudioBlock * block, ssize_t extraMargin) const
{
	int flags = 0;
	size_t sampRate = (size_t) AudioSink::Instance().getSampleRate();
	size_t numSamples = (size_t) (m_TimeInterval * sampRate);
	size_t leftNumSamples = (numSamples >> 1) + extraMargin;
	size_t leftBlockSize = queueSize;
	if (leftBlockSize >= leftNumSamples)
	{
		flags |= m_LeftBlockReady;
	}
	if (block != nullptr)
	{
		size_t rightNumSamples = ((numSamples + 1) >> 1) + extraMargin;
		size_t rightBlockSize = block->getNumSamples();
		if (rightBlockSize >= rightNumSamples)
		{
			flags |= m_RightBlockReady;
		}
	}
	return flags;
}

int Filter::GetFilterReadyFlags(const HoldBackQueue & hbqueue, const AudioBlock & block, ssize_t extraMargin) const
{
	return GetFilterReadyFlagsAux(hbqueue.GetTotalSize(), &block, extraMargin);
}

bool Filter::IsQueueReadyAfterQueueFetch(const HoldBackQueue & hbqueue, ssize_t extraMargin) const
{
	return GetFilterReadyFlagsAux(hbqueue.GetTotalSizeWithoutFront(), nullptr, extraMargin) != 0;
}

bool Filter::IsQueueReadyAfterQueueFetchAndAdd(const HoldBackQueue & hbqueue, const AudioBlock & block, ssize_t extraMargin) const
{
	return GetFilterReadyFlagsAux(hbqueue.GetTotalSizeWithoutFront() + block.getNumSamples(), nullptr, extraMargin) != 0;
}
