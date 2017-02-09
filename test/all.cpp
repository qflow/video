#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"

void main_coro() {
  qflow::video::demuxer demux;
  demux.open("big.mp4");
  auto codec = demux.codecpar(1);
  qflow::size s = { codec->width, codec->height };
  qflow::video::decoder dec(codec);
  qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_BGR24, {256, 256});
  qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_H264, s, { 25, 1 });
  qflow::video::muxer<std::experimental::filesystem::path> mux("asf", "out.mp4", {enc.codecpar()});
  for (auto packet : demux.packets(14000, 20000)) {
    if (packet->stream_index != 1)
      continue;
	dec.send_packet(packet);
	while (auto frame = dec.receive_frame())
	{
        auto rgb = conv.convert(frame);
		enc.send_frame(frame);
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
