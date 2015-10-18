#include "CubicInterpFilter.h"
#include "../AudioBlock.h"
#include "../AudioSink.h"

CubicInterpFilter::CubicInterpFilter()
: m_HaveLeftSideInformation(false), m_LeftSample(0.0f), m_LeftSlope(0.0f),
  m_HaveRightSideInformation(false), m_RightSample(0.0f), m_RightSlope(0.0f)
{
	// TODO Auto-generated constructor stub
}

CubicInterpFilter::~CubicInterpFilter()
{
	// TODO Auto-generated destructor stub
}

void CubicInterpFilter::Reset()
{
	m_HaveLeftSideInformation = m_HaveRightSideInformation = false;
	m_LeftSample = m_LeftSlope = m_RightSample = m_RightSlope = 0.0f;
}

void CubicInterpFilter::GetLeftBlockInformation(const AudioBlock & block, size_t channel)
{
	size_t sampRate = AudioSink::Instance().getSampleRate();
	size_t numSamples = block.getNumSamples();
	size_t minNumSamples = (size_t) (m_TimeInterval * sampRate);
	size_t minNumSamplesLeft = (minNumSamples >> 1) + 1;
	if (numSamples >= minNumSamplesLeft)
	{
		size_t leftSideStart = numSamples - minNumSamplesLeft;
		m_HaveLeftSideInformation = true;
		float v1 = block.getSampleAtPosition(channel, leftSideStart);
		float v2 = block.getSampleAtPosition(channel, leftSideStart + 1);
		m_LeftSample = v2;
		m_LeftSlope = (v2 - v1) / AudioSink::Instance().getSampleRate();
	}
}

void CubicInterpFilter::GetRightBlockInformation(const AudioBlock & block, size_t channel)
{
	size_t sampRate = AudioSink::Instance().getSampleRate();
	size_t numSamples = block.getNumSamples();
	size_t minNumSamples = (size_t) (m_TimeInterval * sampRate) + 1;
	size_t minNumSamplesRight = ((minNumSamples + 1) >> 1) + 1;
	if (numSamples >= minNumSamplesRight)
	{
		m_HaveRightSideInformation = true;
		float v1 = block.getSampleAtPosition(channel, minNumSamplesRight - 2);
		float v2 = block.getSampleAtPosition(channel, minNumSamplesRight - 1);
		m_RightSample = v1;
		m_RightSlope = (v2 - v1) / AudioSink::Instance().getSampleRate();
	}
}

float CubicInterpFilter::ProcessSample(float sample, float t) const
{
	if (m_HaveLeftSideInformation && m_HaveRightSideInformation)
	{
		if (t >= 0.0f && t <= 1.0f)
		{
			float tsq = t * t;
			float h00 = 1.0f + tsq * (-3.0f + t * 2.0f);
			float h01 = t * (1.0f + t * (-2.0f - t));
			float h10 = tsq * (3.0f + t * -2.0f);
			float h11 = tsq * (-1.0f + t);
			sample = h00 * m_LeftSample + h01 * m_LeftSlope
				   + h10 * m_RightSample + h11 * m_RightSlope;
		}
	}
	return sample;
}
