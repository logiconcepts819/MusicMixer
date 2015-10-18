#ifndef SRC_CORE_FILTERS_CUBICINTERPFILTER_H_
#define SRC_CORE_FILTERS_CUBICINTERPFILTER_H_

#include "Filter.h"
#include <string>

class AudioBlock;

class CubicInterpFilter: public Filter {
	bool m_HaveLeftSideInformation;
	float m_LeftSample;
	float m_LeftSlope;

	bool m_HaveRightSideInformation;
	float m_RightSample;
	float m_RightSlope;

public:
	CubicInterpFilter();
	virtual ~CubicInterpFilter();

	void Reset();
	void GetLeftBlockInformation(const AudioBlock & block, size_t channel);
	void GetRightBlockInformation(const AudioBlock & block, size_t channel);

	float ProcessSample(float sample, float t) const;
};

#endif /* SRC_CORE_FILTERS_CUBICINTERPFILTER_H_ */
