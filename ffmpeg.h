#pragma once
extern "C" {
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
#include <mutex>
#include <memory>
#include <cassert>
#include "size.h"
#include "rational.h"

namespace qflow {
namespace video {
bool ffmpegInitialized = false;
std::mutex ffmpegInitMutex;
void initFfmpeg() {
  std::lock_guard<std::mutex> lock(ffmpegInitMutex);
  if (!ffmpegInitialized) {
    av_register_all();
    avdevice_register_all();
    avcodec_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_QUIET);
    ffmpegInitialized = true;
  }
}
AVFrame *alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *frame;
	frame = av_frame_alloc();
	frame->format = pix_fmt;
	frame->width = width;
	frame->height = height;
	frame->pts = 0;
	int res = av_frame_get_buffer(frame, 32);
	assert(!res);

	return frame;
}
struct AVFormatContextDeleter {
  void operator()(AVFormatContext *context) {
    if (context) {
      if (context->pb && context->oformat) {
        av_write_trailer(context);
      }
      if (context->pb && (context->flags & AVFMT_FLAG_CUSTOM_IO)) {
        delete[] context->pb->buffer;
        av_free(context->pb);
      }
      avformat_close_input(&context);
    }
  }
};
struct AVCodecContextDeleter {
  void operator()(AVCodecContext *context) {
    if(context)
    {
        if(avcodec_is_open(context))
        {
            avcodec_close(context);
        }
        avcodec_free_context(&context);
    }
  }
};
struct AVFrameDeleter {
	void operator()(AVFrame *frame) {
		if (frame)
		{
			av_frame_free(&frame);
		}
	}
};
struct AVPacketDeleter {
	void operator()(AVPacket *packet) {
		if (packet)
		{
			av_packet_unref(packet);
			delete packet;
		}
	}
};
struct SwsContextDeleter {
	void operator()(SwsContext *context) {
		if (context)
		{
			sws_freeContext(context);
		}
	}
};
using AVFormatContextPointer = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPointer = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using SwsContextPointer = std::unique_ptr<SwsContext, SwsContextDeleter>;
using AVFramePointer = std::shared_ptr<AVFrame>;
using AVPacketPointer = std::shared_ptr<AVPacket>;

struct stream_params
{
	AVCodecID codec_id;
	size size;
	rational sample_rate;
};
}
}
