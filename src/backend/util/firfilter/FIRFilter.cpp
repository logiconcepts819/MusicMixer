#include "FIRFilter.h"
#include "../firwindows/HannWindow.h"

FIRFilter::FIRFilter(size_t filterSize)
{
	m_Filter.resize(filterSize);
	m_InputHistory.resize(filterSize);
}

FIRFilter::~FIRFilter()
{
}

void FIRFilter::ApplyWindow()
{
	HannWindow(m_Filter.size()).MultiplyByWindow(m_Filter);
}

float FIRFilter::ProcessSample(float input)
{
	float output = 0.0f;

	m_InputHistory.pop_front();
	m_InputHistory.push_back(input);

	size_t k = 0;
	for (auto it = m_InputHistory.begin(); it != m_InputHistory.end(); ++it, ++k)
	{
		output += *it * m_Filter.at(k);
	}
	return output;
}
