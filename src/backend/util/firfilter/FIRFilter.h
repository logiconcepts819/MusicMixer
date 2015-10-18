#ifndef SRC_UTIL_FIRFILTER_FIRFILTER_H_
#define SRC_UTIL_FIRFILTER_FIRFILTER_H_

#include <vector>
#include <deque>
#include <cstring>

class FIRFilter
{
protected:
	std::vector<float> m_Filter;
	std::deque<float> m_InputHistory;

	void ApplyWindow();
public:
	FIRFilter(size_t filterSize);
	virtual ~FIRFilter();

	float At(size_t index) const { return m_Filter.at(index); }
	void AssignFilter(std::vector<float> & vec) const { vec = m_Filter; }
	size_t GetFilterDelay() const { return m_Filter.size() >> 1; }

	float ProcessSample(float input);
};

#endif /* SRC_UTIL_FIRFILTER_FIRFILTER_H_ */
