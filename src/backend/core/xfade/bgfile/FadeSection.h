#ifndef SRC_CORE_XFADE_BGFILE_FADESECTION_H_
#define SRC_CORE_XFADE_BGFILE_FADESECTION_H_

#include <vector>
#include <cstring>

class FadeSection
{
	size_t m_Time;

	double m_FadeOutTime;
	double m_InstantaneousFadeOutTempo;
	std::vector<double> m_FadeOutInfo;

public:
	FadeSection();
	virtual ~FadeSection();

	virtual const char * SectionName() const = 0;


};

#endif /* SRC_CORE_XFADE_BGFILE_FADESECTION_H_ */
