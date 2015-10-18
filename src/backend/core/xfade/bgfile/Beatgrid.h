#ifndef SRC_CORE_XFADE_BGFILE_BEATGRID_H_
#define SRC_CORE_XFADE_BGFILE_BEATGRID_H_

#include <vector>
#include <cstring>

class Beatgrid
{
	bool m_HasGrid;
	double m_RefTime;
	size_t m_RefTimeOffset;
	double m_BaseTempo;
	std::vector<double> m_BPMBeatgrid;
	std::vector<double> m_TimeBeatgrid;
public:
	Beatgrid();
	Beatgrid(const Beatgrid & bg);
	Beatgrid(Beatgrid && bg);
	virtual ~Beatgrid();

	Beatgrid & operator=(const Beatgrid & rhs);
	Beatgrid & operator=(Beatgrid && rhs);

	bool HasGrid() const
	{
		return m_HasGrid;
	}

	double GetRefTime() const
	{
		return m_RefTime;
	}

	void SetRefTime(double refTime)
	{
		m_RefTime = refTime;
	}

	double GetBaseTempo() const
	{
		return m_BaseTempo;
	}

	void SetBaseTempo(double baseTempo)
	{
		m_BaseTempo = baseTempo;
	}

	const std::vector<double> & GetBPMBeatgrid() const
	{
		return m_BPMBeatgrid;
	}

	const std::vector<double> & GetTimeBeatgrid() const
	{
		return m_TimeBeatgrid;
	}

	Beatgrid GetBeatgridSection(size_t start = 0) const;
	Beatgrid GetBeatgridSection(size_t start, size_t count) const;

	// To be called after obtaining offset + beatgrid
	void Sanitize();
};

#endif /* SRC_CORE_XFADE_BGFILE_BEATGRID_H_ */
