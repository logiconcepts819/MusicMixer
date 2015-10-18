#ifndef SRC_CORE_FILTERS_HOLDBACKQUEUE_H_
#define SRC_CORE_FILTERS_HOLDBACKQUEUE_H_

#include <deque>
#include <memory>

class AudioBlock;

class HoldBackQueue {
	std::deque<std::shared_ptr<AudioBlock> > queue;
	size_t totalBufSize;

public:
	HoldBackQueue();
	virtual ~HoldBackQueue();

	void Add(const std::shared_ptr<AudioBlock> & item);
	std::shared_ptr<AudioBlock> Remove();
	std::shared_ptr<AudioBlock> RemoveUntilNextClickRemovalBlock();

	size_t GetBackBufferSize() const;
	size_t GetFrontBufferSize() const;

	size_t GetTotalSize() const;
	size_t GetTotalSizeWithoutFront() const;

	std::shared_ptr<AudioBlock> CreateBlock() const;

	bool IsEmpty() const;
	void Clear();
};

#endif /* SRC_CORE_FILTERS_HOLDBACKQUEUE_H_ */
