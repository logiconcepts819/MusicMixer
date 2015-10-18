#include "AudioReceiver.h"
#include "../AudioBlock.h"

AudioReceiver::AudioReceiver() {
	// TODO Auto-generated constructor stub

}

AudioReceiver::~AudioReceiver() {
	// TODO Auto-generated destructor stub
}

void AudioReceiver::OnAudioInput(const std::shared_ptr<AudioBlock> & blk)
{
}

void AudioReceiver::OnFadeStart()
{
}

void AudioReceiver::OnFadeEnd()
{
}
