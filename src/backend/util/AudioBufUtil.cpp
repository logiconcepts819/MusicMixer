#include "AudioBufUtil.h"
#include "../core/AudioSink.h"

AudioBuf AudioBufUtil::NewAudioBuffer(size_t numBlocks)
{
	AudioBuf newBuf;
	for (size_t ch = 0; ch < (size_t) AudioSink::Instance().getNumChannels(); ++ch)
	{
		newBuf.push_back(new float[numBlocks]);
	}
	return newBuf;
}

void AudioBufUtil::FreeAudioBuffer(AudioBuf & buffer)
{
	while (!buffer.empty())
	{
		AudioBuf::iterator lastOne = buffer.end() - 1;
		delete[] *lastOne;
		buffer.erase(lastOne);
	}
}
