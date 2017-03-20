#pragma once
#include "ffmpeg.h"
#include "size.h"
#include "rational.h"
#include <experimental/filesystem>
#include <string>
#include <vector>

namespace qflow {
namespace video {
using options_type = std::map<std::string, std::string>;
template<typename T>
class muxer
{
    using options_type = std::map<std::string, std::string>;
public:
    muxer(const std::string& format, T& ostream, const std::vector<AVCodecParameters*>& streams, const options_type& options = options_type {}) :
        _ostream(ostream)
    {
        AVFormatContext* ctx = NULL;
        int err = avformat_alloc_output_context2(&ctx, NULL, format.c_str(), NULL);
        _formatContext.reset(ctx);
        //_formatContext->oformat = av_guess_format(format.c_str(), NULL, NULL);
        for (auto codec_param : streams)
        {
            AVCodec* codec = avcodec_find_encoder(codec_param->codec_id);
            AVStream* avStream = avformat_new_stream(_formatContext.get(), codec);
            int ret = avcodec_parameters_copy(avStream->codecpar, codec_param);
            int y=0;
        }
        const int bufSize = 32 * 1024;     
        unsigned char* buffer = new unsigned char[bufSize];     
        _formatContext->pb = avio_alloc_context(buffer, bufSize, 1, this, 0, muxer::writePacket, 0);     
        _formatContext->flags = AVFMT_FLAG_CUSTOM_IO;
        AVDictionary* dict = NULL;
        for(auto opt: options)
        {
            err = av_dict_set(&dict, opt.first.c_str(), opt.second.c_str(), 0);
        }
        auto a = _formatContext.get();
        int ret = avformat_init_output(_formatContext.get(), &dict);
        if(ret == AVSTREAM_INIT_IN_WRITE_HEADER) ret = avformat_write_header(_formatContext.get(), &dict);
        if (ret < 0) {
            char* errBuf = new char[255];
            av_strerror(ret, errBuf, 255);
            throw std::runtime_error(errBuf);
        }
        if(dict)
        {
            std::string str = "Muxer Options ";
            char* buf;
            int err = av_dict_get_string(dict, &buf, ':', ' ');
            str += std::string(buf) + " could not be set";
            throw std::runtime_error(str);
        }
        av_dump_format(_formatContext.get(), 0, "custom", 1);
    }
    void write(AVPacketPointer packet)
    {
        AVRational fpsScale = { 1,25 };
        av_packet_rescale_ts(packet.get(), fpsScale, _formatContext->streams[packet->stream_index]->time_base);
        int ret = av_write_frame(_formatContext.get(), packet.get());
        int t=0;
    }
    std::string header() const
    {
        return header_;
    }    
private:
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
    muxer(const std::string& format, const path& filename, const std::vector<AVCodecParameters*>& streams, const options_type& options = options_type {})
    {
        AVFormatContext* ctx = NULL;
        int err = avformat_alloc_output_context2(&ctx, NULL, format.c_str(), filename.c_str());
        _formatContext.reset(ctx);
        for (auto codec_param : streams)
        {
            AVCodec* codec = avcodec_find_encoder(codec_param->codec_id);
            AVStream* avStream = avformat_new_stream(_formatContext.get(), codec);
            int ret = avcodec_parameters_copy(avStream->codecpar, codec_param);
            int y=0;
        }
        
        AVIOContext* io_ctx = NULL;
        auto a = _formatContext.get();
        int ret = avio_open(&io_ctx, filename.c_str(), AVIO_FLAG_WRITE);
        _formatContext->pb = io_ctx;
        AVDictionary* dict = NULL;
        for(auto opt: options)
        {
            err = av_dict_set(&dict, opt.first.c_str(), opt.second.c_str(), 0);
        }
        ret = avformat_write_header(_formatContext.get(), &dict);
        if (ret < 0) {
            char* errBuf = new char[255];
            av_strerror(ret, errBuf, 255);
            throw std::runtime_error(errBuf);
        }
        av_dump_format(_formatContext.get(), 0, filename.c_str(), 1);
    }
    void write(AVPacketPointer packet)
    {
        AVRational fpsScale = { 1,25 };
        av_packet_rescale_ts(packet.get(), fpsScale, _formatContext->streams[packet->stream_index]->time_base);
        int ret = av_write_frame(_formatContext.get(), packet.get());
    }
private:
    AVFormatContextPointer _formatContext;

};
}
}
