#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include <gflags/gflags.h>

DEFINE_string(input_file, "/path/to/input.mp4", "path to input video file");
DEFINE_string(output_file, "/path/to/output.webm", "path to output file");

namespace fs = std::experimental::filesystem;

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("converts video files");
    gflags::SetVersionString("1.0.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    qflow::video::demuxer demux;
    demux.open(FLAGS_input_file);
    
    qflow::video::muxer<fs::path> mux("webm", FLAGS_output_file, {demux.codecpar(0)}, {});
    
    for (auto packet : demux.packets()) {
        mux.write(packet);
    }
}

