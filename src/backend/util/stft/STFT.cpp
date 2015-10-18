#include "STFT.h"
#include "../MathConstants.h"
#include "../firwindows/HannWindow.h"
#include <cmath>
#include <cstring>

STFT::STFT()
: m_SpecProcessor(nullptr), m_FrameSize(0), m_OsampFactor(0), m_OutSize(0),
  m_FIFOPos(0), m_StepSize(0), m_FFTLatency(0), m_HeadLatencyConsumed(0),
  m_TailLatencyConsumed(0), m_TotalLatency(0)
{
}

STFT::~STFT()
{
}

void STFT::Initialize(SpectrumProcessor * processor, size_t osampFactor)
{
	m_SpecProcessor = processor;

	m_FrameSize = processor->GetFFTSize();
	m_OsampFactor = osampFactor;

	// Set up input configurations
	m_InFIFO.clear();
	m_InFIFO.resize(m_FrameSize);

	m_InFFTData.clear();
	m_InFFTData.resize(m_FrameSize);

	m_InFFTCache.clear();
	m_InFFTCache.resize(plan_cache_size(m_FrameSize));

	setup_plan(&m_InFFTPlan, m_InFFTData.data(), m_InFFTCache.data(), m_FrameSize);

	// Set up output configurations
	m_OutInfo.clear();
	m_OutInfo.resize(processor->GetNumOutputs());
	for (std::vector<OutputInfo>::iterator
			it = m_OutInfo.begin(); it != m_OutInfo.end(); ++it)
	{
		it->m_OutFIFO.resize(m_FrameSize);
		it->m_OutAccum.resize(m_FrameSize << 1);
		it->m_OutFFTData.resize(m_FrameSize);
		it->m_OutFFTCache.resize(plan_cache_size(m_FrameSize));
		setup_plan(&it->m_OutFFTPlan, it->m_OutFFTData.data(),
				it->m_OutFFTCache.data(), m_FrameSize);
	}

	// Set up window
	HannWindow(m_FrameSize).AssignWindow(m_Window);

	m_StepSize = m_FrameSize / osampFactor;
	m_FIFOPos = m_FFTLatency = m_FrameSize - m_StepSize;

	// Set up latency variables
	m_HeadLatencyConsumed = m_TailLatencyConsumed = 0;
	m_TotalLatency = m_FFTLatency + m_StepSize * processor->GetProcessDelayPerStep();
}

void STFT::SubmitSample(float sample)
{
	m_InFIFO.at(m_FIFOPos) = sample;
	for (size_t k = 0; k != m_SpecProcessor->GetNumOutputs(); ++k)
	{
		OutputInfo & outInfo = m_OutInfo.at(k);
		outInfo.m_Output.push_back(
				outInfo.m_OutFIFO.at(m_FIFOPos - m_FFTLatency));
	}
	++m_OutSize;

	if (++m_FIFOPos >= m_FrameSize)
	{
		m_FIFOPos = m_FFTLatency;
		for (size_t k = 0; k != m_FrameSize; ++k)
		{
			m_InFFTData.at(k) = m_InFIFO.at(k) * m_Window.at(k);
		}
		r2hc(&m_InFFTPlan);

		float * inputPtr = plan_samples(&m_InFFTPlan);
		std::vector<float> input(inputPtr, inputPtr + m_FrameSize);
		m_SpecProcessor->Process(input);
		for (size_t k = 0; k != m_SpecProcessor->GetNumOutputs(); ++k)
		{
			const std::vector<float> & data = m_SpecProcessor->GetOutputSpectrum(k);
			OutputInfo & outInfo = m_OutInfo.at(k);
			memcpy(outInfo.m_OutFFTData.data(), data.data(), m_FrameSize * sizeof(float));
			hc2r(&outInfo.m_OutFFTPlan);
			float * outSamples = plan_samples(&outInfo.m_OutFFTPlan);
			for (size_t m = 0; m != m_FrameSize; ++m)
			{
				outInfo.m_OutAccum.at(m) += outSamples[m] * m_Window.at(m)
						* 1.27519f / ((m_FrameSize >> 1) * m_OsampFactor);
			}
			memcpy(outInfo.m_OutFIFO.data(), outInfo.m_OutAccum.data(), m_StepSize * sizeof(float));
			memmove(outInfo.m_OutAccum.data(), outInfo.m_OutAccum.data() + m_StepSize, m_FrameSize * sizeof(float));
			memmove(m_InFIFO.data(), m_InFIFO.data() + m_StepSize, m_FFTLatency * sizeof(float));
		}
	}
}

/*
 * A typical flow of calls here:
 *
 * stft.Initialize();
 * while (have_more_input() && !stft.DiscardInitialOutputAndCheck()) {
 *     stft.SubmitSample(get_input_sample());
 * }
 * while (have_more_input()) {
 *     stft.SubmitSample(get_input_sample());
 *     process_output(stft.GatherOutput());
 * }
 * while (!stft.OutputFinalized()) {
 *     vector<vector<float> > lastOutput = stft.GatherFinalOutput();
 *     if (!lastOutput.empty()) process_output(lastOutput);
 * }
 */

bool STFT::DiscardInitialOutputAndCheck(size_t maxNumSamples)
{
	if (m_HeadLatencyConsumed != m_TotalLatency)
	{
		size_t samplesRem = maxNumSamples == std::string::npos
				? m_OutSize : std::min(m_OutSize, maxNumSamples);
		samplesRem = std::min(m_TotalLatency - m_HeadLatencyConsumed,
				samplesRem);

		while (samplesRem != 0)
		{
			for (auto it = m_OutInfo.begin(); it != m_OutInfo.end(); ++it)
			{
				it->m_Output.pop_front();
			}
			--m_OutSize;
			--samplesRem;
			++m_HeadLatencyConsumed;
		}
	}

	return m_HeadLatencyConsumed == m_TotalLatency;
}

std::vector<std::vector<float> > STFT::GatherOutput(size_t maxNumSamples)
{
	std::vector<std::vector<float> > output;
	size_t samplesRem = maxNumSamples == std::string::npos
			? m_OutSize : std::min(m_OutSize, maxNumSamples);

	output.resize(m_SpecProcessor->GetNumOutputs());
	while (samplesRem != 0)
	{
		auto outIt = output.begin();
		for (auto it = m_OutInfo.begin(); it != m_OutInfo.end(); ++it, ++outIt)
		{
			outIt->push_back(it->m_Output.front());
			it->m_Output.pop_front();
		}
		--m_OutSize;
		--samplesRem;
	}

	return output;
}

std::vector<std::vector<float> > STFT::GatherFinalOutput(size_t maxNumSamples)
{
	std::vector<std::vector<float> > output;

	size_t difference = (m_TotalLatency - m_TailLatencyConsumed)
			+ (m_TotalLatency - m_HeadLatencyConsumed);

	if (difference != 0)
	{
		size_t samplesRem = maxNumSamples == std::string::npos
				? difference : std::min(difference, maxNumSamples);

		output.resize(m_SpecProcessor->GetNumOutputs());
		while (samplesRem != 0)
		{
			SubmitSample(0.0f); // insert a dummy sample

			auto outIt = output.begin();
			for (auto it = m_OutInfo.begin(); it != m_OutInfo.end(); ++it, ++outIt)
			{
				if (m_HeadLatencyConsumed == m_TotalLatency)
				{
					outIt->push_back(it->m_Output.front());
				}
				it->m_Output.pop_front();
			}
			--m_OutSize;
			--samplesRem;
			if (m_HeadLatencyConsumed == m_TotalLatency)
			{
				// Consumed all the latency at the head; now, we need to
				// consume it at the tail
				++m_TailLatencyConsumed;
			}
			else
			{
				// We still need to consume the latency at the head
				++m_HeadLatencyConsumed;
			}
		}
	}

	return output;
}

bool STFT::OutputFinalized() const
{
	return m_HeadLatencyConsumed == m_TotalLatency &&
			m_TailLatencyConsumed == m_TotalLatency;
}
