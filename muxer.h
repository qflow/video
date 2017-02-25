#pragma once
#include "ffmpeg.h"
#include "size.h"
#include "rational.h"
#include <experimental/filesystem>
#include <string>
#include <vector>

namespace qflow {
namespace video {
template<typename T>
class muxer
{
public:
    muxer(const std::string& format, T& ostream, const std::vector<AVCodecParameters*>& streams) :
        _ostream(ostream)
    {
        init(format, streams);
        const int bufSize = 32 * 1024;     
        unsigned char* buffer = new unsigned char[bufSize];     
        _formatContext->pb = avio_alloc_context(buffer, bufSize, 1, this, 0, muxer::writePacket, 0);     _formatContext->flags = AVFMT_FLAG_CUSTOM_IO;
        int ret = avformat_write_header(_formatContext.get(), NULL);
        if (ret < 0) {
            char* errBuf = new char[255];
            av_strerror(ret, errBuf, 255);
            int i = 0;
        }
        av_dump_format(_formatContext.get(), 0, "custom", 1);
    }
    void write(AVPacketPointer packet)
    {
        AVRational fpsScale = { 1,30 };
        av_packet_rescale_ts(packet.get(), fpsScale, _formatContext->streams[packet->stream_index]->time_base);
        int ret = av_write_frame(_formatContext.get(), packet.get());
    }
    std::string header() const
    {
        return header_;
    }    
private:
        void init(const std::string& format, const std::vector<AVCodecParameters*>& streams)
    {
        _formatContext.reset(avformat_alloc_context());
        _formatContext->oformat = av_guess_format(format.c_str(), NULL, NULL);
        for (auto codec_param : streams)
        {
            AVCodec* codec = avcodec_find_encoder(codec_param->codec_id);
            AVStream* avStream = avformat_new_stream(_formatContext.get(), codec);
            int ret = avcodec_parameters_copy(avStream->codecpar, codec_param);
            int y=0;
        }
    }
    static int writePacket(void *opaque, uint8_t *buf, int buf_size)     
    {
        muxer* mux = static_cast<muxer*>(opaque);
        std::string str = std::string(reinterpret_cast<const char*>(buf), buf_size);
        if(mux->header_.empty())
        {
            mux->header_ = str;
        }
        mux->_ostream << str;
        return buf_size;
    }
    AVFormatContextPointer _formatContext;
    T& _ostream;
    std::string header_;
};



using std::experimental::filesystem::path;
template<>
class muxer<path>
{
public:
    muxer(const std::string& format, const path& filename, const std::vector<AVCodecParameters*>& streams)
    {
        init(format, streams);
        AVIOContext* ctx = NULL;
        auto a = filename.string();
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
    void init(const std::string& format, const std::vector<AVCodecParameters*>& streams)
    {
        _formatContext.reset(avformat_alloc_context());
        _formatContext->oformat = av_guess_format(format.c_str(), NULL, NULL);
        for (auto codec_param : streams)
        {
            AVCodec* codec = avcodec_find_encoder(codec_param->codec_id);
            AVStream* avStream = avformat_new_stream(_formatContext.get(), codec);
            int ret = avcodec_parameters_copy(avStream->codecpar, codec_param);
            int y=0;
        }
    }
    AVFormatContextPointer _formatContext;

};
}
}
