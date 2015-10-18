#ifndef SRC_CORE_XFADE_FADEMAPS_FADEMAP_H_
#define SRC_CORE_XFADE_FADEMAPS_FADEMAP_H_

class FadeMap {
public:
	FadeMap();
	virtual ~FadeMap();

	virtual double MapCrossfadeVolume(double volume) const = 0;
};

#endif /* SRC_CORE_XFADE_FADEMAPS_FADEMAP_H_ */
