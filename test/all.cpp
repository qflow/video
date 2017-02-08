#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"
#include <future>
#include <type_traits>
#include <sstream>

void main_coro() {
  qflow::video::demuxer demux;
  demux.open("small.mp4");
  auto codec = demux.codecpar(0);
  qflow::size s = { codec->width, codec->height };
  qflow::video::decoder dec(codec);
  qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_BGR24, {256, 256});
  qflow::video::classifier clas("/home/michal/workspace/digits/digits/jobs/20160615-092011-01f2", 
                                "/home/michal/workspace/digits/digits/jobs/20160609-101844-274f/mean.binaryproto", 
                                "/home/michal/workspace/digits/digits/jobs/20160609-101844-274f/labels.txt", 10);
  std::ofstream out_csv;
  out_csv.open("out.csv");
  out_csv << clas.labels();
  qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_H264, s, { 30, 1 });
  std::stringstream ss;
  qflow::video::muxer<std::stringstream> mux("asf", ss, {enc.codecpar()});
  for (auto packet : demux.packets()) {
    if (packet->stream_index != 0)
      continue;
	dec.send_packet(packet);
	while (auto frame = dec.receive_frame())
	{
        auto rgb = conv.convert(frame);
		enc.send_frame(frame);
        if(clas.append(rgb)) //fill batch
        {
            out_csv << clas.forward();//calculate
        }
		while (auto new_packet = enc.receive_packet())
		{
			mux.write(new_packet);
		}
        
	}
  }
  //flush remaining batch
  out_csv << clas.forward();
  
  //flush encoder buffer
  enc.send_frame(qflow::video::AVFramePointer());
  while (auto new_packet = enc.receive_packet())
  {
	  mux.write(new_packet);
  }
  out_csv.close();
}


int main()
{
	main_coro();
	int t = 0;
}
