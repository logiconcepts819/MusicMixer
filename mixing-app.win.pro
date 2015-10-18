TEMPLATE = app
TARGET = mixing-app
INCLUDEPATH += .

#CONFIG += debug

DEFINES += USE_SWRESAMPLE
QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS +=
LIBS += -lrubberband -lportaudio -lavformat -lavcodec -lavutil -lswresample -lm -lz -lws2_32 -liconv -lvamp-sdk -lsamplerate -lsndfile -lfftw3

INCLUDEPATH += /local/include
LIBS += -L/local/lib

QT += widgets

UI_DIR = src

HEADERS += src/TheMainWindow.h \
	src/backend/core/AudioBlock.h \
	src/backend/core/AudioSink.h \
	src/backend/core/AudioFile.h \
	src/backend/core/AudioRequest.h \
	src/backend/core/RequestQueue.h \
	src/backend/core/stretch/AudioStretcher.h \
	src/backend/core/stretch/AudioStretchInfo.h \
	src/backend/core/xfade/Crossfader.h \
	src/backend/core/xfade/CrossfadeCalculator.h \
	src/backend/core/xfade/DJCrossfadeCalculator.h \
	src/backend/core/xfade/DJCrossfadeCalculatorOld.h \
	src/backend/core/xfade/fademaps/KneeFadeMap.h \
	src/backend/core/xfade/fademaps/LinearFadeMap.h \
	src/backend/core/xfade/fademaps/FadeMap.h \
	src/backend/core/filters/Filter.h \
	src/backend/core/filters/CubicInterpFilter.h \
	src/backend/core/filters/HoldBackQueue.h \
	src/backend/os/Path.h \
	src/backend/program/ProgramInfo.h \
	src/backend/util/AudioBufUtil.h \
	src/backend/util/StrUtil.h
FORMS += ui/mixing-app.ui
RESOURCES = mixing-app.qrc
SOURCES += src/main.cpp src/TheMainWindow.cpp \
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
