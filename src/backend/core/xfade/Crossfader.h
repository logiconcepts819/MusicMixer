#ifndef SRC_CORE_XFADE_CROSSFADER_H_
#define SRC_CORE_XFADE_CROSSFADER_H_

#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <rubberband/RubberBandStretcher.h>

class AudioFile;
class AudioStretchInfo;
class AudioStretcher;
class AudioBlock;
class CrossfadeCalculator;
class FadeMap;

class Crossfader {
	static const size_t m_DefaultXfadeBufferSize;

	std::unique_ptr<AudioStretcher> m_Stretcher1;
	std::unique_ptr<AudioStretcher> m_Stretcher2;

	std::unique_ptr<CrossfadeCalculator> xfadeCalc;
	std::vector<char> m_StretchBuf;
	size_t m_StretchBufReadPos;

	AudioFile * m_File1;
	AudioFile * m_File2;
	FadeMap * m_FadeMap;

	bool m_Initialized;
	bool m_Ineligible;
	bool m_AllowDJCrossfade;
	bool m_AllowCrossfade;

	double m_CrossfadeTime;
	bool m_UseOptimisticTempoAdaptation;

	size_t m_SampleCounter;
	size_t m_XfadeBufferSize;

	std::mutex m_File2HasCompRefPosLock;
	std::condition_variable m_File2HasCompRefPosCond;
	bool m_File2HasCompRefPos;
	bool m_File2HasCompRefPosAvailable;

	std::mutex m_ExtraEndInfoMutex;
	std::condition_variable m_ExtraEndInfoCond;
	double m_PercentPadding;
	bool m_ExtraEndInfoAvailable;

	std::mutex m_ExtraEndBlockMutex;
	std::condition_variable m_ExtraEndBlockCond;
	std::shared_ptr<AudioBlock> m_ExtraEndBlock;

	std::shared_ptr<AudioBlock> m_XfadeLeftover;
	std::unique_ptr<std::thread> m_XfadeThread1;
	std::unique_ptr<std::thread> m_XfadeThread2;
	std::unique_ptr<std::thread> m_PlaybackThread;

	void SubmitStretchInfoAndReset(bool fadeOut, std::shared_ptr<AudioStretchInfo> & stretchInfo);
	std::shared_ptr<AudioBlock> ChunkAndSendToStretcher(std::unique_ptr<AudioStretcher> & stretcher, AudioBlock * block = nullptr);

	static void PlaybackThread(Crossfader * xfader);
	static void LaunchCrossfade1Thread(Crossfader * xfader, std::shared_ptr<AudioBlock> startingFadeOutBlock);
	static void LaunchCrossfade2Thread(Crossfader * xfader);

	void PlaybackCrossfadeMix();
public:
	Crossfader(AudioFile & file1, AudioFile & file2, FadeMap & fadeMap);
	virtual ~Crossfader();

	void InitializeCrossfade();
	bool ReadyToCrossfade(double & backDelta) const;

	bool isAllowingDJCrossfade() const
	{
		return m_AllowDJCrossfade;
	}

	void setAllowingDJCrossfade(bool enable)
	{
		m_AllowDJCrossfade = enable;
	}

	bool isAllowingCrossfade() const
	{
		return m_AllowCrossfade;
	}

	void setAllowingCrossfade(bool enable)
	{
		m_AllowCrossfade = enable;
	}

	bool isEligible() const
	{
		return !m_Ineligible;
	}

	double getCrossfadeTime() const
	{
		return m_CrossfadeTime;
	}

	void setCrossfadeTime(double crossfadeTime);

	bool isUsingOptimisticTempoAdaptation() const
	{
		return m_UseOptimisticTempoAdaptation;
	}

	void setUsingOptimisticTempoAdaptation(bool enable);

	FadeMap & GetFadeMap();
	const FadeMap & GetFadeMap() const;
	void SetFadeMap(FadeMap & fadeMap);

	void StartCrossfade(std::shared_ptr<AudioBlock> & startingFadeOutBlock);
	void WaitOnThreadsAndGiveXfadeLeftover(std::shared_ptr<AudioBlock> & leftover);
};

#endif /* SRC_CORE_XFADE_CROSSFADER_H_ */
