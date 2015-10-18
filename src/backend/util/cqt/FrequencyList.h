#ifndef SRC_UTIL_CQT_FREQUENCYLIST_H_
#define SRC_UTIL_CQT_FREQUENCYLIST_H_

#include <vector>
#include <cstring>

class FrequencyList {
	std::vector<float> m_Frequencies;
public:
	FrequencyList(float minFreq, float maxFreq, size_t numBinsPerOctave, size_t sampRate);
	virtual ~FrequencyList();

	float At(size_t index) const { return m_Frequencies.at(index); }
};

#endif /* SRC_UTIL_CQT_FREQUENCYLIST_H_ */
