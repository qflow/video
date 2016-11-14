#pragma once
#include "ffmpeg.h"
#include "size.h"
#include "rational.h"
#include <vector>

namespace qflow{
namespace video {
class muxer
{
public:
	muxer(std::string format, std::string filename, std::vector<AVCodecParameters*> streams)
	{
		_formatContext.reset(avformat_alloc_context());
		_formatContext->oformat = av_guess_format(format.c_str(), NULL, NULL);
		for (auto codec_param : streams)
		{
			AVCodec* codec = avcodec_find_encoder(codec_param->codec_id);
			AVStream* avStream = avformat_new_stream(_formatContext.get(), codec);
			int ret = avcodec_parameters_copy(avStream->codecpar, codec_param);
		}
		AVIOContext* ctx = NULL;
		int ret = avio_open(&ctx, filename.c_str(), AVIO_FLAG_WRITE);
		_formatContext->pb = ctx;
		ret = avformat_write_header(_formatContext.get(), NULL);
		if (ret < 0) {
			char* errBuf = new char[255];
			av_strerror(ret, errBuf, 255);
			int i = 0;
		}
		av_dump_format(_formatContext.get(), 0, filename.c_str(), 1);
	}
	void write(AVPacketPointer packet)
	{
		AVRational fpsScale = { 1,30 };
		av_packet_rescale_ts(packet.get(), fpsScale, _formatContext->streams[packet->stream_index]->time_base);
		int ret = av_write_frame(_formatContext.get(), packet.get());
	}
private:
	AVFormatContextPointer _formatContext;
	
};
}
}