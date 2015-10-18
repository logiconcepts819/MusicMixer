#ifndef SRC_UTIL_FIRWINDOWS_WINDOW_H_
#define SRC_UTIL_FIRWINDOWS_WINDOW_H_

#include <vector>
#include <cstring>

class Window {
protected:
	std::vector<float> m_Window;
public:
	Window(size_t fftSize);
	virtual ~Window();

	float At(size_t k) const { return m_Window.at(k); }
	void AssignWindow(std::vector<float> & vec) const { vec = m_Window; }
	void MultiplyByWindow(std::vector<float> & vec) const;
};

#endif /* SRC_UTIL_FIRWINDOWS_WINDOW_H_ */
