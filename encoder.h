#pragma once
#include "ffmpeg.h"
#include "size.h"
#include "rational.h"
#include <iostream>

namespace qflow {
namespace video {
class encoder {
    using options_type = std::map<std::string, std::string>;
public:
    encoder(AVCodecID codec_id, size s, rational framerate, const options_type& options = options_type {})
    {
        AVCodec* codec = avcodec_find_encoder(codec_id);
        context_.reset(avcodec_alloc_context3(codec));
        context_->width = s.width;
        context_->height = s.height;
        AVRational timeBase = { framerate.den, framerate.num };
        context_->time_base = timeBase;
        context_->pix_fmt = codec->pix_fmts[0];
        AVDictionary* dict = NULL;
        int err;
        for(auto opt: options)
        {
            err = av_dict_set(&dict, opt.first.c_str(), opt.second.c_str(), 0);
        }

        int ret = avcodec_open2(context_.get(), codec, &dict);
        if (ret < 0)
        {
            char* errBuf = new char[255];
            av_strerror(ret, errBuf, 255);
            throw std::runtime_error(errBuf);
        }
        if(dict)
        {
            std::string str = "Encoder Options ";
            char* buf;
            int err = av_dict_get_string(dict, &buf, ':', ' ');
            str += std::string(buf) + " could not be set";
            throw std::runtime_error(str);
        }
        int i = 0;
    }
    int send_frame(AVFramePointer frame)
    {
        if(frame) frame->pts = _pts;
        int err = avcodec_send_frame(context_.get(), frame.get());
        if (err < 0)
        {
            char* errBuf = new char[255];
            av_strerror(err, errBuf, 255);
            std::cout << errBuf;
        }
        _pts++;
        return err;
    }
    AVPacketPointer receive_packet()
    {
        AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
        av_init_packet(packet.get());
        packet.get()->data = NULL;
        packet.get()->size = 0;
        int err = avcodec_receive_packet(context_.get(), packet.get());
        if (err == 0) {
            return packet;
        }
        return AVPacketPointer();
    }
    auto codecpar()
    {
        AVCodecParameters* par = avcodec_parameters_alloc();
        avcodec_parameters_from_context(par, context_.get());
        return par;
    }
private:
    AVCodecContextPointer context_;
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
