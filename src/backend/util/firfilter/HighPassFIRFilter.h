#ifndef SRC_UTIL_FIRFILTER_HIGHPASSFIRFILTER_H_
#define SRC_UTIL_FIRFILTER_HIGHPASSFIRFILTER_H_

#include "FIRFilter.h"

class HighPassFIRFilter: public FIRFilter
{
public:
	HighPassFIRFilter(size_t filterSize, float cutoffFreq);
	virtual ~HighPassFIRFilter();
};

#endif /* SRC_UTIL_FIRFILTER_HIGHPASSFIRFILTER_H_ */
