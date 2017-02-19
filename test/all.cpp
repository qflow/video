#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"
#include <wamp2/wamp_router.h>
#include <boost/signals2/signal.hpp>
#include <boost/thread/thread.hpp>

class video_session : public std::enable_shared_from_this<video_session>
{
public:
    explicit video_session(tcp::socket socket)
        : socket_(std::move(socket)), strand_(socket_.get_io_service())
    {
    }
    void operator()()
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self](boost::asio::yield_context yield)
        {
            tcp::socket sock {socket_.get_io_service()};
            req_type req;
            try
            {

            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
        });
    }
private:
    tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};

int main() {

    boost::thread_group threadpool;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service);
    for(int i=0; i<4; i++)
    {
        threadpool.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service)
        );
    }
    qflow::tcp_server<video_session>(io_service, "0.0.0.0", "80")();

    qflow::video::demuxer demux;
    int stream_index = 0;
    demux.open("/dev/video0");
    boost::signals2::signal<void (qflow::video::AVPacketPointer)> sig;
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
                sig(new_packet);
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
    
    return 0;
}
