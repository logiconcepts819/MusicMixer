#ifndef SRC_CORE_FILTERS_FILTER_H_
#define SRC_CORE_FILTERS_FILTER_H_

#include <string>

class AudioBlock;
class HoldBackQueue;

class Filter {
	static const float m_DefaultTimeInterval;
	static const size_t m_MinNumSamples;

	static const int m_LeftBlockReady;
	static const int m_RightBlockReady;

	float GetMinTimeInterval() const;
	int GetFilterReadyFlagsAux(size_t queueSize,
				const AudioBlock * block, ssize_t extraMargin) const;
protected:
	float m_TimeInterval;
public:
	static int GetLeftBlockReadyFlag();
	static int GetRightBlockReadyFlag();
	static int GetBothBlocksReadyFlag();

	Filter();
	virtual ~Filter();

	float GetTimeInterval() const { return m_TimeInterval; }
	void SetTimeInterval(float timeInterval);

	virtual float ProcessSample(float sample, float t) const = 0;
	int GetFilterReadyFlags(const HoldBackQueue & hbqueue,
			const AudioBlock & block, ssize_t extraMargin = 0) const;
	bool IsQueueReadyAfterQueueFetch(const HoldBackQueue & hbqueue,
			ssize_t extraMargin = 0) const;
	bool IsQueueReadyAfterQueueFetchAndAdd(const HoldBackQueue & hbqueue,
			const AudioBlock & block, ssize_t extraMargin = 0) const;
};

#endif /* SRC_CORE_FILTERS_FILTER_H_ */
