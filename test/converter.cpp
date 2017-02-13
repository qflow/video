#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"
#include <gflags/gflags.h>
#include <algorithm>
#include <set>

std::istream& safe_get_line(std::istream& is, std::string& t)
{
    t.clear();
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case EOF:
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }
}
int64_t milisecs(std::string time)
{
    int64_t res = 0;
    int hours;
    int minutes;
    double secs;
    std::sscanf(time.c_str(), "%2d:%2d:%10lf", &hours, &minutes, &secs);
    res = (hours * 3600 * 1E3) + (minutes * 60 * 1E3) + (secs * 1E3);
    return res;
}
std::ostream& operator<<(std::ostream& os, const std::set<std::string>& set)  
{  
    for(auto e: set)
    {
        os << e << '\n';
    }
    return os; 
}


namespace fs = std::experimental::filesystem;
DEFINE_string(input_file, "/path/to/input.mp4", "path to input video file");
DEFINE_string(output_folder, "/path/to/folder", "path to output folder");

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("converts video file and corresponding clip classification csv file into Digits format");
    gflags::SetVersionString("1.0.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    qflow::video::demuxer demux;
    demux.open(FLAGS_input_file);
    auto codec = demux.codecpar(0);
    qflow::size s = { codec->width, codec->height };
    qflow::video::decoder dec(codec);
    qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_BGR24, {256, 256});


    fs::path input = FLAGS_input_file;
    input.replace_extension(".csv");
    std::ifstream csv;
    csv.open(input.string());
    std::string header;
    std::getline(csv, header);
    std::string record;
    std::set<std::string> behaviors;
    std::ofstream out_labels;
    fs::create_directories(FLAGS_output_folder);
    out_labels.open(FLAGS_output_folder + "/labels.txt");
    while(safe_get_line(csv, record))
    {
        dec.flush();
        std::stringstream rs(record);
        std::string behavior;
        std::getline(rs, behavior, ',');
        if(behavior.empty()) continue;
        behaviors.insert(behavior);
        std::string start_time;
        std::getline(rs, start_time, ',');
        std::string end_time;
        std::getline(rs, end_time, ',');
        auto start = milisecs(start_time);
        auto end = milisecs(end_time);
        
        int stream_index = 0;
        auto framerate = demux.frame_rate(stream_index);

        std::replace(behavior.begin(), behavior.end(), ' ', '_');
        std::string dir = FLAGS_output_folder + "/" + behavior;
        fs::create_directories(dir);
        std::string out =  dir + "/" + input.stem().string() + "-" + start_time;
        std::remove(out.begin(), out.end(), '.');
        std::remove(out.begin(), out.end(), ':');
        
        out += ".mp4";
        
        
        qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_H264, s, framerate);
        qflow::video::muxer<std::experimental::filesystem::path> mux("asf", out, {enc.codecpar()});
        for (auto packet : demux.packets(start, end)) {
            if (packet->stream_index != stream_index)
                continue;
            dec.send_packet(packet);
            while (auto frame = dec.receive_frame())
            {
                auto timestamp = demux.timestamp_miliseconds(frame->pts, stream_index);
                if(timestamp < start || timestamp > end) continue;
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

    out_labels << behaviors;


    return 0;
}
