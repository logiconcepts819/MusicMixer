#ifndef SRC_CORE_XFADE_DJCROSSFADECALCULATOROLD_H_
#define SRC_CORE_XFADE_DJCROSSFADECALCULATOROLD_H_

#include "CrossfadeCalculator.h"
#include <fstream>
#include <vector>

class AudioFile;

// The technology here is similar to what is implemented in Mixmeister,
// but this is even better because the beatgrid can vary in BPM in this
// implementation.

class DJCrossfadeCalculatorOld: public CrossfadeCalculator {
	static const std::string m_bpmInfoFileExt;

	double m_FadeOutTime;
	double m_InstantaneousFadeOutTempo;
	std::vector<double> m_FadeOutInfo;

	double m_FadeInTime;
	double m_InstantaneousFadeInTempo;
	std::vector<double> m_FadeInInfo;

	bool m_UseOptimisticTempoAdaptation;

	static bool ReadBeatgrid(std::ifstream & ifs, double & instantaneousFadeTempo, std::vector<double> & fadeInfo);
	static bool ReadFadeInfo(std::ifstream & ifs, bool fadeOut, double & fadeTime, double & instantaneousFadeTempo, std::vector<double> & fadeInfo);

	bool VerifyRubberBandConstraintsForSingleGrid(const std::vector<double> & fadeInfo, size_t (*bgOffsetFunc)(const DJCrossfadeCalculatorOld &)) const;
	bool VerifyRubberBandConstraints() const;

	double GetBPMAtStartOfCrossfade() const;
	double GetBPMAtEndOfCrossfade() const;
	size_t GetFadeOutBeatgridOffset() const;
	size_t GetFadeInBeatgridOffset() const;
	size_t GetMinBeatgridSize() const;
	size_t GetMaxBeatgridSize() const;

	// Note:  these routines assume absolute time, not stretched time!
	double GetTimeAtFadeOutBeat(size_t beatPos) const;
	double GetTimeAtEndOfFadeOut() const;
	double GetTimeAtFadeInBeat(size_t beatPos) const;
	double GetTimeAtEndOfFadeIn() const;

	bool ReadBPMFiles();
	double InterpolateBPMAtCrossfade(double percent) const;

public:
	DJCrossfadeCalculatorOld(const AudioFile & audioFile1, const AudioFile & audioFile2);
	virtual ~DJCrossfadeCalculatorOld();

	double GetTimeAtStartOfFadeOut() const;
	double GetTimeAtStartOfFadeIn() const;
	double ComputePercentageChangeForFadeOutTrack(double percent, double time, double timeChange, double & newTime, bool & done);
	double ComputePercentageChangeForFadeInTrack(double percent, double time, double timeChange, double & newTime, bool & done);
	double ComputeOriginalTimeChangeForFadeOutTrack(double percent, double percentChange);
	double ComputeOriginalTimeChangeForFadeInTrack(double percent, double percentChange);
	double ComputeVolumeForFadeInTrack(double percent) const;
	double ComputeStretchFactorForFadeOutTrack(double percent) const;
	double ComputeStretchFactorForFadeInTrack(double percent) const;
	double GetFadeOutPercentage() const;
	double GetFadeInPercentage() const;

	bool isUsingOptimisticTempoAdaptation() const
	{
		return m_UseOptimisticTempoAdaptation;
	}

	void setUsingOptimisticTempoAdaptation(bool enable)
	{
		m_UseOptimisticTempoAdaptation = enable;
	}

	bool IsDJCalculator() const
	{
		return true;
	}

	bool UseRubberband() const
	{
		return true;
	}

	bool CheckCrossfadeCondition();
};

#endif /* SRC_CORE_XFADE_DJCROSSFADECALCULATOROLD_H_ */
