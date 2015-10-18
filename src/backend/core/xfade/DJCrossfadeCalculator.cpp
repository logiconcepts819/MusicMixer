#include "DJCrossfadeCalculator.h"
#include "../AudioFile.h"
#include <string>
#include <sstream>
#include <utility>
#include <limits>
#include <algorithm>
#include <cmath>

const std::string DJCrossfadeCalculator::m_bpmInfoFileExt = ".bpm";

DJCrossfadeCalculator::DJCrossfadeCalculator(const AudioFile & audioFile1, const AudioFile & audioFile2)
: CrossfadeCalculator(audioFile1, audioFile2), m_FadeOutTime(0.0),
  m_InstantaneousFadeOutTempo(0.0), m_FadeInTime(0.0),
  m_InstantaneousFadeInTempo(0.0), m_UseOptimisticTempoAdaptation(false)
{
	// TODO Auto-generated constructor stub
}

DJCrossfadeCalculator::~DJCrossfadeCalculator() {
	// TODO Auto-generated destructor stub
}

bool DJCrossfadeCalculator::ReadBeatgrid(std::ifstream & ifs, double & instantaneousFadeTempo, std::vector<double> & fadeInfo)
{
	bool rv = false;

	int numBeats;
	double bpm;
	std::vector<std::pair<int, double> > beatgrid;
	char ch;
	while ((ifs >> numBeats >> ch >> bpm) && ch == '@')
	{
		beatgrid.push_back(std::make_pair(numBeats, bpm));
	}
	ifs.clear();

	fadeInfo.clear();
	if (!beatgrid.empty())
	{
		std::vector<std::pair<int, double> >::const_iterator
			it = beatgrid.begin();

		bool haveInstTempo = false;
		const double eps = 2.0 * std::numeric_limits<double>::epsilon();
		for (; it != beatgrid.end(); ++it)
		{
			if (it->second >= eps) // BPM must be strictly positive
			{
				if (it->first == 0)
				{
					haveInstTempo = true;
					instantaneousFadeTempo = it->second;
				}
				else
				{
					for (int k = 0; k < it->first; ++k)
					{
						fadeInfo.push_back(it->second);
					}
				}
			}
		}

		if (!haveInstTempo && !beatgrid.empty())
		{
			instantaneousFadeTempo = beatgrid.back().second;
		}

		rv = true;
	}

	return rv;
}

bool DJCrossfadeCalculator::ReadFadeInfo(std::ifstream & ifs, bool fadeOut, double & fadeTime, double & instantaneousFadeTempo, std::vector<double> & fadeInfo)
{
	bool rv = false;

	bool done = false;
	bool secondRun = false;
	double time;
	std::string parsedLine;

	while (!done && std::getline(ifs, parsedLine))
	{
		char ch1, ch2;
		if ((std::istringstream(parsedLine) >> ch1 >> time >> ch2) &&
				ch1 == '[' && ch2 == ']')
		{
			fadeTime = time;
			if (fadeOut)
			{
				if (secondRun)
				{
					done = true;
				}
				else
				{
					secondRun = true;
				}
			}
			else
			{
				done = true;
			}
		}
	}

	if (done)
	{
		rv = ReadBeatgrid(ifs, instantaneousFadeTempo, fadeInfo);
	}

	return rv;
}

bool DJCrossfadeCalculator::VerifyRubberBandConstraintsForSingleGrid(const std::vector<double> & fadeInfo, double instantaneousFadeTempo, size_t (*bgOffsetFunc)(const DJCrossfadeCalculator &)) const
{
	// We need to check whether it is possible to do the crossfade with
	// RubberBand.  If we can't, we have to resort to normal crossfade or
	// a disjoint transition into the next song.
	const double minSeekSpeed = 1.0 / 128.0; // due to a bug in RubberBand
	std::vector<double>::const_iterator it, nextIt;
	size_t k = bgOffsetFunc(*this);
	size_t N = GetMaxBeatgridSize();
	for (it = fadeInfo.begin() + k; it != fadeInfo.end(); it = nextIt)
	{
		nextIt = it + 1;

		double percent1 = k / ((double) N);
		double percent2 = (k + 1) / ((double) N);
		double normalBPM = *it;
		double normalNextBPM = nextIt == fadeInfo.end()
				? instantaneousFadeTempo : *nextIt;
		if (normalBPM < m_Epsilon)
		{
			// BPM must be strictly positive
			return false;
		}

		double stretchedBPM1 = InterpolateBPMAtCrossfade(percent1);
		double stretchedBPM2 = InterpolateBPMAtCrossfade(percent2);
		double stretchFactor1 = normalBPM / stretchedBPM1;
		double stretchFactor2 = normalNextBPM / stretchedBPM2;
		if (stretchFactor1 < minSeekSpeed || stretchFactor2 < minSeekSpeed)
		{
			// Constraint due to bug in RubberBand violated
			return false;
		}
	}
	return true;
}

bool DJCrossfadeCalculator::VerifyRubberBandConstraints() const
{
	return VerifyRubberBandConstraintsForSingleGrid(m_FadeOutInfo,
			m_InstantaneousFadeOutTempo,
			[](const DJCrossfadeCalculator & calc)
				{ return calc.GetFadeOutBeatgridOffset(); })
			&& VerifyRubberBandConstraintsForSingleGrid(m_FadeInInfo,
			m_InstantaneousFadeInTempo,
			[](const DJCrossfadeCalculator & calc)
				{ return calc.GetFadeInBeatgridOffset(); });
}

double DJCrossfadeCalculator::GetBPMAtStartOfCrossfade() const
{
	return m_FadeOutInfo.empty() ? m_InstantaneousFadeOutTempo
	                             : *m_FadeOutInfo.begin();
}

double DJCrossfadeCalculator::GetBPMAtEndOfCrossfade() const
{
	return m_FadeInInfo.empty() ? m_InstantaneousFadeInTempo
	                            : *m_FadeInInfo.rbegin();
}

size_t DJCrossfadeCalculator::GetFadeOutBeatgridOffset() const
{
	return 0; // This is zero, no matter what!
	// (I leave this defined to make the code look balanced and elegant.)
}

size_t DJCrossfadeCalculator::GetFadeInBeatgridOffset() const
{
	size_t offset = 0;
	if (m_FadeInInfo.size() < m_FadeOutInfo.size())
	{
		offset = m_FadeOutInfo.size() - m_FadeInInfo.size();
	}
	return offset;
}

size_t DJCrossfadeCalculator::GetMinBeatgridSize() const
{
	return std::min(m_FadeInInfo.size(), m_FadeOutInfo.size());
}

size_t DJCrossfadeCalculator::GetMaxBeatgridSize() const
{
	return std::max(m_FadeInInfo.size(), m_FadeOutInfo.size());
}

double DJCrossfadeCalculator::GetTimeAtFadeOutBeat(size_t beatPos) const
{
	double delta = 0.0;
	size_t count = 0;
	for (std::vector<double>::const_iterator it = m_FadeOutInfo.begin();
			count != beatPos && it != m_FadeOutInfo.end(); ++it, ++count)
	{
		delta += 1.0 / *it;
	}
	return m_FadeOutTime + delta;
}

double DJCrossfadeCalculator::GetTimeAtEndOfFadeOut() const
{
	return GetTimeAtFadeOutBeat(m_FadeOutInfo.size());
}

double DJCrossfadeCalculator::GetTimeAtFadeInBeat(size_t beatPos) const
{
	double delta = 0.0;
	size_t count = 0;
	for (std::vector<double>::const_iterator it = m_FadeInInfo.begin();
			count != beatPos && it != m_FadeInInfo.end(); ++it, ++count)
	{
		delta += 1.0 / *it;
	}
	return m_FadeInTime + delta;
}

double DJCrossfadeCalculator::GetTimeAtEndOfFadeIn() const
{
	return GetTimeAtFadeOutBeat(m_FadeInInfo.size());
}

double DJCrossfadeCalculator::InterpolateBPMAtCrossfade(double percent) const
{
	// We could have used time as an argument to this function.  However,
	// since the playback speed will vary for both tracks, we have no intuitive
	// concept of time, so we must use a percentage into the crossfade instead.
	//
	// For ending track:
	// Start at our normal speed, and adapt to the speed of the other track
	// near the end
	//
	// For starting track:
	// Start close to the speed of the other track, and adapt to our normal
	// speed near the end
	double interpolatedBPM;
	double startBPM = GetBPMAtStartOfCrossfade();
	double endBPM = GetBPMAtEndOfCrossfade();
	// Sanitize percentage value
	percent = std::min(std::max(percent, 0.0), 1.0);
	if (m_UseOptimisticTempoAdaptation)
	{
		// If we use optimistic tempo adaptation, we assume that we adapt to
		// the speed dictated by the longer beatgrid.  This gives a more
		// natural transition into the next track, but at the cost of
		// increased CPU time and more artifacts
		interpolatedBPM = startBPM + (endBPM - startBPM) * percent;
	}
	else
	{
		// If we use pessimistic tempo adaptation, we assume that we adapt to
		// the speed dictated by the shorter beatgrid.  This saves on CPU
		// usage and results in fewer artifacts, but at the cost of a less
		// natural transition into the next track
		size_t outSize = m_FadeOutInfo.size();
		size_t inSize = m_FadeInInfo.size();
		if (outSize > inSize)
		{
			double fracOverlap = inSize / ((double) outSize); // in [0, 1]
			double minStartPercent = 1.0 - fracOverlap;
			if (percent >= minStartPercent)
			{
				if (fracOverlap < m_Epsilon)
				{
					// Prevent division by zero (in the case that
					// outSize >> inSize)
					interpolatedBPM = endBPM;
				}
				else
				{
					double alpha = (percent - minStartPercent) / fracOverlap;
					interpolatedBPM = startBPM + alpha * (endBPM - startBPM);
				}
			}
			else
			{
				interpolatedBPM = startBPM;
			}
		}
		else if (inSize == 0)
		{
			// inSize = outSize = 0 => step function
			// This case prevents division of zero by zero in the case below
			interpolatedBPM = percent < 0.0 ? startBPM : endBPM;
		}
		else
		{
			double fracOverlap = outSize / ((double) inSize); // in [0, 1]
			double maxEndPercent = fracOverlap;
			if (percent <= maxEndPercent)
			{
				if (fracOverlap < m_Epsilon)
				{
					// Prevent division by zero (in the case that
					// inSize >> outSize)
					interpolatedBPM = startBPM;
				}
				else
				{
					double beta = (maxEndPercent - percent) / fracOverlap;
					interpolatedBPM = endBPM + beta * (startBPM - endBPM);
				}
			}
			else
			{
				interpolatedBPM = endBPM;
			}
		}
	}
	return interpolatedBPM;
}

double DJCrossfadeCalculator::ComputePercentageChangeForFadeOutTrack(double percent, double time, double timeChange, double & newTime, bool & done)
{
	newTime = time + timeChange;

	double timeBase = 0.0;
	double percentChange = 0.0;
	size_t maxBGS = GetMaxBeatgridSize();
	double dIndex = percent * maxBGS;
	size_t index = (size_t) dIndex;
	size_t limit = std::min(m_FadeOutInfo.size(), index);
	bool finished = false;
	for (size_t k = 0; k < limit; ++k)
	{
		timeBase += 60.0 / m_FadeOutInfo.at(k);
	}
	double prevTime = time;
	while (!finished && index < m_FadeOutInfo.size())
	{
		double dt = 60.0 / m_FadeOutInfo.at(index);
		double updatedTime = timeBase + dt;
		if (updatedTime >= newTime)
		{
			percentChange += (newTime - prevTime) / (dt * maxBGS);
			finished = true;
		}
		else
		{
			percentChange += (updatedTime - prevTime) / (dt * maxBGS);
			prevTime = timeBase = updatedTime;
			++index;
		}
	}
	if (!finished)
	{
		percentChange = 1.0 - percent;
	}
	done = percent + percentChange >= 1.0 - m_Epsilon;
	return percentChange;
}

double DJCrossfadeCalculator::ComputePercentageChangeForFadeInTrack(double percent, double time, double timeChange, double & newTime, bool & done)
{
	newTime = time + timeChange;

	double timeBase = 0.0;
	double percentChange = 0.0;
	size_t maxBGS = GetMaxBeatgridSize();
	double dIndex = percent * maxBGS;
	size_t index = (size_t) dIndex;
	size_t limit = std::min(m_FadeInInfo.size(), index);
	bool finished = false;
	for (size_t k = 0; k < limit; ++k)
	{
		timeBase += 60.0 / m_FadeInInfo.at(k);
	}
	double prevTime = time;
	while (!finished && index < m_FadeInInfo.size())
	{
		double dt = 60.0 / m_FadeInInfo.at(index);
		double updatedTime = timeBase + dt;
		if (updatedTime >= newTime)
		{
			percentChange += (newTime - prevTime) / (dt * maxBGS);
			finished = true;
		}
		else
		{
			percentChange += (updatedTime - prevTime) / (dt * maxBGS);
			prevTime = timeBase = updatedTime;
			++index;
		}
	}
	if (!finished)
	{
		percentChange = 1.0 - percent;
	}
	done = percent + percentChange >= 1.0 - m_Epsilon;
	return percentChange;
}

double DJCrossfadeCalculator::ComputeOriginalTimeChangeForFadeOutTrack(double percent, double percentChange)
{
	double timeChange = 0.0;
	double dStartIndex = percent * GetMaxBeatgridSize();
	size_t startIndex = (size_t) dStartIndex;
	double dEndIndex = (percent + percentChange) * GetMaxBeatgridSize();
	size_t endIndex = dEndIndex;
	size_t minStart = GetFadeOutBeatgridOffset();
	if (startIndex < minStart && endIndex >= minStart)
	{
		// minStart is bracketed between startIndex and endIndex, so we
		// should include it and start from there
		startIndex = minStart;

		double startBPM = m_FadeOutInfo.at(startIndex);
		timeChange -= 60.0 * (dStartIndex - startIndex) / startBPM;
	}
	if (startIndex >= minStart && endIndex >= startIndex)
	{
		size_t relStartIndex = startIndex - minStart;
		size_t relEndIndex = endIndex - minStart;
		size_t clipEndIndex = std::max((size_t) 0,
				std::min(relEndIndex, m_FadeOutInfo.size() - 1));

		std::vector<double>::const_iterator startIt = m_FadeOutInfo.begin() + relStartIndex;
		std::vector<double>::const_iterator endIt = m_FadeOutInfo.begin() + relEndIndex;
		for (std::vector<double>::const_iterator it = startIt;
				it != endIt; ++it)
		{
			timeChange += 60.0 / *it;
		}

		double endBPM = m_FadeOutInfo.at(clipEndIndex);
		timeChange += 60.0 * (dEndIndex - endIndex) / endBPM;
	}
	return timeChange;
}

double DJCrossfadeCalculator::ComputeOriginalTimeChangeForFadeInTrack(double percent, double percentChange)
{
	double timeChange = 0.0;
	double dStartIndex = percent * GetMaxBeatgridSize();
	size_t startIndex = (size_t) dStartIndex;
	double dEndIndex = (percent + percentChange) * GetMaxBeatgridSize();
	size_t endIndex = (size_t) dEndIndex;
	size_t minStart = GetFadeInBeatgridOffset();
	if (startIndex < minStart && endIndex >= minStart)
	{
		// minStart is bracketed between startIndex and endIndex, so we
		// should include it and start from there
		startIndex = minStart;

		double startBPM = m_FadeInInfo.at(startIndex);
		timeChange -= 60.0 * (dStartIndex - startIndex) / startBPM;
	}
	if (startIndex >= minStart && endIndex >= startIndex)
	{
		size_t relStartIndex = startIndex - minStart;
		size_t relEndIndex = endIndex - minStart;
		size_t clipEndIndex = std::max((size_t) 0,
				std::min(relEndIndex, m_FadeInInfo.size() - 1));

		std::vector<double>::const_iterator startIt = m_FadeInInfo.begin() + relStartIndex;
		std::vector<double>::const_iterator endIt = m_FadeInInfo.begin() + relEndIndex;
		for (std::vector<double>::const_iterator it = startIt;
				it != endIt; ++it)
		{
			timeChange += 60.0 / *it;
		}

		double endBPM = m_FadeInInfo.at(clipEndIndex);
		timeChange += 60.0 * (dEndIndex - endIndex) / endBPM;
	}
	return timeChange;
}

double DJCrossfadeCalculator::ComputeVolumeForFadeInTrack(double percent) const
{
	// To find the volume of the fade-out track, take 1 minus the output of
	// this function.
	double volume = 0.0;
	double dIndex = percent * GetMaxBeatgridSize();
	size_t index = (size_t) dIndex; // NB:  This is the global index, not the beatgrid-specific index
	size_t start = GetFadeInBeatgridOffset();
	size_t minSize = GetMinBeatgridSize();
	if (index >= start + minSize)
	{
		volume = 1.0;
	}
	else if (index >= start)
	{
		volume = (dIndex - start) / ((double) minSize);
	}
	return volume;
}

double DJCrossfadeCalculator::ComputeStretchFactorForFadeOutTrack(double percent) const
{
	double factor = 1.0;
	double dIndex = percent * GetMaxBeatgridSize();
	size_t index = (size_t) dIndex; // NB:  This is the global index, not the beatgrid-specific index
	size_t start = GetFadeOutBeatgridOffset();
	if (index >= start && index < start + m_FadeOutInfo.size())
	{
		double alpha = dIndex - (double) index;
		double BPM1 = m_FadeOutInfo.at(index);
		double BPM2 = index + 1 >= m_FadeOutInfo.size()
				? m_InstantaneousFadeOutTempo : m_FadeOutInfo.at(index + 1);
		double interpBPM = (1.0 - alpha) * BPM1 + alpha * BPM2;
		factor = interpBPM / InterpolateBPMAtCrossfade(percent);
	}
	return factor;
}

double DJCrossfadeCalculator::ComputeStretchFactorForFadeInTrack(double percent) const
{
	double factor = 1.0;
	double dIndex = percent * GetMaxBeatgridSize();
	size_t index = (size_t) dIndex; // NB:  This is the global index, not the beatgrid-specific index
	size_t start = GetFadeInBeatgridOffset();
	if (index >= start && index < start + m_FadeInInfo.size())
	{
		double alpha = dIndex - (double) index;
		index -= start;
		double BPM1 = m_FadeInInfo.at(index);
		double BPM2 = index + 1 >= m_FadeInInfo.size()
				? m_InstantaneousFadeInTempo : m_FadeInInfo.at(index + 1);
		double interpBPM = (1.0 - alpha) * BPM1 + alpha * BPM2;
		factor = interpBPM / InterpolateBPMAtCrossfade(percent);
	}
	return factor;
}

double DJCrossfadeCalculator::GetFadeOutPercentage() const
{
	size_t maxBGSize = GetMaxBeatgridSize();
	return maxBGSize == 0 ? 0.0 : GetFadeOutBeatgridOffset() / ((double) maxBGSize);
}

double DJCrossfadeCalculator::GetFadeInPercentage() const
{
	size_t maxBGSize = GetMaxBeatgridSize();
	return maxBGSize == 0 ? 0.0 : GetFadeInBeatgridOffset() / ((double) maxBGSize);
}

bool DJCrossfadeCalculator::ReadBPMFiles()
{
	bool rv = false;

	std::ifstream ifs(m_AudioFile1->getFilename() + m_bpmInfoFileExt);
	if (ifs.good())
	{
		rv = ReadFadeInfo(ifs, true, m_FadeOutTime,
				m_InstantaneousFadeOutTempo, m_FadeOutInfo);
		ifs.close();
	}

	if (rv)
	{
		ifs.open(m_AudioFile2->getFilename() + m_bpmInfoFileExt);
		if ((rv = ifs.good()))
		{
			rv = ReadFadeInfo(ifs, false, m_FadeInTime,
					m_InstantaneousFadeInTempo, m_FadeInInfo);
			ifs.close();
		}
	}

	return rv;
}

double DJCrossfadeCalculator::GetTimeAtStartOfFadeOut() const
{
	return m_FadeOutTime;
}

double DJCrossfadeCalculator::GetTimeAtStartOfFadeIn() const
{
	return m_FadeInTime;
}

bool DJCrossfadeCalculator::CheckCrossfadeCondition()
{
	bool rv = false;

	if (ReadBPMFiles())
	{
		// If we successfully read files, both criteria must be satisfied:
		// 1. The endpoints of the fades for both files do not exceed the EOF
		// 2. RubberBand constraints are satisfied.
		rv = GetTimeAtEndOfFadeOut() < m_AudioFile1->getDuration() &&
		     GetTimeAtEndOfFadeIn() < m_AudioFile2->getDuration() &&
		     VerifyRubberBandConstraints();
	}

	return rv;
}
