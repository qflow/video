#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include <future>
#include <type_traits>

std::future<void> main_coro() {
  qflow::video::demuxer demux;
  demux.open("small.mp4");
  auto codec = demux.codecpar(0);
  qflow::video::size s = { codec->width, codec->height };
  qflow::video::decoder dec(codec);
  qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_RGB24, s);
  for (auto packet : demux.packets()) {
    if (packet->stream_index != 0)
      continue;
    auto frame = co_await dec.decode(packet);
	auto rgb = conv.convert(frame);
	int t = 0;
  }
  int i = 0;
}

int main()
{
	main_coro();
	int t = 0;
}