#include "ffmpeg.h"
#include <string>
#include "generator.h"
#include <experimental/resumable>
#include <experimental/generator>

namespace qflow {
namespace video {
class demuxer {
public:
  demuxer()
  {

  }
  void open(std::string filename) {
    initFfmpeg();
    _formatContext.reset(avformat_alloc_context());
    AVFormatContext* ctx = _formatContext.get();
    int err = avformat_open_input(&ctx, filename.c_str(), NULL, NULL);
    if(err < 0)
    {
        char* errBuf = new char[255];
        av_strerror(err, errBuf, 255);
        int i = 0;
    }
    err = avformat_find_stream_info(_formatContext.get(), NULL);
    int i=0;
  }
  auto packets()
  {
    for(;;)
    {
		AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
        int read = av_read_frame(_formatContext.get(), packet.get());
        if(read < 0) break;
        co_yield packet;
    }
  }
  auto packets2()
  {
	  return qflow::generator<AVPacketPointer>([this](auto par)
	  {
		  AVPacketPointer packet(new AVPacket(), AVPacketDeleter());
		  int read = av_read_frame(_formatContext.get(), packet.get());
		  if (read == 0) *par = packet;
	  });
  }
  auto codecpar(unsigned int stream_index)
  {
     AVStream* stream = _formatContext->streams[stream_index];
     return stream->codecpar;
  }

private:
  AVFormatContextPointer _formatContext;
};
}
}