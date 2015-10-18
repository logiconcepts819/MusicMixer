#ifndef SRC_CORE_STRETCH_AUDIOSTRETCHER_H_
#define SRC_CORE_STRETCH_AUDIOSTRETCHER_H_

#include "../../util/AudioBufUtil.h"
#include <rubberband/RubberBandStretcher.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class AudioBlock;
class AudioStretchInfo;

class AudioStretcher
{
	static const size_t m_RubberbandBlockSize;

	std::mutex m_AudioStretchInfoQueueMutex;
	std::queue<std::shared_ptr<AudioStretchInfo> > m_AudioStretchInfoQueue;
	std::condition_variable m_AudioStretchInfoQueueCond;

	std::unique_ptr<RubberBand::RubberBandStretcher> m_RBS;
	std::shared_ptr<AudioStretchInfo> m_StretchInfo;
	std::shared_ptr<AudioBlock> m_Block;
	std::vector<float> m_RBBuf;
	AudioBuf m_RBProcBuf;
	bool m_Rotate;
	size_t m_BufPos;
	size_t m_BufLen;
	size_t m_NumSIFrames;
	size_t m_RefPos;
	size_t m_RelRefPos;
	double m_RefPosStretch;
	size_t m_CompRefPos;
	size_t m_RelCompRefPos;
	bool m_EOS;
	size_t m_Latency;
	size_t m_ConsumedLatency;
	bool m_PastInitialLatency;
	size_t m_FinalLatency;

	size_t GetFromRubberBand(float * buffer, size_t frames);
	void PutIntoRubberBand(const float * buffer, size_t frames, bool flush);

	std::shared_ptr<AudioStretchInfo> ObtainAudioStretchInfo();
	size_t ComputeRefPos(size_t stretchedSize, size_t & coarseRefPos, size_t & relRefPos);
public:
	AudioStretcher();
	virtual ~AudioStretcher();

	void InitRubberband();

	void SubmitAudioStretchInfo(const std::shared_ptr<AudioStretchInfo> & stretchInfo);

	const std::vector<float> & getStretchedAudio(size_t requestedStretchedSize, size_t & actualStretchedSize, size_t & refPos, size_t & compRefPos, bool & eos);
	std::shared_ptr<AudioBlock> getStretchedAudioAsBlock(size_t requestedStretchedSize, size_t & refPos, size_t & compRefPos, bool & eos);
};

#endif /* SRC_CORE_STRETCH_AUDIOSTRETCHER_H_ */
