#include "SpectrumProcessor.h"
#include <cassert>

SpectrumProcessor::SpectrumProcessor(size_t fftSize, size_t numOutputs)
{
	Initialize(fftSize, numOutputs);
}

SpectrumProcessor::~SpectrumProcessor()
{
}

void SpectrumProcessor::Initialize(size_t fftSize, size_t numOutputs)
{
	assert(numOutputs != 0);
	assert(fftSize != 0 && (fftSize & (fftSize - 1)) == 0);

	m_OutputSpectrum.clear();
	m_OutputSpectrum.resize(numOutputs);
	for (std::vector<std::vector<float> >::iterator
			it = m_OutputSpectrum.begin(); it != m_OutputSpectrum.end(); ++it)
	{
		it->resize(fftSize);
	}
}

size_t SpectrumProcessor::GetNumMagnitudes(size_t fftSize)
{
	return (fftSize >> 1) + 1;
}

float SpectrumProcessor::GetMagnitudeSquared(const std::vector<float> & spectrum, size_t index)
{
	float magSquared;
	if (index == 0 || (index << 1) == spectrum.size())
	{
		magSquared = spectrum.at(index);
		magSquared *= magSquared;
	}
	else
	{
		float re = spectrum.at(index);
		float im = spectrum.at(spectrum.size() - index);
		magSquared = re * re + im * im;
	}
	return magSquared;
}

void SpectrumProcessor::ScaleMagnitude(std::vector<float> & spectrum, size_t index, float factor)
{
	spectrum.at(index) *= factor;
	if (index != 0 && (index << 1) != spectrum.size())
	{
		spectrum.at(spectrum.size() - index) *= factor;
	}
}
