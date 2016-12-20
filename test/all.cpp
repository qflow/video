#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "generator.h"
#include <future>
#include <type_traits>

void main_coro() {
  qflow::video::demuxer demux;
  demux.open("small.mp4");
  auto codec = demux.codecpar(0);
  qflow::size s = { codec->width, codec->height };
  qflow::video::decoder dec(codec);
  qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_RGB24, s);
  qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_H264, s, { 30, 1 });
  qflow::video::muxer mux("mp4", "D:/test.mp4", {enc.codecpar()});
  for (auto packet : demux.packets()) {
    if (packet->stream_index != 0)
      continue;
	dec.send_packet(packet);
	while (auto frame = dec.receive_frame())
	{
		enc.send_frame(frame);
		while (auto new_packet = enc.receive_packet())
		{
			mux.write(new_packet);
		}
		auto rgb = conv.convert(frame);
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
