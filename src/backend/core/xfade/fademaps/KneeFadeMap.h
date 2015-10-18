#ifndef SRC_CORE_XFADE_FADEMAPS_KNEEFADEMAP_H_
#define SRC_CORE_XFADE_FADEMAPS_KNEEFADEMAP_H_

#include "FadeMap.h"

class KneeFadeMap: public FadeMap {
	static const double m_DefaultKneeLocation;
	static const double m_DefaultLevelAtKnee;

	double m_KneeLocation;
	double m_LevelAtKnee;
public:
	KneeFadeMap();
	virtual ~KneeFadeMap();

	double MapCrossfadeVolume(double volume) const;
};

#endif /* SRC_CORE_XFADE_FADEMAPS_KNEEFADEMAP_H_ */
