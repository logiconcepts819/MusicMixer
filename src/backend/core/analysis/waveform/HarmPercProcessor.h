#ifndef SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCPROCESSOR_H_
#define SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCPROCESSOR_H_

#include "../../../util/stft/SpectrumProcessor.h"
#include <deque>
#include <vector>

class HarmPercProcessor: public SpectrumProcessor
{
	bool m_UseHardMask;
	float m_SoftMaskPower;
	size_t m_MedianFilterSize;
	size_t m_NumMagnitudes;
	size_t m_Threshold;
	std::vector<std::deque<float> > m_TemporalHistory; // magnitudes only
	std::deque<std::vector<float> > m_Spectrogram;     // mags AND phases

	float GetMedianFromQueue(const std::deque<float> & queue);
	void GetScales(float Hsq, float Psq, float & MH, float & MP);

public:
	HarmPercProcessor(size_t fftSize = 1024, size_t medFilterSize = 16, bool useHardMask = true, float softMaskPower = 1.0f);
	virtual ~HarmPercProcessor();

	void Initialize(size_t fftSize = 1024, size_t medFilterSize = 16, bool useHardMask = true, float softMaskPower = 1.0f);

	void Process(const std::vector<float> & inputSpectrum);
	size_t GetProcessDelayPerStep() const
	{
		return m_Threshold - 1;
	}
};

#endif /* SRC_CORE_ANALYSIS_WAVEFORM_HARMPERCPROCESSOR_H_ */
