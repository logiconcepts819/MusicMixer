#include <iostream>
#include <cstdlib>
#include <cassert>
#include <csignal>
#include <thread>
#include <chrono>
#include "../backend/core/AudioSink.h"
#include "../backend/core/AudioFile.h"
#include "../backend/core/RequestQueue.h"
#include "../backend/core/xfade/DJCrossfadeCalculator.h"
#include "../backend/core/xfade/fademaps/KneeFadeMap.h"
#include "../backend/core/filters/CubicInterpFilter.h"
#include "../backend/util/StrUtil.h"

class MyRequestQueue : public RequestQueue
{
	MyRequestQueue() : RequestQueue() {}

	void BreakUpTime(double time, int & minute, double & second)
	{
		minute = floor(time / 60.0);
		second = time - 60.0 * minute;
	}
public:
	virtual ~MyRequestQueue() {}

	static MyRequestQueue & Instance()
	{
		static MyRequestQueue inst;
		return inst;
	}

	void OnMetadataLoaded(const AudioFile & file)
	{
		std::cout << "Now playing: " << file.getTitle();
		if (!file.getArtist().empty())
		{
			std::cout << " by " << file.getArtist();
		}
		std::cout << std::endl;
	}

	void OnPositionUpdate(const AudioFile & file)
	{
		int positionMin, durationMin;
		double positionSec, durationSec;
		BreakUpTime(file.getPosition(), positionMin, positionSec);
		BreakUpTime(file.getDuration(), durationMin, durationSec);
		std::cout << "Position: "
			  << StrUtil::format("%02d:%05.2f/%02d:%05.2f",
			     positionMin, positionSec, durationMin, durationSec)
			  << std::endl;
	}
};

static MyRequestQueue & reqQueue = MyRequestQueue::Instance();

static void signal_handler(int signal)
{
	(void) signal;
	reqQueue.StopRequestProcessorAndSink();
	exit(EXIT_SUCCESS);
}

int main(void)
{
	std::string text;

	// Work around an issue that is causing segfaults when Python opens
	// the application
	std::this_thread::sleep_for(std::chrono::seconds(2));

	signal(SIGINT, signal_handler);

	if (AudioSink::Instance().StartSink())
        {
                std::unique_ptr<CubicInterpFilter> filter(new CubicInterpFilter)
;
                filter->SetTimeInterval(0.0005);
                AudioSink::Instance().takeClickRemovalFilter(std::move(filter));

                std::unique_ptr<KneeFadeMap> fadeMap(new KneeFadeMap);
                reqQueue.TakeFadeMap(std::move(fadeMap));

                AudioFile::InitializeAvformat();
                reqQueue.StartRequestProcessor();
                reqQueue.SetDJXfadeEnabled(true);
	}

	for (;;)
	{
		std::getline(std::cin, text);
		if (!text.empty())
		{
			reqQueue.Play(text);
		}
	}

	return 0;
}
