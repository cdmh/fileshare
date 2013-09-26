//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_CONNECTION_HPP
#define HTTP_SERVER3_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/timer.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"

namespace http {
namespace server3 {

/// Represents a single connection from a client.
class connection
  : public std::enable_shared_from_this<connection>,
    private boost::noncopyable
{
  public:
    /// Construct a connection with the given io_service.
    connection(boost::asio::io_service& io_service, request_handler& handler);
    ~connection();

    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket();

    /// Start the first asynchronous operation for the connection.
    void start();

    std::size_t const sync_send_data(void const *data, size_t const len);

    template<typename Buffer>
    std::size_t const sync_send_data_buffer(Buffer &buffer)
    {
        return boost::asio::write(socket_, buffer);
    }

  private:
    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred);

    void handle_read_post(const boost::system::error_code& e,
    std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e);

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand strand_;

    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

    /// The handler used to process the incoming request.
    request_handler& request_handler_;

    /// Buffer for incoming data.
    std::array<char, 1024*1024> buffer_;

    /// The incoming request.
    std::shared_ptr<request> request_;

    /// The parser for the incoming request.
    request_parser request_parser_;

    /// The reply to be sent back to the client.
    reply reply_;

    // percent complete that was last logged
    short last_reported_;

    boost::timer timer_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_CONNECTION_HPP
