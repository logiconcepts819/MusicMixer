#include "BandPassFIRFilter.h"
#include "../MathConstants.h"
#include <cmath>
#include <limits>

BandPassFIRFilter::BandPassFIRFilter(size_t filterSize, float lowerCutoffFreq, float upperCutoffFreq)
: FIRFilter(filterSize)
{
	float halfFilterSize = 0.5f * filterSize;
	for (size_t k = 0; k != filterSize; ++k)
	{
		float koff = k - halfFilterSize;
		if (std::abs(koff) < 4.0f * std::numeric_limits<float>::epsilon())
		{
			m_Filter.at(k) = 2.0f * (upperCutoffFreq - lowerCutoffFreq);
		}
		else
		{
			float trigFactor = 2.0f * MathConstants::Pi * koff;
			m_Filter.at(k) =
					(sinf(trigFactor * upperCutoffFreq)
					- sinf(trigFactor * lowerCutoffFreq)
					/ MathConstants::Pi) / koff;
		}
	}
	ApplyWindow();
}

BandPassFIRFilter::~BandPassFIRFilter()
{
}
