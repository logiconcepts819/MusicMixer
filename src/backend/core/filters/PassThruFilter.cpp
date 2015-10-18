#include "PassThruFilter.h"

PassThruFilter::PassThruFilter() {
	// TODO Auto-generated constructor stub

}

PassThruFilter::~PassThruFilter() {
	// TODO Auto-generated destructor stub
}

float PassThruFilter::ProcessSample(float sample, float t) const
{
	(void) t;
	return sample;
}
