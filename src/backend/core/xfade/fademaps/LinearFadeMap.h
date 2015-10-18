#ifndef SRC_CORE_XFADE_FADEMAPS_LINEARFADEMAP_H_
#define SRC_CORE_XFADE_FADEMAPS_LINEARFADEMAP_H_

#include "FadeMap.h"

class LinearFadeMap: public FadeMap {
public:
	LinearFadeMap();
	virtual ~LinearFadeMap();

	double MapCrossfadeVolume(double volume) const
	{
		return volume;
	}
};

#endif /* SRC_CORE_XFADE_FADEMAPS_LINEARFADEMAP_H_ */
