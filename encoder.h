#pragma once
#include "ffmpeg.h"
#include "size.h"
#include "rational.h"

namespace qflow {
namespace video {
class encoder {
public:
	encoder(AVCodecID codec_id, size s, rational framerate)
	{
		AVCodec* codec = avcodec_find_encoder(codec_id);
		_context.reset(avcodec_alloc_context3(codec));
		_context->width = s.width;
		_context->height = s.height;
		AVRational timeBase = { framerate.den, framerate.num };
		_context->time_base = timeBase;
		_context->pix_fmt = codec->pix_fmts[0];
		int ret = avcodec_open2(_context.get(), codec, nullptr);
		if (ret < 0)
		{
			char* errBuf = new char[255];
			av_strerror(ret, errBuf, 255);
			int a = 0;
		}
		int i = 0;
	}
	void send_frame(AVFramePointer frame)
	{
		if(frame) frame->pts = _pts;
		int err = avcodec_send_frame(_context.get(), frame.get());
		if (err < 0)
		{
			char* errBuf = new char[255];
			av_strerror(err, errBuf, 255);
		}
		_pts++;
	}
	AVPacketPointer receive_packet()
	{
		AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
		av_init_packet(packet.get());
		packet.get()->data = NULL;
		packet.get()->size = 0;
		int err = avcodec_receive_packet(_context.get(), packet.get());
		if (err == 0) {
			return packet;
		}
		return AVPacketPointer();
	}
	auto codecpar()
	{
		AVCodecParameters* par = avcodec_parameters_alloc();
		avcodec_parameters_from_context(par, _context.get());
		return par;
	}
private:
	AVCodecContextPointer _context;
	size _size;
	rational _framerate;
	int _bitrate;
	int _asyncDepth;
	int _gopSize;
	AVCodecID _codec;
	int _pts = 0;
};
}
}