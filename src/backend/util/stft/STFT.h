#ifndef SRC_UTIL_STFT_STFT_H_
#define SRC_UTIL_STFT_STFT_H_

#include "../minfft.h"
#include "SpectrumProcessor.h"
#include <memory>
#include <deque>
#include <vector>
#include <string>

// A class that implements the short-time Fourier transform (STFT).
class STFT
{
	SpectrumProcessor * m_SpecProcessor;

	size_t m_FrameSize;
	size_t m_OsampFactor;

	fft_plan_t m_InFFTPlan;
	std::vector<float> m_InFFTData;  // FFT length
	std::vector<float> m_InFFTCache; // FFT length * (7/4)
	std::vector<float> m_InFIFO;     // FFT length

	struct OutputInfo
	{
		fft_plan_t m_OutFFTPlan;
		std::vector<float> m_OutFFTData;  // FFT length
		std::vector<float> m_OutFFTCache; // FFT length * (7/4)
		std::vector<float> m_OutFIFO;     // FFT length
		std::vector<float> m_OutAccum;    // FFT length
		std::deque<float> m_Output;       // variable length
	};
	std::vector<OutputInfo> m_OutInfo; // Number of outputs
	size_t m_OutSize;

	std::vector<float> m_Window; // FFT length
	size_t m_FIFOPos;

	size_t m_StepSize;
	size_t m_FFTLatency;

	size_t m_HeadLatencyConsumed;
	size_t m_TailLatencyConsumed;
	size_t m_TotalLatency;

public:
	STFT();
	virtual ~STFT();

	size_t GetStepSize() const
	{
		return m_StepSize;
	}

	size_t GetLatency() const
	{
		return m_FFTLatency;
	}

	size_t GetTotalLatency() const
	{
		return m_TotalLatency;
	}

	void Initialize(SpectrumProcessor * processor, size_t osampFactor = 4);
	void SubmitSample(float sample);

	bool DiscardInitialOutputAndCheck(size_t maxNumSamples = std::string::npos);
	std::vector<std::vector<float> > GatherOutput(size_t maxNumSamples = std::string::npos);
	std::vector<std::vector<float> > GatherFinalOutput(size_t maxNumSamples = std::string::npos);
	bool OutputFinalized() const;
};

#endif /* SRC_UTIL_STFT_STFT_H_ */
