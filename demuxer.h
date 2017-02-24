#include "ffmpeg.h"
#include <string>
#include <iostream>
#include "generator.h"
#include <map>

namespace qflow {
namespace video {
class demuxer {
    using options_type = std::map<std::string, std::string>;
public:
    demuxer()
    {

    }
    ~demuxer()
    {
        
    }
    void open(std::string filename, const options_type& options = options_type{} ) {
        initFfmpeg();
        formatContext_.reset(avformat_alloc_context());
        AVFormatContext* ctx = formatContext_.get();
        AVDictionary* dict = NULL;
        int err;
        for(auto opt: options)
        {
            err = av_dict_set(&dict, opt.first.c_str(), opt.second.c_str(), 0);
        }
        err = avformat_open_input(&ctx, filename.c_str(), NULL, &dict);
        if(err < 0)
        {
            char* errBuf = new char[255];
            av_strerror(err, errBuf, 255);
            throw std::runtime_error(filename + " " + errBuf);
        }
        err = avformat_find_stream_info(formatContext_.get(), NULL);
        int t=0;
        
    }
    /*auto packets()
    {
      for(;;)
      {
    	AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
          int read = av_read_frame(_formatContext.get(), packet.get());
          if(read < 0) break;
          co_yield packet;
      }
    }*/
    rational frame_rate(unsigned int stream_index)
    {
        AVRational avr = av_guess_frame_rate(formatContext_.get(), formatContext_->streams[stream_index], NULL);
        return rational {avr.num, avr.den};
    }
    auto packets()
    {
        return qflow::generator<AVPacketPointer>([this](auto par)
        {
            AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
            int read = av_read_frame(formatContext_.get(), packet.get());
            if (read == 0) *par = packet;
        });
    }
    auto packets(int64_t start_milisec, int64_t end_milisec)
    {
        key_count_ = 0;
        int ds = av_find_default_stream_index(formatContext_.get());
        AVStream* stream = formatContext_->streams[ds];
        auto timestamp = av_rescale_q(start_milisec, milisec_scale, stream->time_base);
        int res = av_seek_frame(formatContext_.get(), ds,
                      timestamp + stream->start_time -  2 * stream->time_base.den, 0);
        
        return qflow::generator<AVPacketPointer>([this, end_milisec](auto par)
        {
            AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
            int read = av_read_frame(formatContext_.get(), packet.get());
            if(packet->flags & AV_PKT_FLAG_KEY) key_count_++;

            AVStream* stream = formatContext_->streams[packet->stream_index];
            auto miliseconds = av_rescale_q(packet->pts - stream->start_time, stream->time_base, milisec_scale);
            
            if ((read == 0 && miliseconds < end_milisec) || (key_count_ < 2)) *par = packet;
        });
    }
    auto timestamp_miliseconds(int64_t t, int stream_index)
    {
        AVStream* stream = formatContext_->streams[stream_index];
        return av_rescale_q(t - stream->start_time, stream->time_base, milisec_scale);
    }
    auto codecpar(unsigned int stream_index)
    {
        AVStream* stream = formatContext_->streams[stream_index];
        return stream->codecpar;
    }

private:
    AVFormatContextPointer formatContext_;
    AVRational milisec_scale = {1, (int)1E3};
    int key_count_;
};
}
}
