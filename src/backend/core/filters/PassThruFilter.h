#ifndef SRC_CORE_FILTERS_PASSTHRUFILTER_H_
#define SRC_CORE_FILTERS_PASSTHRUFILTER_H_

#include "Filter.h"

class PassThruFilter: public Filter {
public:
	PassThruFilter();
	virtual ~PassThruFilter();

	float ProcessSample(float sample, float t) const;
};

#endif /* SRC_CORE_FILTERS_PASSTHRUFILTER_H_ */
