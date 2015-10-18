#ifndef SRC_UTIL_STFT_SPECTRUMPROCESSOR_H_
#define SRC_UTIL_STFT_SPECTRUMPROCESSOR_H_

#include <vector>
#include <cstring>

class SpectrumProcessor
{
	std::vector<std::vector<float> > m_OutputSpectrum;

public:
	SpectrumProcessor(size_t fftSize = 1024, size_t numOutputs = 1);
	virtual ~SpectrumProcessor();

	void Initialize(size_t fftSize = 1024, size_t numOutputs = 1);

	size_t GetNumOutputs() const
	{
		return m_OutputSpectrum.size();
	}

	size_t GetFFTSize() const
	{
		return m_OutputSpectrum.at(0).size();
	}

	const std::vector<float> & GetOutputSpectrum(size_t outputIndex) const
	{
		return m_OutputSpectrum.at(outputIndex);
	}

	std::vector<float> & GetOutputSpectrum(size_t outputIndex)
	{
		return m_OutputSpectrum.at(outputIndex);
	}

	static size_t GetNumMagnitudes(size_t fftSize);
	static float GetMagnitudeSquared(const std::vector<float> & spectrum, size_t index);
	static void ScaleMagnitude(std::vector<float> & spectrum, size_t index, float factor);

	virtual void Process(const std::vector<float> & inputSpectrum) = 0;
	virtual size_t GetProcessDelayPerStep() const
	{
		return 0;
	}
};

#endif /* SRC_UTIL_STFT_SPECTRUMPROCESSOR_H_ */
