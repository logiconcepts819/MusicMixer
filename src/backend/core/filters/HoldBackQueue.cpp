#include "HoldBackQueue.h"
#include "../AudioBlock.h"

HoldBackQueue::HoldBackQueue() : totalBufSize(0) {
	// TODO Auto-generated constructor stub

}

HoldBackQueue::~HoldBackQueue() {
	// TODO Auto-generated destructor stub
}

void HoldBackQueue::Add(const std::shared_ptr<AudioBlock> & item)
{
	queue.push_back(item);
	totalBufSize += item->getNumSamples();
}

std::shared_ptr<AudioBlock> HoldBackQueue::Remove()
{
	std::shared_ptr<AudioBlock> item;
	if (!queue.empty())
	{
		item = queue.front();
		totalBufSize -= item->getNumSamples();
		queue.pop_front();
	}
	return item;
}

std::shared_ptr<AudioBlock> HoldBackQueue::RemoveUntilNextClickRemovalBlock()
{
	std::shared_ptr<AudioBlock> item = Remove();
	while (!queue.empty() && queue.front()->isClickRemovalSet())
	{
		item->append(queue.front());
		totalBufSize -= queue.front()->getNumSamples();
		queue.pop_front();
	}
	return item;
}

size_t HoldBackQueue::GetBackBufferSize() const
{
	return queue.empty() ? 0 : queue.back()->getNumSamples();
}

size_t HoldBackQueue::GetFrontBufferSize() const
{
	return queue.empty() ? 0 : queue.front()->getNumSamples();
}

size_t HoldBackQueue::GetTotalSize() const
{
	return totalBufSize;
}

size_t HoldBackQueue::GetTotalSizeWithoutFront() const
{
	return totalBufSize - GetFrontBufferSize();
}

std::shared_ptr<AudioBlock> HoldBackQueue::CreateBlock() const
{
	std::shared_ptr<AudioBlock> accumulate = std::make_shared<AudioBlock>();
	accumulate->initializeChannels();
	for (auto it = queue.rbegin(); it != queue.rend(); ++it)
	{
		accumulate->append(*it);
	}
	return accumulate;
}

bool HoldBackQueue::IsEmpty() const
{
	return queue.empty();
}

void HoldBackQueue::Clear()
{
	queue.clear();
}
