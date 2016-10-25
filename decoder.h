#pragma once
#include "ffmpeg.h"
#include <future>
#include <memory>

namespace qflow {
namespace video {
class decoder {
public:
  decoder(AVCodecParameters *codecpar) {
    AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *context = avcodec_alloc_context3(codec);
    _context.reset(context);
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
  std::future<AVFramePointer> decode(AVPacketPointer packet) {
    int err = avcodec_send_packet(_context.get(), packet.get());
    /*if(err == AVERROR(EAGAIN)) co_return;
    if(err < 0)
    {
        char* errBuf = new char[255];
        av_strerror(err, errBuf, 255);
        co_return;
    }*/
	if (err == 0) {
		//AVFrame *avFrameOut = _frameOut.get();
		//AVFrame* f = av_frame_alloc();
		/*AVFramePointer frame(av_frame_alloc(), [](AVFrame* f) {
			av_frame_free(&f);
		});*/
		AVFramePointer frame(av_frame_alloc(), AVFrameDeleter());
		err = avcodec_receive_frame(_context.get(), frame.get());
		if (err == 0) {
			co_return frame;
		}
	}
  }

private:
  AVCodecContextPointer _context;
  //std::unique_ptr<AVFrame> _frameOut;
};
}
}