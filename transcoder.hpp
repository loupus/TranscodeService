#pragma once
extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
//#include "libavutil\avutil.h"
//#include "libavutil\avassert.h"
//#include "libavutil\avconfig.h"
//#include "libavutil\samplefmt.h"
#include "libavutil\opt.h"
//#include "libavutil\time.h"
#include "libavutil\avstring.h"
#include "libavutil\imgutils.h"
//#include "libavfilter\buffersink.h"
//#include "libavfilter\buffersrc.h"
#include "libswscale\swscale.h"
//#include "libswresample\swresample.h"
}

#include <queue>
#include <pthread.h>

#include <mutex>
#include "Globals.hpp"
#include "TranscodeCallback.h"





typedef struct AlaContextStruct
{
	std::string assetid;
	std::string inFile;
	std::string outFile;

	int VideoDecStreamIndex = -1;
	int	AudioDecStreamIndex = -1;
	int VideoEncStreamIndex = -1;
	int	AudioEncStreamIndex = -1;

	AVFormatContext* DecFormCtx = nullptr;
	AVFormatContext* EncFormCtx = nullptr;

	AVStream* VideoEncStream = nullptr;
	AVStream* AudioEncStream = nullptr;
	AVStream* VideoDecStream = nullptr;
	AVStream* AudioDecStream = nullptr;

	AVCodecContext* VideoDecCtx = nullptr;
	AVCodecContext* AudioDecCtx = nullptr;
	AVCodecContext* VideoEncCtx = nullptr;
	AVCodecContext* AudioEncCtx = nullptr;

	AVCodec* VideoEncCodec = nullptr;
	AVCodec* AudioEncCodec = nullptr;

	//AVRational VideoDecSampleAspectRatio;

	AVCodecID VideoEncCodecID = AV_CODEC_ID_NONE;
	int VideoEncWidth = -1;
	int VideoEncHeight = -1;
	int64_t VideoEncBitrate = -1;
	AVPixelFormat VideoEncPixelFormat = AV_PIX_FMT_NONE;
	AVRational VideoEncSampleAspectRatio;
	AVRational VideoEncFrameRate;

	AVCodecID AudioEncCodecID = AV_CODEC_ID_NONE;
	int AudioEncOutputChannels = -1;
	int AudioEncBitrate = -1;
	AVSampleFormat AudioEncSampleFormat = AV_SAMPLE_FMT_NONE;
	AVRational AudioEncTimeBase;
	uint64_t AudioChannelLayout;

	AVStream* EncStream = nullptr;
	AVStream* DecStream = nullptr;
	AVCodecContext* DecCtx = nullptr;
	AVCodecContext* EncCtx = nullptr;
	int OutStreamIndex = -1;
	AVMediaType MediaType = AVMEDIA_TYPE_UNKNOWN;

	struct SwsContext* sws_ctx = nullptr;

	void switchEncoder(AVMediaType mt)
	{
		if (mt == AVMEDIA_TYPE_VIDEO)
		{
			EncStream = VideoEncStream;
			DecStream = VideoDecStream;
			DecCtx = VideoDecCtx;
			EncCtx = VideoEncCtx;
			OutStreamIndex = VideoEncStreamIndex;
			MediaType = AVMEDIA_TYPE_VIDEO;
		}
		else if(mt == AVMEDIA_TYPE_AUDIO)
		{
			EncStream = AudioEncStream;
			DecStream = AudioDecStream;
			DecCtx = AudioDecCtx;
			EncCtx = AudioEncCtx;
			OutStreamIndex = AudioEncStreamIndex;
			MediaType = AVMEDIA_TYPE_AUDIO;
		}
	}

}AlaContext;

typedef struct RetValStruct
{
	int ReadFrame = 0;
	int WriteFrame = 0;
	int DecoderSend = 0;
	int DecoderReceive = 0;
	int EncoderSend = 0;
	int EncoderReceive = 0;
	int SwScaleFrame = 0;
}RetVal;


typedef struct TSettingsStruct
{
	std::string assetid;
	std::string infile;
	std::string outfile;
	int width=0;
	int height=0;
	int VbitrateKb=0;
	int AbitrateKb=0;
	TSettingsStruct(){};
}TSettings;


class Transcoder
{
    public:
	Transcoder();
	~Transcoder();
	BackObject DoTransCode();
	void CommitSettings(TSettings pSettings);
	void SetCallBack(ITranscoderCallBack* pcb);
	BackObject TranscodeAS(TSettings pSettings);
	void TranscodeASv(TSettings pSettings, ITranscoderCallBack* pCallback);
	void AddItem(TSettings &pItem);
	void Start();
	void Stop();

    private:
	AlaContext ctx;
	std::queue<TSettings> tq;
	bool StopFlag = false;
	pthread_t thHandle;
	pthread_attr_t thAttr;
	std::mutex mtx;
	TSettings GetFront();
	void PopItem();
	int GetQueueSize();
	
	//std::thread TranscodeThreadHandle;

	ITranscoderCallBack* callback = nullptr;
    //void handle_error(int ret, const char* msg);
   // void CommitSettings(AlaContext* pCtx);
    int check_sample_fmt(AVCodec* codec, enum AVSampleFormat sample_fmt);
    int select_sample_rate(AVCodec* codec);
    BackObject open_input_file(AlaContext* ctx);
    BackObject add_video_stream(AlaContext* ctx);
    BackObject add_audio_stream(AlaContext* ctx);
    BackObject add_audio_stream_clone(AlaContext* ctx);
    BackObject set_muxer(AlaContext* ctx);
    int init_VideoRescaleContext(AlaContext* ctx);
    int drain(AlaContext* ctx);
    int freeCtx(AlaContext* ctx);
    BackObject transcode_loop(AlaContext* ctx);
	void FireCallBack(TranscoderCBArgument pArg);
	static void* Alomelo(void*arg);
	
	


	

	

};