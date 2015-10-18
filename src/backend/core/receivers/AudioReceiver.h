#ifndef SRC_CORE_RECEIVERS_AUDIORECEIVER_H_
#define SRC_CORE_RECEIVERS_AUDIORECEIVER_H_

#include <memory>

class AudioBlock;

class AudioReceiver
{
public:
	AudioReceiver();
	virtual ~AudioReceiver();

	virtual void OnAudioInput(const std::shared_ptr<AudioBlock> & blk);
	virtual void OnFadeStart();
	virtual void OnFadeEnd();
};

#endif /* SRC_CORE_RECEIVERS_AUDIORECEIVER_H_ */
