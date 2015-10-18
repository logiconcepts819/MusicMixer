#include "HarmPercSeparator.h"
#include "../../../util/MathConstants.h"
#include <cmath>
#include <algorithm>

const size_t HarmPercSeparator::m_DefaultDelta = 64;
const size_t HarmPercSeparator::m_DefaultFrameSize = 1024;
const size_t HarmPercSeparator::m_DefaultTimeChunksPerSTFT = 128;
const size_t HarmPercSeparator::m_DefaultMedianFilterSize = 16;

HarmPercSeparator::HarmPercSeparator()
: m_ImaginarySize(0), m_EOS(false), m_Delta(m_DefaultDelta),
  m_FrameSize(m_DefaultFrameSize),
  m_TimeChunksPerSTFT(m_DefaultTimeChunksPerSTFT),
  m_MedianFilterSize(m_DefaultMedianFilterSize), m_PrevDiscardAmount(0),
  m_FirstRun(true)
{
	// TODO Auto-generated constructor stub
	m_HarmonicState.resize((m_FrameSize >> 1) + 1);
}

HarmPercSeparator::~HarmPercSeparator() {
	// TODO Auto-generated destructor stub
}

std::vector<float> HarmPercSeparator::GetChunk(size_t & overlapAmount)
{
	std::vector<float> chunk;

	// Uses overlap-save method
	if (m_EOS || m_AudioData.size() >= m_FrameSize)
	{
		size_t endIdx = std::min(m_AudioData.size(), m_FrameSize);
		chunk.insert(chunk.end(), m_AudioData.begin(),
				m_AudioData.begin() + endIdx);
		chunk.resize(m_FrameSize);

		size_t clipIdx = std::min(m_AudioData.size(), m_Delta);
		overlapAmount = m_FrameSize - clipIdx;
		m_AudioData.erase(m_AudioData.begin(), m_AudioData.begin() + clipIdx);
	}

	return chunk;
}

float HarmPercSeparator::GetWindowFactor(size_t k, size_t N)
{
	return 0.5f - 0.5f * cosf(2.0f * MathConstants::Pi * k / (N - 1));
}

void HarmPercSeparator::ApplyWindow(std::vector<float> & data)
{
	// Applies a Hann window to the data
	size_t N = data.size();
	size_t k = 0;
	for (std::vector<float>::iterator it = data.begin(); it != data.end(); ++it)
	{
		*it *= GetWindowFactor(k++, N);
	}
}

float HarmPercSeparator::_GetMagnitudeSqr(const float * spectrum, size_t nyquistIndex, size_t index)
{
	float sample;
	if (index == 0 || index == nyquistIndex)
	{
		sample = spectrum[index];
		sample *= sample;
	}
	else
	{
		float re = spectrum[index];
		float im = spectrum[m_FrameSize - index];
		sample = re*re + im*im;
	}
	return sample;
}

std::vector<std::vector<float> > HarmPercSeparator::GetHarmonicSpectrogram(const std::vector<float*> & spectrogram, size_t analysisSize)
{
	std::vector<std::vector<float> > newSpectrogram;
	std::deque<float> samples;
	size_t fftSize = m_FrameSize >> 1;

	size_t idx1 = m_MedianFilterSize >> 1;
	size_t idx2 = (m_MedianFilterSize + 1) >> 1;

	size_t initPadding = 0;
	if (m_FirstRun)
	{
		initPadding = idx2 - 1;
	}

	size_t analysisSize2 = analysisSize;
	if (m_EOS)
	{
		analysisSize2 += m_MedianFilterSize - (idx2 - 1);
	}

	for (size_t m = 0; m <= fftSize; ++m)
	{
		std::deque<float> & samples = m_HarmonicState.at(m);
		if (m_FirstRun)
		{
			samples.resize(m_MedianFilterSize);
		}

		for (size_t k = 0; k < analysisSize2; ++k)
		{
			if (m == 0 && k >= initPadding)
			{
				std::vector<float> newSpectrum;
				newSpectrum.resize(fftSize);
				newSpectrogram.push_back(std::move(newSpectrum));
			}

			float sample = k >= analysisSize
					? 0.0f : _GetMagnitudeSqr(spectrogram.at(k), fftSize, m);
			samples.pop_front();
			samples.push_back(sample);

			if (k >= initPadding)
			{
				std::vector<float> sortedSamples(samples.begin(), samples.end());
				std::sort(sortedSamples.begin(), sortedSamples.end());

				float median = 0.5f * (sortedSamples.at(idx1) + sortedSamples.at(idx2));
				newSpectrogram.at(k - initPadding).at(m) = median;
			}
		}
	}

	return newSpectrogram;
}

template <typename F>
void StorePercussiveInfo(HarmPercSeparator & separator, float * spectrum, size_t index, size_t nyquistIndex, F routine)
{
	size_t idx1 = separator.GetMedianFilterSize() >> 1;
	size_t idx2 = (separator.GetMedianFilterSize() + 1) >> 1;

	size_t lb = 0;
	size_t ub = nyquistIndex - 1;

	std::vector<float> samples;
	if (index < idx1)
	{
		samples.resize(idx1 - index);
	}
	else
	{
		lb = index - idx1;
	}
	if (index + idx2 < nyquistIndex)
	{
		ub = index + idx2;
	}
	for (size_t k = lb; k <= ub; ++k)
	{
		float sample = separator._GetMagnitudeSqr(spectrum, nyquistIndex, index);
		samples.push_back(sample);
	}
	samples.resize(separator.GetMedianFilterSize());
	std::sort(samples.begin(), samples.end());

	float median = 0.5f * (samples.at(idx1) + samples.at(idx2));
	routine(median);
}

std::vector<std::vector<float> > HarmPercSeparator::GetPercussiveSpectrogram(const std::vector<float*> & spectrogram, size_t analysisSize)
{
	std::vector<std::vector<float> > newSpectrogram;
	std::vector<float> samples;
	samples.resize(m_MedianFilterSize);

	size_t fftSize = m_FrameSize >> 1;

	//size_t idx1 = m_MedianFilterSize >> 1;
	size_t idx2 = (m_MedianFilterSize + 1) >> 1;

	size_t analysisSize2 = analysisSize;
	if (!m_EOS)
	{
		analysisSize2 -= (idx2 - 1);
	}

	newSpectrogram.insert(newSpectrogram.end(),
			m_PercussiveState.begin(), m_PercussiveState.end());
	m_PercussiveState.clear();

	newSpectrogram.resize(analysisSize2);
	for (size_t k = 0; k < analysisSize2; ++k)
	{
		newSpectrogram.at(k).resize(fftSize + 1);
		for (size_t m = 0; m <= fftSize; ++m)
		{
			StorePercussiveInfo(*this, spectrogram.at(k), m, fftSize,
					[&newSpectrogram, k, m](float median) {
						newSpectrogram.at(k).at(m) = median;
					});
		}
	}

	for (size_t k = analysisSize2; k < analysisSize; ++k)
	{
		std::vector<float> newSpectrum;
		newSpectrum.resize(fftSize + 1);
		for (size_t m = 0; m <= fftSize; ++m)
		{
			StorePercussiveInfo(*this, spectrogram.at(k), m, fftSize,
					[&newSpectrum, k, m](float median) {
						newSpectrum.at(m) = median;
					});
		}
		m_PercussiveState.push_back(std::move(newSpectrum));
	}

	return newSpectrogram;
}

void HarmPercSeparator::SubmitData(const float * data, size_t size, bool eos)
{
	m_EOS = eos;
	m_AudioData.insert(m_AudioData.end(), data, data + size);
}

void HarmPercSeparator::GetSeparatedSignals(std::vector<float> & harmonicContent, std::vector<float> & percussiveContent)
{
	bool done = false;
	size_t halfSizeCeil = (m_MedianFilterSize + 1) >> 1;
	size_t extraSize = halfSizeCeil - 1;
	size_t desiredSize = m_TimeChunksPerSTFT + extraSize;
	size_t analysisSize = 0;

	while (!done && m_ImaginarySize < desiredSize)
	{
		size_t overlapAmount;
		std::vector<float> newFrame = GetChunk(overlapAmount);
		done = newFrame.empty();
		if (!done)
		{
			m_AudioFrames.push_back(
					DataOverlapPair(std::move(newFrame), overlapAmount));
			++m_ImaginarySize;
		}
	}

	harmonicContent.clear();
	percussiveContent.clear();
	if (m_ImaginarySize == desiredSize || (m_EOS && !m_AudioFrames.empty()))
	{
		// Ready to separate the signals
		analysisSize = m_EOS ? m_AudioFrames.size() : m_TimeChunksPerSTFT;

		// Setup
		std::vector<float*> spectrogram;
		std::vector<float> cache;
		cache.resize(plan_cache_size(m_FrameSize));

		// Take FFT of each time frame.  This is our discrete STFT
		fft_plan_t fftPlan;
		for (std::deque<DataOverlapPair>::iterator
				it = m_AudioFrames.begin(); it != m_AudioFrames.end(); ++it)
		{
			ApplyWindow(it->first);

			setup_plan(&fftPlan, it->first.data(), cache.data(), m_FrameSize);
			r2hc(&fftPlan);
			spectrogram.push_back(plan_samples(&fftPlan));
		}

		// Separate harmonic and percussive content
		std::vector<std::vector<float> > harmonicInfo = GetHarmonicSpectrogram(spectrogram, analysisSize);
		std::vector<std::vector<float> > percussiveInfo = GetPercussiveSpectrogram(spectrogram, analysisSize);

		std::vector<std::vector<float> > harmFilteredSpec, percFilteredSpec;

		size_t fftSize = m_FrameSize >> 1;
		harmFilteredSpec.resize(harmonicInfo.size());
		percFilteredSpec.resize(harmonicInfo.size());
		for (size_t k = 0; k < harmonicInfo.size(); ++k)
		{
			harmFilteredSpec.at(k).resize(m_FrameSize);
			percFilteredSpec.at(k).resize(m_FrameSize);
			for (size_t m = 0; m <= fftSize; ++m)
			{
				bool harmonicDominant = harmonicInfo.at(k).at(m) >= percussiveInfo.at(k).at(m);
				auto & filteredSpec = harmonicDominant ? harmFilteredSpec : percFilteredSpec;
				filteredSpec.at(k).at(m) = m_AudioFrames.at(k).first[m];
				if (m != 0 && m != fftSize)
				{
					filteredSpec.at(k).at(m_FrameSize - m) = m_AudioFrames.at(k).first[m_FrameSize - m];
				}
			}
		}

		// Take backward FFT of each frame.
		size_t index = 0;
		for (std::deque<DataOverlapPair>::iterator
				it = m_AudioFrames.begin(); it != m_AudioFrames.end(); ++it)
		{
			std::vector<float> & harmFilteredData = harmFilteredSpec.at(index);
			std::vector<float> & percFilteredData = percFilteredSpec.at(index);

			// Determine how much of the initial output to discard
			size_t discardAmount = it == m_AudioFrames.begin()
					? m_PrevDiscardAmount : (it - 1)->second;

			setup_plan(&fftPlan, harmFilteredData.data(), cache.data(), m_FrameSize);
			hc2r(&fftPlan);

			float * samples = plan_samples(&fftPlan);
			for (size_t k = discardAmount; k < m_FrameSize; ++k)
			{
				harmonicContent.push_back(samples[k] * GetWindowFactor(k, m_FrameSize));
			}

			setup_plan(&fftPlan, percFilteredData.data(), cache.data(), m_FrameSize);
			hc2r(&fftPlan);

			samples = plan_samples(&fftPlan);
			for (size_t k = discardAmount; k < m_FrameSize; ++k)
			{
				percussiveContent.push_back(samples[k] * GetWindowFactor(k, m_FrameSize));
			}

			++index;
		}

		// Finally, clean up
		m_ImaginarySize = 0;
		m_FirstRun = m_EOS;
		m_PrevDiscardAmount = m_AudioFrames.back().second;
		if (m_AudioFrames.size() <= m_TimeChunksPerSTFT)
		{
			m_AudioFrames.clear();
		}
		else
		{
			m_AudioFrames.erase(m_AudioFrames.begin() + m_TimeChunksPerSTFT);
		}
		if (m_EOS)
		{
			m_PrevDiscardAmount = 0;
			m_AudioData.clear();
			m_HarmonicState.clear();
			m_HarmonicState.resize((m_FrameSize >> 1) + 1);
			m_PercussiveState.clear();
		}
	}
}
