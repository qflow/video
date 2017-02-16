#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"

void main_coro() {
  qflow::video::demuxer demux;
  int stream_index = 0;
  demux.open("/dev/video0");
  auto codec = demux.codecpar(stream_index);
  qflow::size s = { codec->width, codec->height };
  qflow::video::decoder dec(codec);
  qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_YUV420P, s);
  qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_H264, s, { 25, 1 });
  qflow::video::muxer<std::experimental::filesystem::path> mux("asf", "out.mp4", {enc.codecpar()});
  for (auto packet : demux.packets()) {
    if (packet->stream_index != stream_index)
      continue;
	dec.send_packet(packet);
	while (auto frame = dec.receive_frame())
	{
        auto yuv = conv.convert(frame);
		enc.send_frame(yuv);
		while (auto new_packet = enc.receive_packet())
		{
			mux.write(new_packet);
		}
        
	}
  }
  
  //flush encoder buffer
  enc.send_frame(qflow::video::AVFramePointer());
  while (auto new_packet = enc.receive_packet())
  {
	  mux.write(new_packet);
  }
}


int main()
{
	main_coro();
	int t = 0;
}
