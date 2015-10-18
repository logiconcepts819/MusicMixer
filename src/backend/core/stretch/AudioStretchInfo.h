#ifndef SRC_CORE_STRETCH_AUDIOSTRETCHINFO_H_
#define SRC_CORE_STRETCH_AUDIOSTRETCHINFO_H_

#include <memory>
#include <vector>
#include <string>

class AudioBlock;

class AudioStretchInfo
{
	std::vector<float> m_Buffer;
	bool m_LastOne;
	double m_TimeRatio;
	bool m_UseRefPos;
	size_t m_ReferencePos;
	bool m_UseComplementaryRefPos;
	size_t m_ComplementaryReferencePos;

public:
	AudioStretchInfo();
	virtual ~AudioStretchInfo();

	void AppendSamples(const AudioBlock & block, float scale = 1.0,
			size_t start = 0, size_t end = std::string::npos);
	void AppendSample(const float * sample, float scale = 1.0);
	std::shared_ptr<AudioBlock> GenerateBlock() const;

	const std::vector<float> & GetBuffer() const
	{
		return m_Buffer;
	}

	bool IsLastOne() const
	{
		return m_LastOne;
	}

	void SetLastOne(bool lastOne)
	{
		m_LastOne = lastOne;
	}

	double GetTimeRatio() const
	{
		return m_TimeRatio;
	}

	void SetTimeRatio(double timeRatio)
	{
		m_TimeRatio = timeRatio;
	}

	bool IsUsingRefPos() const
	{
		return m_UseRefPos;
	}

	void SetUsingRefPos(bool enable)
	{
		m_UseRefPos = enable;
	}

	bool IsUsingComplementaryRefPos() const
	{
		return m_UseComplementaryRefPos;
	}

	void SetUsingComplementaryRefPos(bool enable)
	{
		m_UseComplementaryRefPos = enable;
	}

	size_t GetReferencePos() const
	{
		return m_ReferencePos;
	}

	size_t GetComplementaryReferencePos() const
	{
		return m_ComplementaryReferencePos;
	}

	void SetReferencePos(size_t referencePos)
	{
		m_ReferencePos = referencePos;
	}

	void SetComplementaryReferencePos(size_t complementaryReferencePos)
	{
		m_ComplementaryReferencePos = complementaryReferencePos;
	}
};

#endif /* SRC_CORE_STRETCH_AUDIOSTRETCHINFO_H_ */
