#include "HighPassFIRFilter.h"
#include "../MathConstants.h"
#include <cmath>
#include <limits>

HighPassFIRFilter::HighPassFIRFilter(size_t filterSize, float cutoffFreq)
: FIRFilter(filterSize)
{
	float halfFilterSize = 0.5f * filterSize;
	for (size_t k = 0; k != filterSize; ++k)
	{
		float koff = k - halfFilterSize;
		if (std::abs(koff) < 4.0f * std::numeric_limits<float>::epsilon())
		{
			m_Filter.at(k) = 1.0f - 2.0f * cutoffFreq;
		}
		else
		{
			m_Filter.at(k) =
					(-sinf(2.0f * MathConstants::Pi * cutoffFreq * koff)
					/ MathConstants::Pi) / koff;
		}
	}
	ApplyWindow();
}

HighPassFIRFilter::~HighPassFIRFilter()
{
}
