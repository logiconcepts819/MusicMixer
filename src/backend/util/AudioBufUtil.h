#ifndef SRC_UTIL_AUDIOBUFUTIL_H_
#define SRC_UTIL_AUDIOBUFUTIL_H_

#include <vector>
#include <cstring>

typedef std::vector<float*> AudioBuf;

namespace AudioBufUtil
{
	static const size_t MaxAudioBufLen = 160000;

	AudioBuf NewAudioBuffer(size_t numBlocks = MaxAudioBufLen);
	void FreeAudioBuffer(AudioBuf & buffer);
}

#endif /* SRC_UTIL_AUDIOBUFUTIL_H_ */
