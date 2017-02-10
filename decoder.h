#pragma once
#include "ffmpeg.h"
#include <memory>

namespace qflow {
namespace video {
class decoder {
public:
	decoder(AVCodecParameters *codecpar) {
		AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
		AVCodecContext *context = avcodec_alloc_context3(codec);
		      context_.reset(context);
		int err = avcodec_parameters_to_context(context, codecpar);
		err = avcodec_open2(context, codec, NULL);
		/*AVFrame *frame;
		frame = av_frame_alloc();
		frame->format = _context->pix_fmt;
		frame->width = _context->width;
		frame->height = _context->height;
		frame->pts = 0;
		err = av_frame_get_buffer(frame, 32);
		_frameOut.reset(frame);*/
	}
	void flush()
    {
        avcodec_flush_buffers(context_.get());
    }
	void send_packet(AVPacketPointer packet)
	{
		int err = avcodec_send_packet(context_.get(), packet.get());
		if (err < 0)
		{
			char* errBuf = new char[255];
			av_strerror(err, errBuf, 255);
		}
	}
	AVFramePointer receive_frame()
	{
		AVFramePointer frame(av_frame_alloc(), AVFrameDeleter());
		int err = avcodec_receive_frame(context_.get(), frame.get());
		if (err == 0) {
			return frame;
		}
		return AVFramePointer();
	}

private:
	AVCodecContextPointer context_;
	//std::unique_ptr<AVFrame> _frameOut;
};
}
}
