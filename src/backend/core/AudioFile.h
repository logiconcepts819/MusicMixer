#ifndef SRC_CORE_AUDIOFILE_H_
#define SRC_CORE_AUDIOFILE_H_

#include "AudioBlock.h"

extern "C" {
#ifdef USE_SWRESAMPLE
/* Using FFMpeg */
#include <libswresample/swresample.h>
static inline void swr_close(SwrContext *ctx) {};
#else
/* Using LibAV */
#include <libavresample/avresample.h>
#define SwrContext AVAudioResampleContext
#define swr_init(ctx) avresample_open(ctx)
#define swr_close(ctx) avresample_close(ctx)
#define swr_free(ctx) avresample_free(ctx)
#define swr_alloc() avresample_alloc_context()
#define swr_get_delay(ctx, ...) avresample_get_delay(ctx)
#define swr_convert(ctx, out, out_count, in, in_count) \
    avresample_convert(ctx, out, 0, out_count, (uint8_t **) (in), 0, in_count)
#define av_opt_set_sample_fmt(ctx, name, fmt, flags) \
    av_opt_set_int(ctx, name, fmt, flags)
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

#include <memory>
#include <mutex>

class AudioFile {
	std::string m_Filename;

	std::string m_Title;
	std::string m_Artist;
	std::string m_Album;
	std::string m_Year;

	double m_Position;
	double m_Duration;
	double m_PrevDelta;
	bool m_NegPosition;
	mutable std::mutex m_PositionMutex;

	static const int m_AudioFileBufSize;
	static const AVSampleFormat m_DestSampFmt;

	// Decoder stuff
	AVFormatContext * m_Container;
	int m_StreamId;
	AVPacket m_AvPkt;
	AVFrame * m_DecodedFrame;
	std::vector<uint8_t> m_InBuf;

	// Resampler stuff
	SwrContext * m_SwrContext;
	uint8_t ** m_DestData;
	int m_DestNumChannels;
	int m_MaxDestNumSamples;
	int m_DestSampRate;
	bool m_FileDone;
	mutable std::mutex m_FileDoneMux;

	bool m_DecodeDebug;

	bool OpenDecoder();
	bool OpenResampler();
	void LoadMetadata();
	bool Decode();
	void CloseDecoder();
	void CloseResampler();

	std::shared_ptr<AudioBlock> getNextAudioBlockAux();
public:
	AudioFile(int desiredNumChannels, int desiredSampleRate);
	virtual ~AudioFile();

	static void InitializeAvformat();
	bool OpenFile();

	std::string getFilename() const { return m_Filename; }
	void setFilename(const std::string & filename, bool loadMetadata = false);

	std::string getTitle() const { return m_Title; }
	std::string getArtist() const { return m_Artist; }
	std::string getAlbum() const { return m_Album; }
	std::string getYear() const { return m_Year; }
	double getPosition() const {
		std::lock_guard<std::mutex> lck(m_PositionMutex);
		return m_Position; }
	double getDuration() const { return m_Duration; }

	bool isFileDone() const {
		std::lock_guard<std::mutex> lck(m_FileDoneMux);
		return m_FileDone; }

	std::shared_ptr<AudioBlock> getNextAudioBlock();
	bool seek(double newPosition);
};

#endif /* SRC_CORE_AUDIOFILE_H_ */
