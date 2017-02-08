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
#include <gflags/gflags.h>

DEFINE_string(input_file, "/path/to/input.mp4", "path to input video file");
DEFINE_string(output_file, "/path/to/output.csv", "path to output csv file");
DEFINE_string(model_folder, "/path/to/model/folder", "directory path of model folder");
DEFINE_string(mean_file, "/path/to/mean.binaryproto", "path to mean.binaryproto file");
DEFINE_string(labels_file, "/path/to/labels.txt", "path to labels file");
DEFINE_int32(batch_size, 10, "processing batch size, i.e how many video frames are processed in parallel");

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("classifies video files frame-by-frame");
    gflags::SetVersionString("1.0.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    qflow::video::demuxer demux;
    demux.open(FLAGS_input_file);
    auto codec = demux.codecpar(0);
    qflow::size s = { codec->width, codec->height };
    qflow::video::decoder dec(codec);
    qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_BGR24, {256, 256});
    qflow::video::classifier clas(FLAGS_model_folder, FLAGS_mean_file, FLAGS_labels_file, FLAGS_batch_size);
    std::ofstream out_csv;
    out_csv.open(FLAGS_output_file);
    out_csv << clas.labels();
    for (auto packet : demux.packets()) {
        if (packet->stream_index != 0)
            continue;
        dec.send_packet(packet);
        while (auto frame = dec.receive_frame())
        {
            auto rgb = conv.convert(frame);
            if(clas.append(rgb)) //fill batch
            {
                out_csv << clas.forward();//calculate
            }

        }
    }
    //flush remaining batch
    out_csv << clas.forward();
    out_csv.close();
    
    gflags::ShutDownCommandLineFlags();
    return 0;
}
