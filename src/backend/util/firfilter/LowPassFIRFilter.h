#ifndef SRC_UTIL_FIRFILTER_LOWPASSFIRFILTER_H_
#define SRC_UTIL_FIRFILTER_LOWPASSFIRFILTER_H_

#include "FIRFilter.h"

class LowPassFIRFilter: public FIRFilter
{
public:
	LowPassFIRFilter(size_t filterSize, float cutoffFreq);
	virtual ~LowPassFIRFilter();
};

#endif /* SRC_UTIL_FIRFILTER_LOWPASSFIRFILTER_H_ */
