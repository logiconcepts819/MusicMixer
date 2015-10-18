#ifndef SRC_UTIL_FIRWINDOWS_HANNWINDOW_H_
#define SRC_UTIL_FIRWINDOWS_HANNWINDOW_H_

#include "Window.h"

class HannWindow: public Window
{
public:
	HannWindow(size_t fftSize);
	virtual ~HannWindow();
};

#endif /* SRC_UTIL_FIRWINDOWS_HANNWINDOW_H_ */
