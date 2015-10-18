#!/bin/bash

g++ -fPIC -shared -std=c++11 -o libmixingapi.so \
		src/backend/core/AudioBlock.cpp \
		src/backend/core/AudioSink.cpp \
		src/backend/core/AudioFile.cpp \
		src/backend/core/AudioRequest.cpp \
		src/backend/core/RequestQueue.cpp \
		src/backend/core/stretch/AudioStretcher.cpp \
		src/backend/core/stretch/AudioStretchInfo.cpp \
		src/backend/core/xfade/Crossfader.cpp \
		src/backend/core/xfade/CrossfadeCalculator.cpp \
		src/backend/core/xfade/DJCrossfadeCalculator.cpp \
		src/backend/core/xfade/DJCrossfadeCalculatorOld.cpp \
		src/backend/core/xfade/fademaps/KneeFadeMap.cpp \
		src/backend/core/xfade/fademaps/LinearFadeMap.cpp \
		src/backend/core/xfade/fademaps/FadeMap.cpp \
		src/backend/core/filters/Filter.cpp \
		src/backend/core/filters/CubicInterpFilter.cpp \
		src/backend/core/filters/HoldBackQueue.cpp \
		src/backend/os/Path.cpp \
		src/backend/program/ProgramInfo.cpp \
		src/backend/util/AudioBufUtil.cpp \
		src/backend/util/StrUtil.cpp
