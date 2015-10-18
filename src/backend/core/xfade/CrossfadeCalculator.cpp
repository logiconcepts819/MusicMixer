#include "CrossfadeCalculator.h"
#include "DJCrossfadeCalculatorOld.h"
#include "../AudioBlock.h"
#include "../AudioFile.h"
#include "../RequestQueue.h"
#include <algorithm>

const double CrossfadeCalculator::m_Epsilon = 64.0 * std::numeric_limits<double>::epsilon();

CrossfadeCalculator::CrossfadeCalculator(const AudioFile & audioFile1, const AudioFile & audioFile2)
: m_AudioFile1(&audioFile1), m_AudioFile2(&audioFile2),
  m_CrossfadeTime(RequestQueue::GetDefaultXfadeDuration())
{
}

CrossfadeCalculator::~CrossfadeCalculator()
{
}

double CrossfadeCalculator::GetEpsilon()
{
	return m_Epsilon;
}

double CrossfadeCalculator::GetTimeAtStartOfFadeOut() const
{
	return m_AudioFile1->getDuration() - m_CrossfadeTime;
}

double CrossfadeCalculator::GetTimeAtStartOfFadeIn() const
{
	return 0.0;
}

double CrossfadeCalculator::ComputePercentageChangeForFadeOutTrack(double percent, double time, double timeChange, double & newTime, bool & done)
{
	(void) percent; // Ignore for simple crossfade
	newTime = time + timeChange;
	double value = timeChange / m_CrossfadeTime;
	done = value >= 1.0 - m_Epsilon;
	return value;
}

double CrossfadeCalculator::ComputePercentageChangeForFadeInTrack(double percent, double time, double timeChange, double & newTime, bool & done)
{
	(void) percent; // Ignore for simple crossfade
	newTime = time + timeChange;
	double value = timeChange / m_CrossfadeTime;
	done = value >= 1.0 - m_Epsilon;
	return value;
}

double CrossfadeCalculator::ComputePercentageChangeForTrack(bool fadeOut, double percent, double time, double timeChange, double & newTime, bool & done)
{
	return fadeOut ? ComputePercentageChangeForFadeOutTrack(percent, time, timeChange, newTime, done)
	               : ComputePercentageChangeForFadeInTrack(percent, time, timeChange, newTime, done);
}

double CrossfadeCalculator::ComputeOriginalTimeChangeForFadeOutTrack(double percent, double percentChange)
{
	(void) percent;
	return percentChange * m_CrossfadeTime;
}

double CrossfadeCalculator::ComputeOriginalTimeChangeForFadeInTrack(double percent, double percentChange)
{
	(void) percent;
	return percentChange * m_CrossfadeTime;
}

double CrossfadeCalculator::ComputeVolumeForFadeOutTrack(double percent) const
{
	return 1.0 - ComputeVolumeForFadeInTrack(percent);
}

double CrossfadeCalculator::ComputeVolumeForFadeInTrack(double percent) const
{
	return std::min(std::max(percent, 0.0), 1.0);
}

double CrossfadeCalculator::ComputeVolumeForTrack(bool fadeOut, double percent) const
{
	return fadeOut ? ComputeVolumeForFadeOutTrack(percent)
	               : ComputeVolumeForFadeInTrack(percent);
}

double CrossfadeCalculator::ComputeStretchFactorForFadeOutTrack(double percent) const
{
	(void) percent;
	return 1.0;
}

double CrossfadeCalculator::ComputeStretchFactorForFadeInTrack(double percent) const
{
	(void) percent;
	return 1.0;
}

double CrossfadeCalculator::ComputeStretchFactorForTrack(bool fadeOut, double percent) const
{
	return fadeOut ? ComputeStretchFactorForFadeOutTrack(percent)
	               : ComputeStretchFactorForFadeInTrack(percent);
}

double CrossfadeCalculator::GetFadeOutPercentage() const
{
	return 0.0;
}

double CrossfadeCalculator::GetFadeInPercentage() const
{
	return 0.0;
}

double CrossfadeCalculator::ComputeOriginalTimeChangeForTrack(bool fadeOut, double percent, double percentChange)
{
	(void) percent;
	return fadeOut ? ComputeOriginalTimeChangeForFadeOutTrack(fadeOut, percentChange)
	               : ComputeOriginalTimeChangeForFadeInTrack(fadeOut, percentChange);
}

bool CrossfadeCalculator::CheckCrossfadeCondition()
{
	return m_AudioFile1->getDuration() >= m_CrossfadeTime &&
	       m_AudioFile2->getDuration() >= m_CrossfadeTime;
}

DJCrossfadeCalculatorOld * CrossfadeCalculator::AsDJCalculator()
{
	return dynamic_cast<DJCrossfadeCalculatorOld*>(this);
}
