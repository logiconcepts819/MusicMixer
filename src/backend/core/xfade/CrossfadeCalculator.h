#ifndef SRC_CORE_XFADE_CROSSFADECALCULATOR_H_
#define SRC_CORE_XFADE_CROSSFADECALCULATOR_H_

class AudioBlock;
class AudioFile;
class DJCrossfadeCalculatorOld;

class CrossfadeCalculator {
protected:
	const AudioFile * m_AudioFile1;
	const AudioFile * m_AudioFile2;

	double m_CrossfadeTime; // crossfade time in seconds
	static const double m_Epsilon;
public:
	CrossfadeCalculator(const AudioFile & audioFile1, const AudioFile & audioFile2);
	virtual ~CrossfadeCalculator();

	static double GetEpsilon();

	//const AudioFile & GetAudioFile1() const { return *m_AudioFile1; }
	//const AudioFile & GetAudioFile2() const { return *m_AudioFile2; }

	AudioFile & GetAudioFile1() const
		{ return *const_cast<AudioFile*>(m_AudioFile1); }
	AudioFile & GetAudioFile2() const
		{ return *const_cast<AudioFile*>(m_AudioFile2); }

	virtual double GetTimeAtStartOfFadeOut() const;
	virtual double GetTimeAtStartOfFadeIn() const;
	virtual double ComputePercentageChangeForFadeOutTrack(double percent, double time, double timeChange, double & newTime, bool & done);
	virtual double ComputePercentageChangeForFadeInTrack(double percent, double time, double timeChange, double & newTime, bool & done);
	double ComputePercentageChangeForTrack(bool fadeOut, double percent, double time, double timeChange, double & newTime, bool & done);
	virtual double ComputeOriginalTimeChangeForFadeOutTrack(double percent, double percentChange);
	virtual double ComputeOriginalTimeChangeForFadeInTrack(double percent, double percentChange);
	virtual double ComputeVolumeForFadeOutTrack(double percent) const;
	virtual double ComputeVolumeForFadeInTrack(double percent) const;
	double ComputeVolumeForTrack(bool fadeOut, double percent) const;
	virtual double ComputeStretchFactorForFadeOutTrack(double percent) const;
	virtual double ComputeStretchFactorForFadeInTrack(double percent) const;
	double ComputeStretchFactorForTrack(bool fadeOut, double percent) const;
	virtual double GetFadeOutPercentage() const;
	virtual double GetFadeInPercentage() const;
	double ComputeOriginalTimeChangeForTrack(bool fadeOut, double percent, double percentChange);

	virtual bool CheckCrossfadeCondition();

	virtual bool IsDJCalculator() const
	{
		return false;
	}

	virtual bool UseRubberband() const
	{
		return false;
	}

	double getCrossfadeTime() const
	{
		return m_CrossfadeTime;
	}

	void setCrossfadeTime(double crossfadeTime)
	{
		m_CrossfadeTime = crossfadeTime;
	}

	DJCrossfadeCalculatorOld * AsDJCalculator();
};

#endif /* SRC_CORE_XFADE_CROSSFADECALCULATOR_H_ */
