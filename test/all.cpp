#include "demuxer.h"
#include "decoder.h"
#include "converter.h"
#include "encoder.h"
#include "muxer.h"
#include "classifier.h"
#include "generator.h"
#include "tcp_server.h"
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
    encoder(int i) : index_(i)
    {
    }
    encoder(const encoder& other) = default;
    void start()
    {
        std::thread t([&]() {
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
                qflow::video::encoder enc(AVCodecID::AV_CODEC_ID_VP8, s, { 25, 1 }, {{"flags", "+global_header"}, {"deadline", "realtime"}});
                codecpar_ = enc.codecpar();
                std::stringstream os;
                qflow::video::muxer<std::stringstream> mux("webm", os, {codecpar_});
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
                            packet_(new_packet);
                            mux.write(new_packet);
                            std::vector<char> vec(std::istreambuf_iterator<char>(os), {});
                            if(!vec.empty())
                            {
                                live(vec);
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
    boost::signals2::signal<void (std::vector<char>)> live;
    boost::signals2::signal<void (qflow::video::AVPacketPointer)> packet_;
    std::string header() const
    {
        return header_;
    }
    std::string name() const
    {
        return "video" + std::to_string(index_);
    }
    AVCodecParameters* codecpar() const
    {
        return codecpar_;
    }
private:
    std::string header_;
    int index_;
    AVCodecParameters* codecpar_;

};

class video_session : public std::enable_shared_from_this<video_session>
{
public:
    explicit video_session(tcp::socket socket)
        : socket_(std::move(socket)), strand_(socket_.get_io_service())
    {
    }
    ~video_session()
    {

    }
    void operator()(const std::vector<std::shared_ptr<encoder>>& encoders)
    {
        auto self(shared_from_this());
        boost::asio::spawn(strand_,
                           [this, self, encoders](boost::asio::yield_context yield)
        {
            boost::signals2::connection con;
            try
            {
                qflow::queue<std::vector<char>> q;
                auto encoder = encoders[1];
                con = encoder->live.connect([&](auto arg) {
                    q.push(arg);
                });
                std::stringstream os;
                qflow::video::muxer<std::stringstream> mux("webm", os, {encoder->codecpar()});
                auto media_header = mux.header();
                /*con = encoder->packet_.connect([&](auto packet) {
                    auto p = packet.get();
                    mux.write(packet);
                    std::vector<char> vec(std::istreambuf_iterator<char>(os), {});
                    if(!vec.empty())
                    {
                        q.push(vec);
                    }
                });*/
                req_type req;
                beast::streambuf sb;
                beast::http::async_read(socket_, sb, req, yield);
                std::cout << req.url;
                qflow::uri u(req.url);
                beast::http::response_header header;
                header.version = req.version;
                header.fields.insert("Content-Type", "video/webm");
                header.fields.insert("Server", "test");
                header.fields.insert("Accept-Ranges", "bytes");
                header.fields.insert("Connection", "Keep-Alive");
                if(req.fields.exists("Range"))
                {
                    auto range = req.fields["Range"].to_string();
                    range.replace(5, 1, " ");
                    std::cout << range << "\n";
                    header.status = 206;
                    header.reason = "Partial Content";
                    std::string cr = range + "10000000/*";
                    header.fields.insert("Content-Range", cr);
                    beast::http::async_write(socket_, header, yield);
                    boost::asio::async_write(socket_, boost::asio::buffer(media_header.data(), media_header.size()), yield);
                    for(;;)
                    {
                        auto data = q.pull();
                        for(auto e: data)
                        {
                            boost::asio::async_write(socket_, boost::asio::buffer(e.data(), e.size()), yield);
                            boost::asio::deadline_timer t(socket_.get_io_service(), boost::posix_time::seconds(1));
                            t.async_wait(yield);
                            std::cout << "streaming\n";
                        }
                    }
                }
                else
                {
                    header.status = 200;
                    header.reason = "OK";
                    header.fields.insert("Transfer-Encoding", "chunked");
                    beast::http::async_write(socket_, header, yield);
                    auto head_encoded = beast::http::chunk_encode(false, boost::asio::buffer(media_header.data(), media_header.size()));
                    boost::asio::async_write(socket_, head_encoded, yield);
                    for(;;)
                    {
                        auto data = q.pull();
                        for(auto e: data)
                        {
                            auto data_encoded = beast::http::chunk_encode(false, boost::asio::buffer(e.data(), e.size()));
                            boost::asio::async_write(socket_, data_encoded, yield);
                            boost::asio::deadline_timer t(socket_.get_io_service(), boost::posix_time::seconds(1));
                            t.async_wait(yield);
                        }
                    }
                }


                //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            }
            catch (boost::system::system_error& e)
            {
                auto message = e.what();
                std::cout << message << "\n\n";
            }
            con.disconnect();
        });
    }
private:
    tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};

int main() {

    using namespace std::chrono_literals;
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);

    boost::thread_group threadpool;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service);
    for(int i=0; i<8; i++)
    {
        threadpool.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service)
        );
    }

    qflow::queue<std::tuple<std::string, std::vector<char>>> q;

    int size = 8;
    std::vector<std::shared_ptr<encoder>> encoders;

    for(int i=0; i<size; i++)
    {
        auto e= std::make_shared<encoder>(i);
        encoders.push_back(e);
        e->start();
    }
    qflow::tcp_server<video_session>(io_service, "0.0.0.0", "4444")(encoders);





    boost::asio::spawn(io_service,
                       [&](boost::asio::yield_context yield)
    {
        for(;;)
        {
            std::vector<boost::signals2::connection> connections;
            try
            {

                /*std::string const host = "40.217.1.18";//demo.crossbar.io
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

                qflow::async_queue<std::tuple<std::string, std::vector<char>>> q2(io_service);
                for(int i=0; i<size; i++)
                {
                    auto con = encoders[i]->live.connect([&, i](auto arg) {
                        std::string name = encoders[i]->name();
                        q2.async_push(std::make_tuple(name, arg));

                    });
                    connections.push_back(con);

                    std::string uri = std::string(hostname) + "." + encoders[i]->name() + ".header";
                    auto reg_id = wamp.async_register(uri, yield);
                    int t=0;
                }

                q.flush();
                for(;;)
                {
                    auto vec = q2.async_pull(yield);
                    for(auto e: vec)
                    {
                        //wamp.async_publish(std::get<0>(e), false, yield, std::get<1>(e));
                    }
                    boost::asio::deadline_timer t(io_service, boost::posix_time::seconds(1));
                    t.async_wait(yield);
                    std::cout << "published";
                }*/
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
