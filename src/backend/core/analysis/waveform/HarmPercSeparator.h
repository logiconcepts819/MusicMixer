#ifndef SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCSEPARATOR_H_
#define SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCSEPARATOR_H_

#include "../../../util/minfft.h"
#include <vector>
#include <deque>
#include <string>
#include <utility>

class HarmPercSeparator
{
	std::vector<float> m_AudioData;

	typedef std::pair<std::vector<float>, size_t> DataOverlapPair;
	std::deque<DataOverlapPair> m_AudioFrames;
	std::vector<std::deque<float> > m_HarmonicState;
	std::deque<std::vector<float> > m_PercussiveState;
	std::vector<float> m_OutputState;

	size_t m_ImaginarySize;

	bool m_EOS;
	fft_plan_t m_FFTPlan;

	static const size_t m_DefaultDelta;
	static const size_t m_DefaultFrameSize;
	static const size_t m_DefaultTimeChunksPerSTFT;
	static const size_t m_DefaultMedianFilterSize;

	size_t m_Delta;
	size_t m_FrameSize;
	size_t m_TimeChunksPerSTFT;
	size_t m_MedianFilterSize;
	size_t m_PrevDiscardAmount;
	bool m_FirstRun;

	std::vector<float> GetChunk(size_t & overlapAmount);
	float GetWindowFactor(size_t k, size_t N);
	void ApplyWindow(std::vector<float> & data);

	std::vector<std::vector<float> > GetHarmonicSpectrogram(const std::vector<float*> & spectrogram, size_t analysisSize);
	std::vector<std::vector<float> > GetPercussiveSpectrogram(const std::vector<float*> & spectrogram, size_t analysisSize);
public:
	HarmPercSeparator();
	virtual ~HarmPercSeparator();
	float _GetMagnitudeSqr(const float * spectrum, size_t nyquistIndex, size_t index);

	size_t GetDelta() const
	{
		return m_Delta;
	}

	void SetDelta(size_t delta)
	{
		m_Delta = delta;
	}

	size_t GetFrameSize() const
	{
		return m_FrameSize;
	}

	void SetFrameSize(size_t frameSize)
	{
		m_FrameSize = frameSize;
	}

	size_t GetTimeChunksPerSTFT() const
	{
		return m_TimeChunksPerSTFT;
	}

	void SetTimeChunksPerSTFT(size_t timeChunksPerSTFT)
	{
		m_TimeChunksPerSTFT = timeChunksPerSTFT;
	}

	size_t GetMedianFilterSize() const
	{
		return m_MedianFilterSize;
	}

	void SetMedianFilterSize(size_t medianFilterSize)
	{
		m_MedianFilterSize = medianFilterSize;
	}

	void SubmitData(const float * data, size_t size, bool eos = false);
	void GetSeparatedSignals(std::vector<float> & harmonicContent, std::vector<float> & percussiveContent);
};

#endif /* SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCSEPARATOR_H_ */
