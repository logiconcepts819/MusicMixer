#ifndef SRC_CORE_REQUESTQUEUE_H_
#define SRC_CORE_REQUESTQUEUE_H_

#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "AudioRequest.h"
#include "AudioSink.h"

class AudioFile;
class AudioBlock;
class Crossfader;
class FadeMap;

class RequestQueue {
	std::condition_variable m_DEQueueHasDataCond;
	std::condition_variable m_DEQueueHasNoDataCond;
	std::deque<std::shared_ptr<AudioRequest> > m_DEQueue;
	std::unique_ptr<std::thread> m_RequestThread;
	std::shared_ptr<AudioSink> m_AudioSink;
	std::unique_ptr<AudioFile> m_AudioFile;
	std::unique_ptr<AudioFile> m_NextAudioFile;
	std::unique_ptr<Crossfader> m_Crossfader;
	std::shared_ptr<AudioBlock> m_CrossfadeLeftover;
	std::unique_ptr<FadeMap> m_FadeMap;

	std::mutex m_ThreadMutex;
	bool m_TerminateThread;
	bool m_ThreadRunning;

	static const double m_DefaultXfadeDuration;

	bool m_EnableNormalXfade;
	bool m_EnableDJXFade;
	double m_XfadeDuration;

	bool m_UseOptimisticTempoAdaptation;

	static void ProcessRequests(RequestQueue * reqQueue);
	void ProcessNextRequest();
	void DoProcessRequests();

protected:
	RequestQueue();
public:
	virtual ~RequestQueue();

	static RequestQueue & Instance();

	bool IsNormalXfadeEnabled() const
	{
		return m_EnableNormalXfade;
	}

	void SetNormalXfadeEnabled(bool enable)
	{
		m_EnableNormalXfade = enable;
	}

	bool IsDJXfadeEnabled() const
	{
		return m_EnableDJXFade;
	}

	void SetDJXfadeEnabled(bool enable)
	{
		m_EnableDJXFade = enable;
	}

	static double GetDefaultXfadeDuration();

	double GetXfadeDuration() const
	{
		return m_XfadeDuration;
	}

	void SetXfadeDuration(double xfadeDuration)
	{
		m_XfadeDuration = xfadeDuration;
	}

	FadeMap & GetFadeMap();
	const FadeMap & GetFadeMap() const;
	void TakeFadeMap(std::unique_ptr<FadeMap> && fadeMap);

	void StartRequestProcessor();
	void StopRequestProcessor();
	void StopRequestProcessorAndSink();

	void Play(const std::string & filename);
	void PlayNext(const std::string & filename);

	void WaitForEmptyQueue();
	virtual void OnMetadataLoaded(const AudioFile & audioFile)
		{ (void) audioFile; }
	virtual void OnPositionUpdate(const AudioFile & audioFile)
		{ (void) audioFile; }
};

#endif /* SRC_CORE_REQUESTQUEUE_H_ */
