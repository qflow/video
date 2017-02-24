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
#include <sstream>
#include <chrono>
#include <boost/asio/use_future.hpp>
#include "queue.h"
#include <unistd.h>
#include <limits.h>



int main() {

    using namespace std::chrono_literals;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    boost::thread_group threadpool;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service);
    for(int i=0; i<4; i++)
    {
        threadpool.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service)
        );
    }

    qflow::queue<std::tuple<std::string, std::string>> q;
    boost::signals2::signal<void (std::tuple<std::string, std::string>)> sig;
    for(int i=0; i<9; i++)
    {
        std::thread t([&, i]() {
            try {
                qflow::video::demuxer demux;
                int stream_index = 0;
                std::string name = "video" + std::to_string(i);
                std::cout << "Encoder " + name + " starting\n";
                demux.open("/dev/" + name, {{"standard", "PAL"}});
                auto codec = demux.codecpar(stream_index);
                qflow::size s = { codec->width, codec->height };
                qflow::video::decoder dec(codec);
                qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_YUV420P, s);
                qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_VP8, s, { 25, 1 });
                std::stringstream os;
                qflow::video::muxer<std::stringstream> mux("webm", os, {enc.codecpar()});
                std::cout << "Encoder " + name + " started\n";
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
                            std::string str(std::istreambuf_iterator<char>(os), {});
                            if(!str.empty())
                            {
                                q.push(std::make_tuple(name, str));
                            }
                        }

                    }
                }
            }
            catch(const std::exception& e)
            {
                std::string message = e.what();
                std::cout << message;
            }
        });
        t.detach();
    }






    boost::asio::spawn(io_service,
                       [&](boost::asio::yield_context yield)
    {
        for(;;)
        {
            try
            {

                std::string const host = "40.217.1.18";//demo.crossbar.io
                boost::asio::ip::tcp::resolver r {io_service};
                boost::asio::ip::tcp::socket sock {io_service};
                using stream_type = boost::asio::ip::tcp::socket;
                beast::websocket::stream<stream_type&> ws {sock};
                ws.set_option(beast::websocket::decorate(qflow::protocol {qflow::KEY_WAMP_MSGPACK_SUB}));
                ws.set_option(beast::websocket::message_type {beast::websocket::opcode::binary});
                qflow::wamp_stream<beast::websocket::stream<stream_type&>&, qflow::msgpack_serializer> wamp {ws};

                auto i = r.async_resolve(boost::asio::ip::tcp::resolver::query {host, "8080"}, yield);
                boost::asio::async_connect(sock, i, yield);
                ws.async_handshake(host, "/ws",yield);
                wamp.async_handshake("realm1", yield);

                q.flush();
                for(;;)
                {
                    boost::asio::deadline_timer t(io_service, boost::posix_time::seconds(1));
                    t.async_wait(yield);
                    auto data = q.pull();
                    for(auto e: data)
                    {
                        std::string uri = std::string(hostname) + "." + std::get<0>(e) + ".data";
                        wamp.async_publish(uri, false, yield, std::get<1>(e));
                    }
                }
            }
            catch (const std::exception& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
            boost::asio::deadline_timer t(io_service, boost::posix_time::seconds(5));
            t.async_wait(yield);
        }
    });



    threadpool.join_all();
    return 0;
}
