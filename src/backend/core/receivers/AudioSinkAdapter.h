#ifndef SRC_CORE_RECEIVERS_AUDIOSINKADAPTER_H_
#define SRC_CORE_RECEIVERS_AUDIOSINKADAPTER_H_

#include "AudioReceiver.h"

class AudioSinkAdapter: public AudioReceiver
{
public:
	AudioSinkAdapter();
	virtual ~AudioSinkAdapter();

	void OnAudioInput(const std::shared_ptr<AudioBlock> & blk);
};

#endif /* SRC_CORE_RECEIVERS_AUDIOSINKADAPTER_H_ */
