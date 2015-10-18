#include "KneeFadeMap.h"

const double KneeFadeMap::m_DefaultKneeLocation = 0.25;
const double KneeFadeMap::m_DefaultLevelAtKnee = 0.75;

KneeFadeMap::KneeFadeMap()
: m_KneeLocation(m_DefaultKneeLocation), m_LevelAtKnee(m_DefaultLevelAtKnee)
{
}

KneeFadeMap::~KneeFadeMap() {
	// TODO Auto-generated destructor stub
}

double KneeFadeMap::MapCrossfadeVolume(double volume) const
{
	double mappedVolume = 0.0;
	if (volume < m_KneeLocation)
	{
		mappedVolume = (volume / m_KneeLocation) * m_LevelAtKnee;
	}
	else
	{
		double num = 1.0 - m_LevelAtKnee;
		double offVol = volume - m_KneeLocation;
		double denom = 1.0 - m_KneeLocation;
		mappedVolume = m_LevelAtKnee + (offVol / denom) * num;
	}
	return mappedVolume;
}
