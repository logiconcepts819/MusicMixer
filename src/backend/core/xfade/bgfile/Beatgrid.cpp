#include "Beatgrid.h"

Beatgrid::Beatgrid()
: m_HasGrid(false), m_RefTime(0.0), m_RefTimeOffset(0.0), m_BaseTempo(0.0)
{
}

Beatgrid::Beatgrid(const Beatgrid & bg)
: m_HasGrid(bg.m_HasGrid), m_RefTime(bg.m_RefTime),
  m_RefTimeOffset(bg.m_RefTimeOffset), m_BaseTempo(bg.m_BaseTempo),
  m_BPMBeatgrid(bg.m_BPMBeatgrid), m_TimeBeatgrid(bg.m_TimeBeatgrid)
{
}

Beatgrid::Beatgrid(Beatgrid && bg)
: m_HasGrid(std::move(bg.m_HasGrid)), m_RefTime(std::move(bg.m_RefTime)),
  m_RefTimeOffset(std::move(bg.m_RefTimeOffset)),
  m_BaseTempo(std::move(bg.m_BaseTempo)),
  m_BPMBeatgrid(std::move(bg.m_BPMBeatgrid)),
  m_TimeBeatgrid(std::move(bg.m_TimeBeatgrid))
{
}

Beatgrid::~Beatgrid() {
	// TODO Auto-generated destructor stub
}

Beatgrid & Beatgrid::operator=(const Beatgrid & rhs)
{
	if (this != &rhs)
	{
		m_HasGrid = rhs.m_HasGrid;
		m_RefTime = rhs.m_RefTime;
		m_RefTimeOffset = rhs.m_RefTimeOffset;
		m_BaseTempo = rhs.m_BaseTempo;
		m_BPMBeatgrid = rhs.m_BPMBeatgrid;
		m_TimeBeatgrid = rhs.m_TimeBeatgrid;
	}
	return *this;
}

Beatgrid & Beatgrid::operator=(Beatgrid && rhs)
{
	if (this != &rhs)
	{
		m_HasGrid = std::move(rhs.m_HasGrid);
		m_RefTime = std::move(rhs.m_RefTime);
		m_RefTimeOffset = std::move(rhs.m_RefTimeOffset);
		m_BaseTempo = std::move(rhs.m_BaseTempo);
		m_BPMBeatgrid = std::move(rhs.m_BPMBeatgrid);
		m_TimeBeatgrid = std::move(rhs.m_TimeBeatgrid);
	}
	return *this;
}

Beatgrid Beatgrid::GetBeatgridSection(size_t start) const
{
	Beatgrid bg;
	bg.m_RefTime = m_RefTime;
	bg.m_RefTimeOffset = m_RefTimeOffset + start;
	bg.m_BaseTempo = m_BaseTempo;
	if (start < m_BPMBeatgrid.size())
	{
		bg.m_BPMBeatgrid.assign(m_BPMBeatgrid.begin() + start,
				m_BPMBeatgrid.end());
		bg.m_TimeBeatgrid.assign(m_TimeBeatgrid.begin() + start,
				m_TimeBeatgrid.end());
	}
	return bg;
}

Beatgrid Beatgrid::GetBeatgridSection(size_t start,
		size_t count) const
{
	if (start + count < m_BPMBeatgrid.size())
	{
		Beatgrid bg;
		bg.m_RefTime = m_RefTime;
		bg.m_RefTimeOffset = m_RefTimeOffset + start;
		bg.m_BaseTempo = m_BaseTempo;
		bg.m_BPMBeatgrid.assign(m_BPMBeatgrid.begin() + start,
				m_BPMBeatgrid.begin() + (start + count));
		return bg;
	}
	return GetBeatgridSection(start);
}

void Beatgrid::Sanitize()
{

}
