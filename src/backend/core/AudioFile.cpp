#include "AudioFile.h"
#include "AudioSink.h"
#include "../os/Path.h"
#include <cstdint>
#include <iostream>

#define USE_RESAMPLER 1

using namespace std;

#ifndef MAX_AUDIO_FRAME_SIZE
#ifdef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define MAX_AUDIO_FRAME_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE
#else
#define MAX_AUDIO_FRAME_SIZE 192000
#endif
#endif

const int AudioFile::m_AudioFileBufSize =
		MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
const AVSampleFormat AudioFile::m_DestSampFmt = AV_SAMPLE_FMT_FLTP;

AudioFile::AudioFile(int desiredNumChannels, int desiredSampleRate)
: m_Position(0.0), m_Duration(0.0), m_PrevDelta(0.0), m_NegPosition(false),
  m_Container(nullptr), m_StreamId(0), m_DecodedFrame(nullptr),
  m_SwrContext(nullptr), m_DestData(nullptr),
  m_DestNumChannels(desiredNumChannels), m_MaxDestNumSamples(0),
  m_DestSampRate(desiredSampleRate), m_FileDone(false), m_DecodeDebug(false) {
	m_InBuf.resize(m_AudioFileBufSize);
}

AudioFile::~AudioFile() {
	CloseResampler();
	CloseDecoder();
}

void AudioFile::InitializeAvformat()
{
	av_register_all();
}

void AudioFile::CloseDecoder()
{
	if (m_DecodedFrame)
	{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
        avcodec_free_frame(&m_DecodedFrame);
#else
		av_frame_free(&m_DecodedFrame);
#endif
		m_DecodedFrame = nullptr;
	}
	if (m_Container)
	{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53,4,0)
        av_close_input_file(m_Container);
#else
		avformat_close_input(&m_Container);
#endif
		m_Container = nullptr;
	}
}

void AudioFile::CloseResampler()
{
	if (m_DestData)
	{
		av_freep(&m_DestData[0]);
		av_freep(&m_DestData);
		m_DestData = nullptr;
	}
	if (m_SwrContext)
	{
		swr_close(m_SwrContext);
		swr_free(&m_SwrContext);
		m_SwrContext = nullptr;
	}
}

bool AudioFile::OpenDecoder()
{
	bool rv = false;
	if (avformat_open_input(&m_Container, m_Filename.c_str(),
			nullptr, nullptr) >= 0 &&
		avformat_find_stream_info(m_Container, nullptr) >= 0)
	{
		AVCodec * codec;
		m_StreamId = -1;
		for (unsigned int i = 0;
				m_StreamId == -1 && i < m_Container->nb_streams; ++i)
		{
			if (m_Container->streams[i]->codec->codec_type
					== AVMEDIA_TYPE_AUDIO)
			{
				m_StreamId = (int) i;
			}
		}
		if (m_StreamId != -1)
		{
			AVCodecContext * codecContainer =
					m_Container->streams[m_StreamId]->codec;
			if ((codec = avcodec_find_decoder(codecContainer->codec_id)) &&
				avcodec_open2(codecContainer, codec, NULL) >= 0 &&
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
            	(m_DecodedFrame = avcodec_alloc_frame()))
#else
				(m_DecodedFrame = av_frame_alloc()))
#endif
			{
				av_init_packet(&m_AvPkt);
				m_AvPkt.data = m_InBuf.data();
				m_AvPkt.size = (int) m_InBuf.size();
				m_AvPkt.stream_index = m_StreamId;
				rv = true;
			}
		}
	}
	if (!rv)
	{
		CloseDecoder();
	}
	return rv;
}

bool AudioFile::OpenResampler()
{
	bool rv = false;

	AVCodecContext * codecContainer = m_Container->streams[m_StreamId]->codec;
	uint64_t srcChLayout = codecContainer->channel_layout == 0
			? av_get_default_channel_layout(codecContainer->channels)
			: codecContainer->channel_layout;
	int srcNumChannels = codecContainer->channels;
	int srcSampRate = codecContainer->sample_rate;
	int srcSampFmt = codecContainer->sample_fmt;
	int srcNumSamples = 1024; /* Temporary number to start off */
	int destChLayout = m_DestNumChannels == 1 ? AV_CH_LAYOUT_MONO
			: AV_CH_LAYOUT_STEREO;

	if ((m_SwrContext = swr_alloc()))
	{
		av_opt_set_int(m_SwrContext, "in_channel_layout",    srcChLayout, 0);
		av_opt_set_int(m_SwrContext, "in_sample_rate",       srcSampRate, 0);
		av_opt_set_sample_fmt(m_SwrContext, "in_sample_fmt", srcSampFmt,  0);

		av_opt_set_int(m_SwrContext, "out_channel_layout",    destChLayout,   0);
		av_opt_set_int(m_SwrContext, "out_sample_rate",       m_DestSampRate, 0);
		av_opt_set_sample_fmt(m_SwrContext, "out_sample_fmt", m_DestSampFmt,  0);

		if (swr_init(m_SwrContext) >= 0 &&
			((m_DestData =
				(uint8_t **) av_mallocz(sizeof(*m_DestData) * srcNumChannels))))
		{
			int destNumSamples = m_MaxDestNumSamples =
					av_rescale_rnd(srcNumSamples, m_DestSampRate, srcSampRate,
							AV_ROUND_UP);
			m_DestNumChannels =
					av_get_channel_layout_nb_channels(destChLayout);
			rv = av_samples_alloc(m_DestData, nullptr,
					m_DestNumChannels, destNumSamples, m_DestSampFmt, 0) >= 0;
		}
	}

	if (!rv)
	{
		CloseResampler();
	}
	return rv;
}

bool AudioFile::OpenFile()
{
	bool rv = false;
	if (OpenDecoder())
	{
#ifdef USE_RESAMPLER
		if (OpenResampler())
		{
			rv = true;
			{
				lock_guard<mutex> lck(m_FileDoneMux);
				m_FileDone = false;
			}
		}
		else
		{
			CloseDecoder();
		}
#else
		rv = true;
		{
			lock_guard<mutex> lck(m_FileDoneMux);
			m_FileDone = false;
		}
#endif
	}
	return rv;
}

void AudioFile::LoadMetadata()
{
	AVDictionaryEntry * tag;

	tag = av_dict_get(m_Container->metadata, "title", nullptr, 0);
	m_Title = tag ? tag->value : "";

	tag = av_dict_get(m_Container->metadata, "artist", nullptr, 0);
	m_Artist = tag ? tag->value : "";

	tag = av_dict_get(m_Container->metadata, "album", nullptr, 0);
	m_Album = tag ? tag->value : "";

	tag = av_dict_get(m_Container->metadata, "year", nullptr, 0);
	m_Year = tag ? tag->value : "";

	m_Position = m_Container->start_time / ((double) AV_TIME_BASE);
	m_Duration = m_Container->duration / ((double) AV_TIME_BASE);
	m_PrevDelta = 0.0;
}

void AudioFile::setFilename(const std::string & filename, bool loadMetadata)
{
	m_Filename = filename;
	if (loadMetadata)
	{
		bool doClose = false;
		bool success = true;
		if (!m_Container)
		{
			success = (doClose = avformat_open_input(&m_Container,
					m_Filename.c_str(), nullptr, nullptr) >= 0) &&
					avformat_find_stream_info(m_Container, nullptr) >= 0;
		}
		if (success)
		{
			LoadMetadata();
		}
		if (doClose)
		{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53,4,0)
			av_close_input_file(m_Container);
#else
			avformat_close_input(&m_Container);
#endif
			m_Container = nullptr;
		}
	}
	if (m_Title.empty())
	{
		m_Title = Path::GetBaseName(m_Filename);
	}
}

bool AudioFile::Decode()
{
	bool fileDone = false;
	bool frameDone = false;

	while (!frameDone &&
			!(fileDone = av_read_frame(m_Container, &m_AvPkt) < 0))
	{
		if (m_AvPkt.stream_index == m_StreamId)
		{
			int gotFramePtr = 0;
			AVCodecContext * codecContainer =
					m_Container->streams[m_StreamId]->codec;
			av_frame_unref(m_DecodedFrame);
			avcodec_decode_audio4(codecContainer, m_DecodedFrame,
					&gotFramePtr, &m_AvPkt);
			frameDone = gotFramePtr;
		}
	}
	return fileDone;
}

shared_ptr<AudioBlock> AudioFile::getNextAudioBlockAux()
{
	shared_ptr<AudioBlock> newBlock = make_shared<AudioBlock>();
	newBlock->initializeChannels();
	bool gotFrame = false;
	bool fileDone, oldFileDone;
	{
		lock_guard<mutex> lck(m_FileDoneMux);
		fileDone = m_FileDone;
	}
	oldFileDone = fileDone;
	while (!fileDone && !gotFrame)
	{
#ifdef USE_RESAMPLER
		fileDone = Decode();
		if (!fileDone)
		{
			int destNumSamples;
			int maxDestNumSamples = m_MaxDestNumSamples;
			bool goOn = true;

			AVCodecContext * codecContainer =
					m_Container->streams[m_StreamId]->codec;
			int srcSampRate = codecContainer->sample_rate;
			int srcNumSamples = m_DecodedFrame->nb_samples;

			destNumSamples = av_rescale_rnd(
					swr_get_delay(m_SwrContext, srcSampRate) + srcNumSamples,
					m_DestSampRate, srcSampRate, AV_ROUND_UP);
			if (destNumSamples > maxDestNumSamples)
			{
				av_freep(&m_DestData[0]);
				if (av_samples_alloc(m_DestData, nullptr,
						m_DestNumChannels, destNumSamples,
						m_DestSampFmt, 0) < 0)
				{
					goOn = false;
				}
				else
				{
					maxDestNumSamples = destNumSamples;
				}
			}

			if (goOn && swr_convert(m_SwrContext, m_DestData, destNumSamples,
					(const uint8_t **) m_DecodedFrame->extended_data,
					srcNumSamples) >= 0)
			{
				m_MaxDestNumSamples = maxDestNumSamples;
				gotFrame = true;
				double timeDelta = destNumSamples / ((double) m_DestSampRate);
				{
					lock_guard<mutex> lck(m_PositionMutex);
					m_Position += m_PrevDelta;
				}
				m_PrevDelta = timeDelta;
				for (int ch = 0; ch < m_DestNumChannels; ++ch)
				{
					newBlock->setChannelData(ch, (float *) m_DestData[ch],
							destNumSamples);
				}
			}
		}
#else
		fileDone = Decode();
		if (!fileDone)
		{
			gotFrame = true;
			double timeDelta = destNumSamples / ((double) m_DestSampRate);
			newBlock->setTimeDelta(timeDelta);
			{
				lock_guard<mutex> lck(m_PositionMutex);
				m_Position += timeDelta;
			}

			AVCodecContext * codecContainer =
					m_Container->streams[m_StreamId]->codec;
			int srcNumSamples = m_DecodedFrame->nb_samples;
			for (int ch = 0; ch < codecContainer->channels; ++ch)
			{
				newBlock->pushChannelData(
						(int16_t *) m_DecodedFrame->extended_data[ch],
						srcNumSamples);
			}
		}
#endif
	}
	if (oldFileDone != fileDone)
	{
		lock_guard<mutex> lck(m_FileDoneMux);
		m_FileDone = fileDone;
	}
	return newBlock;
}

shared_ptr<AudioBlock> AudioFile::getNextAudioBlock()
{
	// This function handles negative position values (i.e., silence
	// before the start of the file).
	shared_ptr<AudioBlock> newBlock;
	double position;
	bool negPosition;
	{
		lock_guard<mutex> lck(m_PositionMutex);
		position = m_Position;
		negPosition = m_NegPosition;
	}
	if (negPosition)
	{
		newBlock = make_shared<AudioBlock>();
		std::vector<float> blankSample;
		size_t framesToStart = (size_t) (-position * m_DestSampRate);
		size_t frameSize = std::min((size_t) 1024, framesToStart);
		blankSample.resize(frameSize);
		for (int ch = 0; ch < m_DestNumChannels; ++ch)
		{
			newBlock->pushChannelData(blankSample.data(), frameSize);
		}

		{
			lock_guard<mutex> lck(m_PositionMutex);
			if (frameSize == framesToStart)
			{
				// i.e., frameSize <= 1024
				m_NegPosition = false;
			}
			m_Position += frameSize / ((double) m_DestSampRate);
		}
	}
	else
	{
		newBlock = getNextAudioBlockAux();
	}
	return newBlock;
}

bool AudioFile::seek(double newPosition)
{
	bool rv = false;

	if (newPosition < 0.0)
	{
		m_PrevDelta = 0.0;
		rv = true;

		std::lock_guard<std::mutex> lck(m_PositionMutex);
		m_NegPosition = true;
		m_Position = newPosition;
	}
	else
	{
		// I don't trust av_seek_frame because it uses DTS instead of PTS.
		bool fileDone;
		{
			lock_guard<mutex> lck(m_FileDoneMux);
			fileDone = m_FileDone;
		}
		if (!fileDone)
		{
			bool oldFileDone = fileDone;
			double position;
			{
				std::lock_guard<std::mutex> lck(m_PositionMutex);
				position = m_Position;
			}
			{
				lock_guard<mutex> lck(m_FileDoneMux);
				m_FileDone = false;
			}

			AVCodecContext * codecContainer =
					m_Container->streams[m_StreamId]->codec;
			int srcSampRate = codecContainer->sample_rate;
			while (position < newPosition && !Decode())
			{
				position += m_DecodedFrame->nb_samples / ((double) srcSampRate);
			}
			if (position < newPosition)
			{
				lock_guard<mutex> lck(m_FileDoneMux);
				fileDone = true;
			}
			else
			{
				m_PrevDelta = 0.0;
				rv = true;

				lock_guard<mutex> lck(m_PositionMutex);
				m_NegPosition = false;
				m_Position = position;
			}
			if (oldFileDone != fileDone)
			{
				lock_guard<mutex> lck(m_FileDoneMux);
				m_FileDone = fileDone;
			}
		}
	}
	return rv;
}
