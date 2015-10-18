#include "HannWindow.h"
#include "../MathConstants.h"
#include <cmath>

HannWindow::HannWindow(size_t fftSize) : Window(fftSize)
{
	size_t k = 0;
	size_t Nm1 = m_Window.size() - 1;
	for (auto it = m_Window.begin(); it != m_Window.end(); ++it, ++k)
	{
		*it = 0.5f - 0.5f * cosf(2.0f * MathConstants::Pi * k / Nm1);
	}
}

HannWindow::~HannWindow()
{
}
