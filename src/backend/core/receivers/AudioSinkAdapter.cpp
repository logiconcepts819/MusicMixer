#include "AudioSinkAdapter.h"
#include "../AudioBlock.h"
#include "../AudioSink.h"

AudioSinkAdapter::AudioSinkAdapter() {
	// TODO Auto-generated constructor stub

}

AudioSinkAdapter::~AudioSinkAdapter() {
	// TODO Auto-generated destructor stub
}

void AudioSinkAdapter::OnAudioInput(const std::shared_ptr<AudioBlock> & blk)
{
	AudioSink::Instance().SubmitAudioBlock(blk);
}
