#include "HarmPercProcessor.h"
#include <cmath>
#include <cassert>
#include <algorithm>

// We need one output for harmonic information and another output for
// percussive information
HarmPercProcessor::HarmPercProcessor(size_t fftSize, size_t medFilterSize, bool useHardMask, float softMaskPower)
: SpectrumProcessor(fftSize, 2)
{
	Initialize(fftSize, medFilterSize, useHardMask, softMaskPower);
}

HarmPercProcessor::~HarmPercProcessor()
{
}

void HarmPercProcessor::Initialize(size_t fftSize, size_t medFilterSize, bool useHardMask, float softMaskPower)
{
	// Constraints
	assert(medFilterSize > 1);
	assert((medFilterSize & 1) == 1);
	assert(m_Threshold <= m_NumMagnitudes);

	m_UseHardMask = useHardMask;
	m_SoftMaskPower = softMaskPower;
	m_MedianFilterSize = medFilterSize;
	m_NumMagnitudes = GetNumMagnitudes(fftSize);
	m_Threshold = (medFilterSize >> 1) + 1;
	m_TemporalHistory.resize(m_NumMagnitudes);
	for (std::vector<std::deque<float> >::iterator
			it = m_TemporalHistory.begin(); it != m_TemporalHistory.end();
			++it)
	{
		it->resize(medFilterSize);
	}
}

float HarmPercProcessor::GetMedianFromQueue(const std::deque<float> & queue)
{
	std::vector<float> sortedArray(queue.begin(), queue.end());
	std::sort(sortedArray.begin(), sortedArray.end());
	return sortedArray.at(sortedArray.size() >> 1);
}

void HarmPercProcessor::GetScales(float Hsq, float Psq, float & MH, float & MP)
{
	if (m_UseHardMask)
	{
		// A special case of the soft mask, where Hsq and Psq are raised to
		// infinity
		MH = Hsq > Psq ? 1.0f : Hsq < Psq ? 0.0f : 0.5f;
		MP = Hsq > Psq ? 0.0f : Hsq < Psq ? 1.0f : 0.5f;
	}
	else
	{
		float Hpow = powf(Hsq, 0.5f * m_SoftMaskPower);
		float Ppow = powf(Psq, 0.5f * m_SoftMaskPower);
		float denom = Hpow + Ppow;
		MH = Hpow / denom;
		MP = Ppow / denom;
	}
}

void HarmPercProcessor::Process(const std::vector<float> & inputSpectrum)
{
	// Time-delay:  (STFT step size) * floor((Median filter size) / 2)
	size_t k = 0;
	for (auto it = m_TemporalHistory.begin(); it != m_TemporalHistory.end();
			++it, ++k)
	{
		it->pop_front();
		it->push_back(GetMagnitudeSquared(inputSpectrum, k));
	}
	m_Spectrogram.push_back(inputSpectrum);
	if (m_Spectrogram.size() >= m_Threshold)
	{
		std::vector<float> & targetSpectrum = m_Spectrogram.front();
		for (k = 0; k != GetFFTSize(); ++k)
		{
			GetOutputSpectrum(0).at(k) = targetSpectrum.at(k);
			GetOutputSpectrum(1).at(k) = targetSpectrum.at(k);
		}

		std::deque<float> spectralHistory;
		spectralHistory.resize(m_MedianFilterSize);

		size_t idx = 0;
		for (k = 0; k != m_NumMagnitudes; ++k)
		{
			spectralHistory.pop_front();
			spectralHistory.push_back(GetMagnitudeSquared(targetSpectrum, k));
			if (k >= m_Threshold)
			{
				idx = k - m_Threshold;
				float Hsq = GetMedianFromQueue(m_TemporalHistory.at(idx));
				float Psq = GetMedianFromQueue(spectralHistory);
				float MH, MP;
				GetScales(Hsq, Psq, MH, MP);
				ScaleMagnitude(GetOutputSpectrum(0), idx, MH);
				ScaleMagnitude(GetOutputSpectrum(1), idx, MP);
			}
		}
		for (k = 0; k != m_Threshold; ++k, ++idx)
		{
			spectralHistory.pop_front();
			spectralHistory.push_back(0.0f);
			float Hsq = GetMedianFromQueue(m_TemporalHistory.at(idx));
			float Psq = GetMedianFromQueue(spectralHistory);
			float MH, MP;
			GetScales(Hsq, Psq, MH, MP);
			ScaleMagnitude(GetOutputSpectrum(0), idx, MH);
			ScaleMagnitude(GetOutputSpectrum(1), idx, MP);
		}

		m_Spectrogram.pop_front();
	}
}
