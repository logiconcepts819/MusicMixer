#include "Window.h"

Window::Window(size_t fftSize)
{
	m_Window.resize(fftSize);
}

Window::~Window()
{
}

void Window::MultiplyByWindow(std::vector<float> & vec) const
{
	for (size_t k = 0; k != m_Window.size(); ++k)
	{
		vec.at(k) *= m_Window.at(k);
	}
}
