
#include <iostream>
#include <sstream>
#include "color.hpp"
#include "transcoder.hpp"

Transcoder::Transcoder()
{
	std::cout << "transcoder object created" << std::endl;
}

Transcoder::~Transcoder()
{
	std::cout << "transcoder object deleted" << std::endl;
	Stop();
	std::queue<TSettings>  empty;
	std::swap(tq,empty);
	if(callback)
	{
		delete callback;
		callback = nullptr;
	}
}

void Transcoder::AddItem(TSettings &pItem)
{

	std::unique_lock<std::mutex> lock(mtx);
	tq.push(pItem);
	lock.unlock();


}

TSettings Transcoder::GetFront()
	{
		TSettings back;
		std::unique_lock<std::mutex> lock(mtx);
		if(!tq.empty())
			back = tq.front();
		lock.unlock();
		return back;
	}

int Transcoder::GetQueueSize()
{
	int back = 0;
	std::unique_lock<std::mutex> lock(mtx);
	back = tq.size();
	lock.unlock();
	return back;
}

void Transcoder::PopItem()
{
	std::unique_lock<std::mutex> lock(mtx);
	tq.pop();
	lock.unlock();
}

void Transcoder::CommitSettings(TSettings pSettings)
{
	ctx.assetid = pSettings.assetid;
	ctx.inFile = pSettings.infile.c_str();
	ctx.outFile = pSettings.outfile.c_str();
	ctx.VideoEncCodecID = AV_CODEC_ID_H264;
	ctx.VideoEncWidth = pSettings.width > 0 ? pSettings.width : 480;
	ctx.VideoEncHeight = pSettings.height > 0 ? pSettings.height : 270;
	ctx.VideoEncBitrate = pSettings.VbitrateKb > 0 ? pSettings.VbitrateKb * 1000 : 1000 * 1000; // 1000 kilobit /s
	ctx.VideoEncPixelFormat = AV_PIX_FMT_YUV420P;
	ctx.VideoEncFrameRate.num = 25;
	ctx.VideoEncFrameRate.den = 1;
	ctx.AudioEncCodecID = AV_CODEC_ID_AAC;
	ctx.AudioEncOutputChannels = 2;
	ctx.AudioEncBitrate = pSettings.AbitrateKb > 0 ? pSettings.AbitrateKb * 1000 : 1000 * 196; // 196 kilobit /s
	ctx.AudioEncSampleFormat = AV_SAMPLE_FMT_FLTP;
	ctx.AudioChannelLayout = AV_CH_LAYOUT_STEREO;
}

int Transcoder::check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat *p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE)
	{
		if (*p == sample_fmt)
			return 1;
		p++;
	}
	return 0;
}

int Transcoder::select_sample_rate(AVCodec *codec)
{
	const int *p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return 44100;

	p = codec->supported_samplerates;
	while (*p)
	{
		best_samplerate = FFMAX(*p, best_samplerate);
		p++;
	}
	return best_samplerate;
}

BackObject Transcoder::open_input_file(AlaContext *ctx)
{
	BackObject back;
	int ret;
	unsigned int i;

	// ***************** Decoder **************/////////////////////////////
	ctx->DecFormCtx = avformat_alloc_context();
	if (!ctx->DecFormCtx)
	{
		back.ErrDesc = "Decoding Format Context cannot be allocated";
		back.Success = false;
		return back;
	}

	ret = avformat_open_input(&ctx->DecFormCtx, ctx->inFile.c_str(), NULL, NULL);
	if (ret < 0)
	{
		back.ErrDesc = "Cannot open input file";
		back.Success = false;
		return back;
	}

	ret = avformat_find_stream_info(ctx->DecFormCtx, NULL);
	if (ret < 0)
	{
		back.ErrDesc = "Cannot find decoding stream information";
		back.Success = false;
		return back;
	}

	for (i = 0; i < ctx->DecFormCtx->nb_streams; i++)
	{
		AVStream *stream = ctx->DecFormCtx->streams[i];
		AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
		AVCodecContext *codec_ctx;
		if (!dec)
		{
			back.ErrDesc = "Failed to find decoder for stream";
			back.Success = false;
			return back;
		}
		codec_ctx = avcodec_alloc_context3(dec);
		if (!codec_ctx)
		{
			back.ErrDesc = "Failed to allocate the decoder context for stream";
			back.Success = false;
			return back;
		}

		ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
		if (ret < 0)
		{
			back.ErrDesc = "Failed to copy decoder parameters to input decoder context for stream";
			back.Success = false;
			return back;
		}

		if (ctx->DecFormCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && ctx->VideoDecStreamIndex < 0)
		{
			/* Open decoder */
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0)
			{
				back.ErrDesc = "Failed to open decoder for stream";
				back.Success = false;
				return back;
			}
			ctx->VideoDecStreamIndex = i;
			ctx->VideoDecCtx = codec_ctx;
			ctx->VideoDecStream = ctx->DecFormCtx->streams[i];
		}

		if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO && ctx->AudioDecStreamIndex < 0)
		{
			/* Open decoder */
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0)
			{
				back.ErrDesc = "Failed to open decoder for stream";
				back.Success = false;
				return back;
			}
			ctx->AudioDecStreamIndex = i;
			ctx->AudioDecCtx = codec_ctx;
			ctx->AudioDecStream = ctx->DecFormCtx->streams[i];
		}
	}

	av_dump_format(ctx->DecFormCtx, 0, ctx->inFile.c_str(), 0);

	return back;
}

BackObject Transcoder::add_video_stream(AlaContext *ctx)
{
	BackObject back;
	int ret = 0;
	//****** Video Encoder *************************************************
	ctx->VideoEncCodec = avcodec_find_encoder(ctx->VideoEncCodecID);
	if (!(ctx->VideoEncCodec))
	{
		back.ErrDesc = "Could not find the proper video encoding codec";
		back.Success = false;
		return back;
	}

	ctx->VideoEncStream = avformat_new_stream(ctx->EncFormCtx, ctx->VideoEncCodec);
	if (!(ctx->VideoEncStream))
	{
		back.ErrDesc = "Could not create video stream";
		back.Success = false;
		return back;
	}
	ctx->VideoEncStreamIndex = 0;

	ctx->VideoEncCtx = avcodec_alloc_context3(ctx->VideoEncCodec);
	if (!(ctx->VideoEncCtx))
	{
		back.ErrDesc = "Could not create video encoding context";
		back.Success = false;
		return back;
	}

	ctx->VideoEncCtx->height = ctx->VideoEncHeight;
	ctx->VideoEncCtx->width = ctx->VideoEncWidth;
	ctx->VideoEncCtx->bit_rate = ctx->VideoEncBitrate;
	ctx->VideoEncCtx->rc_max_rate = ctx->VideoEncBitrate;
	ctx->VideoEncCtx->rc_buffer_size = 2 * ctx->VideoEncBitrate;
	ctx->VideoEncCtx->qmin = 10;
	ctx->VideoEncCtx->qmax = 51;

	//	ctx->VideoEncCtx->rc_max_rate = ctx->VideoEncBitrate;

	if (ctx->VideoEncCodec->pix_fmts)
		ctx->VideoEncCtx->pix_fmt = ctx->VideoEncCodec->pix_fmts[0];
	else
		ctx->VideoEncCtx->pix_fmt = ctx->VideoEncPixelFormat;

	ctx->VideoEncCtx->sample_aspect_ratio = ctx->VideoDecCtx->sample_aspect_ratio; // pixel aspect ratio, display degil

	ctx->VideoEncCtx->time_base = av_inv_q(ctx->VideoEncFrameRate);
	ctx->VideoEncStream->time_base = ctx->VideoEncCtx->time_base;

	//*************
	if (ctx->VideoEncCodecID == AV_CODEC_ID_H264)
	{
		av_opt_set(ctx->VideoEncCtx->priv_data, "preset", "fast", 0);
		av_opt_set(ctx->VideoEncCtx->priv_data, "profile", "main", 0);
	}

	//************

	ret = avcodec_open2(ctx->VideoEncCtx, ctx->VideoEncCodec, NULL);
	if (ret < 0)
	{
		back.ErrDesc = "Could not open video encoding codec";
		back.Success = false;
		return back;
	}

	ret = avcodec_parameters_from_context(ctx->VideoEncStream->codecpar, ctx->VideoEncCtx);
	if (ret < 0)
	{
		back.ErrDesc = "Could not transfer video encoding parameters from stream context";
		back.Success = false;
		return back;
	}

	av_dump_format(ctx->EncFormCtx, 0, ctx->outFile.c_str(), 1);

	return back;
}

BackObject Transcoder::add_audio_stream(AlaContext *ctx)
{
	BackObject back;
	int ret = 0;
	//****** Audio Encoder *************************************************
	ctx->AudioEncCodec = avcodec_find_encoder(ctx->AudioEncCodecID);
	if (!ctx->AudioEncCodec)
	{
		back.ErrDesc = "Could not find the proper audio encoding codec";
		back.Success = false;
		return back;
	}

	ctx->AudioEncStream = avformat_new_stream(ctx->EncFormCtx, ctx->AudioEncCodec);
	if (!(ctx->AudioEncStream))
	{
		back.ErrDesc = "Could not create video stream";
		back.Success = false;
		return back;
	}

	ctx->AudioEncStreamIndex = 1;

	ctx->AudioEncCtx = avcodec_alloc_context3(ctx->AudioEncCodec);
	if (!(ctx->AudioEncCtx))
	{
		back.ErrDesc = "Could not create audio encoding context";
		back.Success = false;
		return back;
	}

	ctx->AudioEncCtx->channels = ctx->AudioEncOutputChannels;
	// ctx->AudioEncCtx->channel_layout = av_get_default_channel_layout(ctx->AudioEncOutputChannels);
	ctx->AudioEncCtx->channel_layout = ctx->AudioChannelLayout;
	ctx->AudioEncCtx->sample_rate = select_sample_rate(ctx->AudioEncCodec);
	if (ctx->AudioEncCodec->sample_fmts)
		ctx->AudioEncCtx->sample_fmt = ctx->AudioEncCodec->sample_fmts[0];
	else
		ctx->AudioEncCtx->sample_fmt = ctx->AudioEncSampleFormat;

	if (!check_sample_fmt(ctx->AudioEncCodec, ctx->AudioEncCtx->sample_fmt))
	{
		back.ErrDesc = "Audio encoder does not support sample format";
		back.Success = false;
		return back;
	}
	ctx->AudioEncCtx->bit_rate = ctx->AudioEncBitrate;

	ctx->AudioEncTimeBase.num = 1;
	ctx->AudioEncTimeBase.den = ctx->AudioEncCtx->sample_rate;
	ctx->AudioEncCtx->time_base = ctx->AudioEncTimeBase;
	ctx->AudioEncStream->time_base = ctx->AudioEncCtx->time_base;

	ret = avcodec_open2(ctx->AudioEncCtx, ctx->AudioEncCodec, NULL);
	if (ret < 0)
	{
		back.ErrDesc = "Could not open audio encoding codec";
		back.Success = false;
		return back;
	}

	ret = avcodec_parameters_from_context(ctx->AudioEncStream->codecpar, ctx->AudioEncCtx);
	if (ret < 0)
	{
		back.ErrDesc = "Could not transfer audio encoding parameters from stream context";
		back.Success = false;
		return back;
	}

	av_dump_format(ctx->EncFormCtx, 1, ctx->outFile.c_str(), 1);

	return back;
}

BackObject Transcoder::add_audio_stream_clone(AlaContext *ctx)
{
	BackObject back;
	int ret = 0;
	//****** Audio Encoder *************************************************
	ctx->AudioEncCodec = avcodec_find_encoder(ctx->AudioEncCodecID);
	if (!ctx->AudioEncCodec)
	{
		back.ErrDesc = "Could not find the proper audio encoding codec";
		back.Success = false;
		return back;
	}

	ctx->AudioEncStream = avformat_new_stream(ctx->EncFormCtx, ctx->AudioEncCodec);
	if (!(ctx->AudioEncStream))
	{
		back.ErrDesc = "Could not create video stream";
		back.Success = false;
		return back;
	}

	ctx->AudioEncStreamIndex = 1;

	ctx->AudioEncCtx = avcodec_alloc_context3(ctx->AudioEncCodec);
	if (!(ctx->AudioEncCtx))
	{
		back.ErrDesc = "Could not create audio encoding context";
		back.Success = false;
		return back;
	}

	ctx->AudioEncCtx->channels = ctx->AudioDecCtx->channels;
	ctx->AudioEncCtx->channel_layout = av_get_default_channel_layout(ctx->AudioEncOutputChannels);
	ctx->AudioEncCtx->sample_rate = ctx->AudioDecCtx->sample_rate;
	ctx->AudioEncCtx->sample_fmt = ctx->AudioDecCtx->sample_fmt;

	if (!check_sample_fmt(ctx->AudioEncCodec, ctx->AudioEncCtx->sample_fmt))
	{
		back.ErrDesc = "Audio encoder does not support sample format";
		back.Success = false;
		return back;
	}
	ctx->AudioEncCtx->bit_rate = ctx->AudioEncBitrate;

	ctx->AudioEncCtx->time_base = ctx->AudioDecCtx->time_base;
	ctx->AudioEncStream->time_base = ctx->AudioEncCtx->time_base;

	ret = avcodec_open2(ctx->AudioEncCtx, ctx->AudioEncCodec, NULL);
	if (ret < 0)
	{
		back.ErrDesc = "Could not open audio encoding codec";
		back.Success = false;
		return back;
	}

	ret = avcodec_parameters_from_context(ctx->AudioEncStream->codecpar, ctx->AudioEncCtx);
	if (ret < 0)
	{
		back.ErrDesc = "Could not transfer audio encoding parameters from stream context";
		back.Success = false;
		return back;
	}

	av_dump_format(ctx->EncFormCtx, 1, ctx->outFile.c_str(), 1);

	return back;
}

BackObject Transcoder::set_muxer(AlaContext *ctx)
{
	BackObject back;
	int ret = 0;
	// MUXER
	/* Some formats want stream headers to be separate. */
	if (ctx->EncFormCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		ctx->VideoEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		ctx->AudioEncCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (!(ctx->EncFormCtx->oformat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&ctx->EncFormCtx->pb, ctx->outFile.c_str(), AVIO_FLAG_WRITE) < 0)
		{
			back.ErrDesc = "could not open the output file";
			back.Success = false;
			return back;
		}
	}

	ret = avformat_write_header(ctx->EncFormCtx, NULL);
	if (ret < 0)
	{
		char buf[1024] = {0};
		av_strerror(ret, buf, 1024);
		std::ostringstream dtss;
		dtss << "Error occurred when opening output file: " << buf << std::endl;

		if (ret == AVSTREAM_INIT_IN_WRITE_HEADER || ret == AVSTREAM_INIT_IN_INIT_OUTPUT)
			dtss << "Could not write header" << std::endl;

		back.ErrDesc = dtss.str();
		back.Success = false;
		return back;
	}
	return back;
}

int Transcoder::init_VideoRescaleContext(AlaContext *ctx)
{
	ctx->sws_ctx = sws_getContext(ctx->VideoDecCtx->width, ctx->VideoDecCtx->height, ctx->VideoDecCtx->pix_fmt, ctx->VideoEncCtx->width, ctx->VideoEncCtx->height, ctx->VideoEncCtx->pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);

	if (!ctx->sws_ctx)
	{
		return -1;
	}
	else
		return 0;
}

int Transcoder::drain(AlaContext *ctx)
{
	std::cout << "Draining buffers..." << std::endl;
	RetVal ret;
	AVFrame *input_frame = nullptr;

	AVPacket *pkt = av_packet_alloc();
	if (!pkt)
	{
		std::cout << "cannot alloc output packet" << std::endl;
		return -1;
	}

	ret.EncoderSend = avcodec_send_frame(ctx->EncCtx, input_frame);
	if (ret.EncoderSend < 0)
	{
		std::cout << "avcodec_send_frame failed" << std::endl;
		return -1;
	}
	ret.EncoderReceive = 0;
	while (ret.EncoderReceive >= 0)
	{
		ret.EncoderReceive = avcodec_receive_packet(ctx->EncCtx, pkt);
		if (ret.EncoderReceive == AVERROR(EAGAIN) || ret.EncoderReceive == AVERROR_EOF)
			break; // encodera yeni frame gonder
		if (ret.EncoderReceive < 0)
		{
			std::cout << "avcodec_receive_packet failed" << std::endl;
			return -1;
		}

		// writer
		pkt->stream_index = ctx->OutStreamIndex;
		av_packet_rescale_ts(pkt, ctx->DecStream->time_base, ctx->EncStream->time_base);
		ret.WriteFrame = av_interleaved_write_frame(ctx->EncFormCtx, pkt);
		if (ret.WriteFrame < 0)
		{
			std::cout << "av_interleaved_write_frame failed" << std::endl;
			return -1;
		}
	}
	av_packet_unref(pkt);
	av_packet_free(&pkt);

	return av_write_trailer(ctx->EncFormCtx);
}

int Transcoder::freeCtx(AlaContext *ctx)
{

	std::cout << "Releasing resources..." << std::endl;
	avformat_close_input(&ctx->DecFormCtx);

	if (ctx->AudioDecCtx)
		avcodec_free_context(&ctx->AudioDecCtx);
	ctx->AudioDecCtx = nullptr;
	if (ctx->VideoDecCtx)
		avcodec_free_context(&ctx->VideoDecCtx);
	ctx->VideoDecCtx = nullptr;

	if (ctx->AudioEncCtx)
		avcodec_free_context(&ctx->AudioEncCtx);
	ctx->AudioEncCtx = nullptr;
	if (ctx->VideoEncCtx)
		avcodec_free_context(&ctx->VideoEncCtx);
	ctx->VideoEncCtx = nullptr;

	if (ctx->DecFormCtx)
		avformat_free_context(ctx->DecFormCtx);
	ctx->DecFormCtx = nullptr;
	if (ctx->EncFormCtx)
		avformat_free_context(ctx->EncFormCtx);
	ctx->EncFormCtx = nullptr;

	// if (ctx->DecCtx) avcodec_free_context(&ctx->DecCtx);
	ctx->DecCtx = nullptr;
	// if (ctx->EncCtx) avcodec_free_context(&ctx->EncCtx);
	ctx->EncCtx = nullptr;

	if (ctx->sws_ctx)
		sws_freeContext(ctx->sws_ctx);
	ctx->sws_ctx = nullptr;

	ctx->VideoDecStreamIndex = -1;
	ctx->AudioDecStreamIndex = -1;
	ctx->VideoEncStreamIndex = -1;
	ctx->AudioEncStreamIndex = -1;
	ctx->VideoEncWidth = -1;
	ctx->VideoEncHeight = -1;
	ctx->VideoEncBitrate = -1;
	ctx->AudioEncOutputChannels = -1;
	ctx->AudioEncBitrate = -1;
	ctx->OutStreamIndex = -1;

	//delete ctx;
	return 0;
}

BackObject Transcoder::transcode_loop(AlaContext *ctx)
{
	BackObject back;
	RetVal ret;
	int rv = 0;


	AVFrame *input_frame = av_frame_alloc();
	if (!input_frame)
	{
		back.ErrDesc = "cannot alloc input frame";
		back.Success = false;
		return back;
	}

	AVPacket *input_packet = av_packet_alloc();
	if (!input_packet)
	{
		back.ErrDesc = "cannot alloc input packet";
		back.Success = false;
		return back;
	}

	rv = init_VideoRescaleContext(ctx);
	if (rv < 0)
	{
		back.ErrDesc = "cannot init sws context";
		back.Success = false;
		return back;
	}

	AVFrame *frameRGB = av_frame_alloc();
	if (!frameRGB)
	{
		back.ErrDesc = "cannot alloc frame";
		back.Success = false;
		return back;
	}

	AVPacket *outPacket = av_packet_alloc();
	if (!outPacket)
	{
		back.ErrDesc = "cannot alloc output packet";
		back.Success = false;
		return back;
	}

	rv = av_image_alloc(frameRGB->data, frameRGB->linesize, ctx->VideoEncWidth, ctx->VideoEncHeight, ctx->VideoEncPixelFormat, 1);
	if (rv < 0)
	{
		back.ErrDesc = "cannot alloc rgb frame";
		back.Success = false;
		return back;
	}

	// int bufSize = av_image_get_buffer_size(ctx->VideoEncPixelFormat, ctx->VideoEncWidth, ctx->VideoEncHeight, 1);
	// uint8_t* buf = (uint8_t*)av_malloc(bufSize);
	// int rr =av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buf, ctx->VideoEncPixelFormat, ctx->VideoEncWidth, ctx->VideoEncHeight, 1);

		std::cout << "Transcoding InputFile:  " << ctx->inFile << std::endl;

	while (1)
	{

		ret.ReadFrame = av_read_frame(ctx->DecFormCtx, input_packet);
		if (ret.ReadFrame < 0)
		{
			std::cout << std::endl;
			drain(ctx);
			
			goto cikis;
			//	handle_error(ret.ReadFrame, "cannot read frame or end of file");
		}

		if (input_packet->stream_index == ctx->VideoDecStreamIndex)
		{
			ctx->switchEncoder(AVMEDIA_TYPE_VIDEO);
		}
		else if (input_packet->stream_index == ctx->AudioDecStreamIndex)
		{
			ctx->switchEncoder(AVMEDIA_TYPE_AUDIO);
		}
		else
		{
			std::cout << "ne video ne audio, sal gitsin" << std::endl;
			continue;
		}

		// decoder
		ret.DecoderSend = avcodec_send_packet(ctx->DecCtx, input_packet);
		av_packet_unref(input_packet);
		if (ret.DecoderSend < 0)
		{
			std::cout << std::endl;
			back.ErrDesc = "avcodec_send_packet failed";
			back.Success = false;
			return back;
		}
		ret.DecoderReceive = 0;
		while (ret.DecoderReceive >= 0)
		{
			ret.DecoderReceive = avcodec_receive_frame(ctx->DecCtx, input_frame);
			if (ret.DecoderReceive == AVERROR(EAGAIN) || ret.DecoderReceive == AVERROR_EOF)
				break; // decodere yeni paket gonder
			if (ret.DecoderReceive < 0)
			{
				back.ErrDesc = "avcodec_receive_frame failed";
				back.Success = false;
				goto cikis;
			}

			// rescale video
			if (ctx->MediaType == AVMEDIA_TYPE_VIDEO)
			{
				ret.SwScaleFrame = sws_scale(ctx->sws_ctx,
											 input_frame->data,
											 input_frame->linesize,
											 0,
											 ctx->VideoDecCtx->height,
											 (uint8_t *const *)frameRGB->data,
											 (const int *)frameRGB->linesize);
				if (ret.SwScaleFrame <= 0)
				{
					back.ErrDesc = "sws_scale failed";
					back.Success = false;
					goto cikis;
				}

				frameRGB->format = ctx->VideoEncPixelFormat;
				frameRGB->width = ctx->VideoEncWidth;
				frameRGB->height = ctx->VideoEncHeight;
				frameRGB->pts = input_frame->pts;
			}

			AVFrame *tempFrame = nullptr;

			////encoder
			if (ctx->MediaType == AVMEDIA_TYPE_VIDEO)
			{
				if (input_frame)
					input_frame->pict_type = AV_PICTURE_TYPE_NONE;
				tempFrame = frameRGB;
			}
			else
				tempFrame = input_frame;

			// if(outPacket)
			AVPacket *pkt = av_packet_alloc();
			if (!pkt)
			{
				back.ErrDesc = "cannot alloc output packet";
				back.Success = false;
				goto cikis;
			}

			ret.EncoderSend = avcodec_send_frame(ctx->EncCtx, tempFrame);

			av_frame_unref(input_frame);

			if (ret.EncoderSend < 0)
			{
				back.ErrDesc = "avcodec_send_frame failed";
				back.Success = false;
				goto cikis;
			}
			ret.EncoderReceive = 0;
			std::cout.flush();
			while (ret.EncoderReceive >= 0)
			{
				ret.EncoderReceive = avcodec_receive_packet(ctx->EncCtx, pkt);
				if (ret.EncoderReceive == AVERROR(EAGAIN) || ret.EncoderReceive == AVERROR_EOF)
					break; // encodera yeni frame gonder
				if (ret.EncoderReceive < 0)
				{
					back.ErrDesc = "avcodec_receive_packet failed";
					back.Success = false;
					goto cikis;
				}

				// av_frame_unref(input_frame);
				// av_frame_unref(frameRGB);

				// writer
				pkt->stream_index = ctx->OutStreamIndex;

				/*	if (ctx->MediaType == AVMEDIA_TYPE_VIDEO)
					{
						pkt->duration = ctx->EncStream->time_base.den / ctx->EncStream->time_base.num / ctx->DecStream->avg_frame_rate.num * ctx->DecStream->avg_frame_rate.den;
					}*/
				av_packet_rescale_ts(pkt, ctx->DecStream->time_base, ctx->EncStream->time_base);
				ret.WriteFrame = av_interleaved_write_frame(ctx->EncFormCtx, pkt);
				av_packet_unref(pkt);

/*
				std::cout << "\r"
						  << "Encoded Video Frame: " << ctx->EncFormCtx->streams[0]->nb_frames << "			Decoded Video Frame: " << decFrame;
						  */
			//	std::cout << "\r" << "Encoded Frame:" << ctx->EncFormCtx->streams[0]->nb_frames << std::flush;

				if (ret.WriteFrame < 0) // handle_error(ret.WriteFrame, "av_interleaved_write_frame failed");
				{
					back.ErrDesc = "av_interleaved_write_frame failed";
					back.Success = false;
					goto cikis;
				}
			}

		
			av_packet_free(&pkt);
		}

		//_CrtDumpMemoryLeaks();
	}

	cikis:
	if(back.Success)
		std::cout << "Transcoding finished with success. Outfile: " << ctx->outFile << std::endl;
	else
		std::cout << "Transcoding finished with fail. Errdesc: " << back.ErrDesc << std::endl;

	if (frameRGB)
		av_frame_free(&frameRGB);
	if (input_frame)
		av_frame_free(&input_frame);

	if (input_packet)
		av_packet_free(&input_packet);
	if (outPacket)
		av_packet_free(&outPacket);

	return back;
}

BackObject Transcoder::DoTransCode()
{
	BackObject back;
	int rv = 0;
	TranscoderCBArgument cba;
	cba.AssetId = ctx.assetid;
	if (ctx.inFile.empty() || ctx.outFile.empty())
	{
		back.Success = false;
		back.ErrDesc = "Settings not received";
		cba.Success = false;
		cba.ErrMessage = "Settings not received";
		FireCallBack(cba);
		return back;
	}

	av_log_set_level(AV_LOG_ERROR);

	back = open_input_file(&ctx);
	if (!back.Success)
	{
		cba.Success = false;
		cba.ErrMessage = "Failed to open input file";
		FireCallBack(cba);
		return back;
	}

	rv = avformat_alloc_output_context2(&ctx.EncFormCtx, NULL, NULL, ctx.outFile.c_str());
	if (rv < 0 || !(ctx.EncFormCtx))
	{
		back.Success = false;
		back.ErrDesc = "avformat_alloc_output_context2 failed";
		cba.Success = false;
		cba.ErrMessage = "avformat_alloc_output_context2 failed";
		FireCallBack(cba);
		return back;
	}

	back = add_video_stream(&ctx);
	if (!back.Success)
	{
		cba.Success = false;
		cba.ErrMessage = back.ErrDesc;
		FireCallBack(cba);
		return back;
	}

	back = add_audio_stream_clone(&ctx);
	if (!back.Success)
	{
		cba.Success = false;
		cba.ErrMessage = back.ErrDesc;
		FireCallBack(cba);
		return back;
	}

	back = set_muxer(&ctx);
	if (!back.Success)
	{
		cba.Success = false;
		cba.ErrMessage = back.ErrDesc;
		FireCallBack(cba);
		return back;
	}

	back = transcode_loop(&ctx);
	if (!back.Success)
	{
		cba.Success = false;
		cba.ErrMessage = back.ErrDesc;
		FireCallBack(cba);
	}
	else
	{
		cba.Success = true;
		cba.ErrMessage = "";
		cba.ProxyFile = ctx.outFile;
		FireCallBack(cba);
	}

	rv = freeCtx(&ctx);

	return back;
}

void Transcoder::SetCallBack(ITranscoderCallBack *pcb)
{
	if(pcb)
		callback = pcb;
	else
	std::cout << "callback is empty" << std::endl;

}

void Transcoder::FireCallBack(TranscoderCBArgument pArg)
{
	if (callback)
		callback->Call(pArg);
	else
		std::cout << "no valid callbacks!" << std::endl;
}

BackObject Transcoder::TranscodeAS(TSettings pSettings)
{
	BackObject back;
	AlaContext ctx1;
	int rv = 0;

	ctx1.assetid = pSettings.assetid;
	ctx1.inFile = pSettings.infile.c_str();
	ctx1.outFile = pSettings.outfile.c_str();
	ctx1.VideoEncCodecID = AV_CODEC_ID_H264;
	ctx1.VideoEncWidth = pSettings.width > 0 ? pSettings.width : 480;
	ctx1.VideoEncHeight = pSettings.height > 0 ? pSettings.height : 270;
	ctx1.VideoEncBitrate = pSettings.VbitrateKb > 0 ? pSettings.VbitrateKb * 1000 : 1000 * 1000; // 1000 kilobit /s
	ctx1.VideoEncPixelFormat = AV_PIX_FMT_YUV420P;
	ctx1.VideoEncFrameRate.num = 25;
	ctx1.VideoEncFrameRate.den = 1;
	ctx1.AudioEncCodecID = AV_CODEC_ID_AAC;
	ctx1.AudioEncOutputChannels = 2;
	ctx1.AudioEncBitrate = pSettings.AbitrateKb > 0 ? pSettings.AbitrateKb * 1000 : 1000 * 196; // 196 kilobit /s
	ctx1.AudioEncSampleFormat = AV_SAMPLE_FMT_FLTP;
	ctx1.AudioChannelLayout = AV_CH_LAYOUT_STEREO;

	if (ctx1.inFile.empty() || ctx1.outFile.empty())
	{
		back.Success = false;
		back.ErrDesc = "Settings not received";
		return back;
	}

	av_log_set_level(AV_LOG_ERROR);

	back = open_input_file(&ctx1);
	if (!back.Success)
	{
		return back;
	}

	rv = avformat_alloc_output_context2(&ctx1.EncFormCtx, NULL, NULL, ctx1.outFile.c_str());
	if (rv < 0 || !(ctx1.EncFormCtx))
	{
		back.Success = false;
		back.ErrDesc = "avformat_alloc_output_context2 failed";
		return back;
	}

	back = add_video_stream(&ctx1);
	if (!back.Success)
	{
		return back;
	}

	back = add_audio_stream_clone(&ctx1);
	if (!back.Success)
	{
		return back;
	}

	back = set_muxer(&ctx1);
	if (!back.Success)
	{
		return back;
	}

	back = transcode_loop(&ctx1);
	rv = freeCtx(&ctx1);

	return back;
}

void Transcoder::TranscodeASv(TSettings pSettings, ITranscoderCallBack *pCallback)
{
	BackObject back;
	TranscoderCBArgument cbArg;
	cbArg.AssetId = pSettings.assetid;
	AlaContext ctx1;
	int rv = 0;

	ctx1.assetid = pSettings.assetid;
	ctx1.inFile = pSettings.infile.c_str();
	ctx1.outFile = pSettings.outfile.c_str();
	ctx1.VideoEncCodecID = AV_CODEC_ID_H264;
	ctx1.VideoEncWidth = pSettings.width > 0 ? pSettings.width : 480;
	ctx1.VideoEncHeight = pSettings.height > 0 ? pSettings.height : 270;
	ctx1.VideoEncBitrate = pSettings.VbitrateKb > 0 ? pSettings.VbitrateKb * 1000 : 1000 * 1000; // 1000 kilobit /s
	ctx1.VideoEncPixelFormat = AV_PIX_FMT_YUV420P;
	ctx1.VideoEncFrameRate.num = 25;
	ctx1.VideoEncFrameRate.den = 1;
	ctx1.AudioEncCodecID = AV_CODEC_ID_AAC;
	ctx1.AudioEncOutputChannels = 2;
	ctx1.AudioEncBitrate = pSettings.AbitrateKb > 0 ? pSettings.AbitrateKb * 1000 : 1000 * 196; // 196 kilobit /s
	ctx1.AudioEncSampleFormat = AV_SAMPLE_FMT_FLTP;
	ctx1.AudioChannelLayout = AV_CH_LAYOUT_STEREO;

	if (ctx1.inFile.empty() || ctx1.outFile.empty())
	{
		cbArg.Success = false;
		cbArg.ErrMessage = "Settings not received";
		return pCallback->Call(cbArg);
	}

	av_log_set_level(AV_LOG_ERROR);

	back = open_input_file(&ctx1);
	if (!back.Success)
	{
		cbArg.Success = false;
		cbArg.ErrMessage = back.ErrDesc;
		return pCallback->Call(cbArg);
	}

	rv = avformat_alloc_output_context2(&ctx1.EncFormCtx, NULL, NULL, ctx1.outFile.c_str());
	if (rv < 0 || !(ctx1.EncFormCtx))
	{
		cbArg.Success = false;
		cbArg.ErrMessage = "avformat_alloc_output_context2 failed";
		return pCallback->Call(cbArg);
	}

	back = add_video_stream(&ctx1);
	if (!back.Success)
	{
		cbArg.Success = false;
		cbArg.ErrMessage = back.ErrDesc;
		return pCallback->Call(cbArg);
	}

	back = add_audio_stream_clone(&ctx1);
	if (!back.Success)
	{
		cbArg.Success = false;
		cbArg.ErrMessage = back.ErrDesc;
		return pCallback->Call(cbArg);
	}

	back = set_muxer(&ctx1);
	if (!back.Success)
	{
		cbArg.Success = false;
		cbArg.ErrMessage = back.ErrDesc;
		return pCallback->Call(cbArg);
	}

	back = transcode_loop(&ctx1);
	rv = freeCtx(&ctx1);

	cbArg.Success = back.Success;
	cbArg.ErrMessage = back.ErrDesc;
	return pCallback->Call(cbArg);
}

void *Transcoder::Alomelo(void *arg)
{
	Transcoder *pObj = reinterpret_cast<Transcoder*>(arg);
	if(pObj == nullptr) return nullptr;
	std::cout << "Transcoder is starting..." << std::endl;
	int qsize = 0;
	TranscoderCBArgument targ;


	while (true)
	{
		std::cout << "while loop..." << std::endl;
		qsize = pObj->GetQueueSize();
		if (qsize>0)
		{
			//std::cout << "queue size var..." << std::endl;
			//TSettings anItem = pObj->tq.front();
			TSettings anItem = pObj->GetFront();
			if(anItem.infile.empty() || anItem.outfile.empty())
			{
				pObj->PopItem();
				continue;
			}
			//std::future<BackObject> res = std::async(std::launch::async,&Transcoder::TranscodeAS,this,anItem);
			//BackObject back = res.get();
			//std::cout << "will enter transcode..." << std::endl;
			BackObject back = pObj->TranscodeAS(anItem);
			
			targ.AssetId = anItem.assetid;

			if (back.Success)
			{
				targ.Success = true;
				targ.ProxyFile = anItem.outfile;
			}
			else
			{
				targ.Success = false;
				targ.ErrMessage = back.ErrDesc;
			}
			pObj->PopItem();
			pObj->FireCallBack(targ);
		}

		if (pObj->StopFlag)
			break;
		Sleep(30 * 1000);
		if (pObj->StopFlag)
			break;
	}

	std::cout << "Transcoder is stopping..." << std::endl;
   	pthread_exit(NULL);	
	return nullptr;
}

void Transcoder::Start()
{
	int rv  = 0;
	StopFlag = false;

	TranscoderCBArgument targ;
	FireCallBack(targ);


	rv =  pthread_create(&thHandle, nullptr, &Transcoder::Alomelo, (void *)this);
	if(rv != 0)
	{
		//std::cout << dye::red("ERR ===> Transcode thread create failed! HandleX:")  << thHandle.x << std::endl;
		std::cout <<  hue::red << "ERR ===> Transcode thread create failed! RV:" <<  hue::reset << std::endl; 
	}
	else
	{
			//std::cout << dye::blue("Thread: ")  << thHandle.x << std::endl;
			//std::cout << hue::blue << "Thread: " << thHandle.x << hue::reset << std::endl;
	}

	/*
	std::cout << "Thread ID: " <<  TranscodeThreadHandle.get_id() << " will start " << std::endl;
	StopFlag = false;
	TranscodeThreadHandle = std::thread(&Transcoder::Alomelo,this);	
	*/
}

void Transcoder::Stop()
{
	int rv = 0;
	void *res;
	StopFlag = true;
	//rv = pthread_cancel(thHandle);
	//if (rv != 0)
	//	std::cout << "	ERR ====> cancel thread failed" << std::endl;

	rv = pthread_join(thHandle, &res);
	if (rv != 0)
	{
			std::cout << dye::red("	ERR ====> join failed. ErrCode:") << rv << std::endl;
			
	}
		
	

	if (res == PTHREAD_CANCELED)
		std::cout << "thread joined" << std::endl;
	else
	{
		
		std::cout << dye::red( "ERR ====> thread join problem! RES:") << (char*)res <<std::endl;
		
	}
		
	//rv = pthread_attr_destroy(&thAttr); 
	

	/*
	std::cout << "Thread ID: " <<  TranscodeThreadHandle.get_id() << " will stop " << std::endl;
	StopFlag = true;
	if(TranscodeThreadHandle.joinable())
	TranscodeThreadHandle.join();
	*/
}