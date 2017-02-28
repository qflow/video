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


class encoder
{
public:
    encoder()
    {
    }
    encoder(const encoder& other)
    {
    }
    void start(int i)
    {
        index_ = i;
        std::thread t([&, i]() {
            try {
                qflow::video::demuxer demux;
                int stream_index = 0;
                std::string name = this->name();
                std::cout << "Encoder " + name + " starting\n";
                demux.open("/dev/" + name, {{"standard", "PAL"}});
                auto codec = demux.codecpar(stream_index);
                qflow::size s = { codec->width, codec->height };
                qflow::video::decoder dec(codec);
                qflow::video::converter conv(static_cast<AVPixelFormat>(codec->format), s, AVPixelFormat::AV_PIX_FMT_YUV420P, s);
                qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_VP8, s, { 25, 1 });
                std::stringstream os;
                qflow::video::muxer<std::stringstream> mux("webm", os, {enc.codecpar()});
                header_ = mux.header();
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
                            std::vector<char> vec(std::istreambuf_iterator<char>(os), {});
                            auto a = vec.size();
                            if(!vec.empty())
                            {
                                live(std::make_tuple(name, vec));
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
    boost::signals2::signal<void (std::tuple<std::string, std::vector<char>>)> live;
    std::string header() const
    {
        return header_;
    }
    std::string name() const
    {
        return "video" + std::to_string(index_);
    }
private:
    std::string header_;
    int index_;

};

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

    int size = 9;
    std::vector<encoder> encoders(size, encoder());

    for(int i=0; i<size; i++)
    {
        encoders[i].start(i);
    }






    boost::asio::spawn(io_service,
                       [&](boost::asio::yield_context yield)
    {
        for(;;)
        {
            std::vector<boost::signals2::connection> connections;
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

                boost::asio::io_service::strand strand(io_service);
                for(int i=0; i<size; i++)
                {
                    auto con = encoders[i].live.connect([&](auto arg) {
                        std::string uri = std::string(hostname) + "." + std::get<0>(arg);
                        boost::asio::spawn(io_service,
                                           [&wamp, uri, arg](boost::asio::yield_context yield2)
                        {
                            try {
                                wamp.async_publish(uri + ".data", false, yield2, std::get<1>(arg));
                                //wamp.async_publish(uri + ".data", false, yield2, "test");
                            } catch (const std::exception& e)
                            {
                                auto message = e.what();
                                std::cout << message << "\n";
                            }
                        });
                    });
                    connections.push_back(con);

                    std::string uri = std::string(hostname) + "." + encoders[i].name() + ".header";
                    auto reg_id = wamp.async_register(uri, yield);
                    int t=0;
                    /*boost::asio::spawn(io_service,
                                       [&wamp, &encoders, i](boost::asio::yield_context yield2)
                    {
                        for(;;)
                        {
                            auto req_id = wamp.async_receive_invocation(reg_id, yield2);
                            wamp.async_yield<false>(req_id, std::make_tuple(encoders[i].header()), yield2);
                        }
                    });*/
                }

                q.flush();
                for(;;)
                {
                    boost::asio::deadline_timer t(io_service, boost::posix_time::seconds(1));
                    t.async_wait(yield);
                    ws.async_ping("", yield);
                }
            }
            catch (const std::exception& e)
            {
                auto message = e.what();
                std::cout << message << "\n";
            }
            for(auto con: connections)
            {
                con.disconnect();
            }
            boost::asio::deadline_timer t(io_service, boost::posix_time::seconds(5));
            t.async_wait(yield);
        }
    });



    threadpool.join_all();
    return 0;
}
