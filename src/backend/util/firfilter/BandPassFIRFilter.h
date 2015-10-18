#ifndef SRC_UTIL_FIRFILTER_BANDPASSFIRFILTER_H_
#define SRC_UTIL_FIRFILTER_BANDPASSFIRFILTER_H_

#include "FIRFilter.h"

class BandPassFIRFilter: public FIRFilter
{
public:
	BandPassFIRFilter(size_t filterSize, float lowerCutoffFreq, float upperCutoffFreq);
	virtual ~BandPassFIRFilter();
};

#endif /* SRC_UTIL_FIRFILTER_BANDPASSFIRFILTER_H_ */
