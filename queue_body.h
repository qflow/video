#include <beast/core/error.hpp>
#include <beast/http/message.hpp>
#include <beast/http/resume_context.hpp>

namespace qflow{
struct queue_body
{
    using value_type = std::string;

    class writer
    {
        int written_ = 0;
    public:
        writer(writer const&) = delete;
        writer& operator=(writer const&) = delete;

        template<bool isRequest, class Fields>
        writer(beast::http::message<isRequest, queue_body, Fields> const& m) noexcept
        {
        }

        ~writer()
        {
        }

        void
        init(beast::error_code& ec) noexcept
        {
        }

        std::uint64_t
        content_length() const noexcept
        {
            return 1000;
        }

        template<class WriteFunction>
        boost::tribool
        write(beast::http::resume_context&&, beast::error_code&,
            WriteFunction&& wf) noexcept
        {
            written_ += 4;
            wf(boost::asio::buffer("ahoj"));
            return written_ == 8;
        }
    };
};
}
