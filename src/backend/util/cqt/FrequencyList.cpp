#include "FrequencyList.h"
#include <cmath>

FrequencyList::FrequencyList(float minFreq, float maxFreq, size_t numBinsPerOctave, size_t sampRate)
{
	m_Frequencies.resize(ceilf(numBinsPerOctave * log2f(maxFreq / minFreq)));
	size_t k = 0;
	for (auto it = m_Frequencies.begin(); it != m_Frequencies.end(); ++it, ++k)
	{
		*it = (minFreq * powf(2.0f, k / (float) numBinsPerOctave)) / sampRate;
	}
}

FrequencyList::~FrequencyList()
{
}
