//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "request_handler.hpp"
#include <iostream>
#include <boost/lexical_cast.hpp>

namespace http {
namespace server3 {

connection::connection(boost::asio::io_service& io_service,
    request_handler& handler)
  : strand_(io_service),
    socket_(io_service),
    request_handler_(handler),
    last_reported_(0)
{
}

connection::~connection()
{
    double elapsed = timer_.elapsed();
    if (elapsed >= 2.0)
        LOG_MEM << "Connection closed after " << elapsed << " seconds.";
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{
    request_.reset(new request);

    socket_.async_read_some(boost::asio::buffer(buffer_),
    strand_.wrap(
        boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)));
}

void connection::handle_read_post(boost::system::error_code const &e, std::size_t bytes_transferred)
{
    boost::tribool result;
    std::tie(result, std::ignore) = request_parser_.parse(
        request_, begin(buffer_), begin(buffer_) + bytes_transferred);

    request_->content_transferred_so_far += bytes_transferred;

    if (e)
    {
        LOG_MEM << "*** Error " << e.value() << " @ " << __FILE__ << " (" << __LINE__ << "):  " <<  e.message();

        request_handler_.handle_request(request_, reply_, shared_from_this());
        boost::asio::async_write(socket_, reply_.to_buffers(),
            strand_.wrap(
                boost::bind(&connection::handle_write, shared_from_this(),
                boost::asio::placeholders::error)));
        request_->fail();
    }
    else
    {
//        LOG_MEM << bytes_transferred << " bytes received.";

        short const pcent = request_->upload_percent_complete();
        if (pcent-last_reported_ >= 10)
        {
            LOG_MEM << request_->id() << " Uploaded " << bytes_transferred << " bytes. "
                << request_->content_transferred_so_far << " of "
                << request_->content_length << "\t"
                << pcent << "%.";
            last_reported_ = pcent;
        }

        if (request_->content_transferred_so_far == request_->content_length)
        {
            request_handler_.handle_request(request_, reply_, shared_from_this());
            boost::asio::async_write(socket_, reply_.to_buffers(),
                strand_.wrap(
                    boost::bind(&connection::handle_write, shared_from_this(),
                    boost::asio::placeholders::error)));
            request_->close();
        }
        else
        {
            Sleep(10); //throttle upload
//            LOG_MEM << "Queuing read for file upload";
            socket_.async_read_some(boost::asio::buffer(buffer_),
                strand_.wrap(
                    boost::bind(&connection::handle_read_post, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)));
        }
    }

    // If an error occurs then no new asynchronous operations are started. This
    // means that all shared_ptr references to the connection object will
    // disappear and the object will be destroyed automatically after this
    // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_read(
    boost::system::error_code const &e,
    std::size_t bytes_transferred)
{
    if (e)
    {
        LOG_MEM << "*** Error " << e.value() << " @ " << __FILE__ << " (" << __LINE__ << "):  " <<  e.message();
        if (e.value() != 2) // end of file
        {
            socket_.async_read_some(boost::asio::buffer(buffer_),
                strand_.wrap(
                boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)));
        }
    }
    else
    {
        boost::tribool result;
        std::tie(result, std::ignore) = request_parser_.parse(
            request_, buffer_.data(), buffer_.data() + bytes_transferred);

        if (result)
        {
            LOG_MEM << "Method\t" << request_->method;
            if (stricmp(request_->method.c_str(), "POST") == 0)
            {
                request_->content_transferred_so_far = 0;
                request_->boundary_marker.clear();

                for (auto it=request_->headers.begin(); it!=request_->headers.end(); ++it)
                {
                    if (stricmp(it->name.c_str(), "Content-Length") == 0)
                        request_->content_length = boost::lexical_cast<boost::uint64_t>(it->value);
                    else if (strnicmp(it->name.c_str(), "Content-Type", 12) == 0)
                    {
                        // skip "multipart/form-data; boundary="
                        assert(it->value.length() > 30);
                        request_->boundary_marker = "\r\n--";
                        request_->boundary_marker += &it->value[30];
                        request_->boundary_marker += "\r\n";
                    }
                }
                LOG_MEM << "Post File: " << request_->content_length << " bytes";

                if (request_->offset() == bytes_transferred)
                {
                    socket_.async_read_some(boost::asio::buffer(buffer_),
                        strand_.wrap(
                        boost::bind(&connection::handle_read_post, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)));
                }
                else
                {
                    boost::uint64_t dest = 0;
                    for (boost::uint64_t src=request_->offset(); src<bytes_transferred; ++src, ++dest)
                        buffer_[dest] = buffer_[src];
                    handle_read_post(e, bytes_transferred - request_->offset());
                }
            }
#ifndef NDEBUG      // !!! this isn't fully implemented (aka working!)
            else if (stricmp(request_->method.c_str(), "OPTIONS") == 0)
            {
                for (http::server3::headers_t::const_iterator it=request_->headers.begin(); it!=request_->headers.end(); ++it)
                {
                    if (stricmp(it->name.c_str(), "Access-Control-Request-Method") == 0)
                        if (stricmp(it->value.c_str(), "POST") == 0)
                        {
                            reply_ = reply::stock_reply(reply::ok);
                            reply_.headers.clear();
                            reply_.headers.push_back(headers_t::value_type("Access-Control-Allow-Origin","*"));
                            reply_.headers.push_back(headers_t::value_type("Access-Control-Allow-Methods","POST"));
                            reply_.headers.push_back(headers_t::value_type("Access-Control-Request-Headers","UP-TYPE, UP-FILENAME, UP-SIZE, Content-Type"));

                            sync_send_data_buffer(reply_.to_buffers());
                            socket_.async_read_some(boost::asio::buffer(buffer_),
                                strand_.wrap(
                                    boost::bind(&connection::handle_read, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)));
                            break;
                        }
                }
            }
#endif
            else
            {
                request_handler_.handle_request(request_, reply_, shared_from_this());
                if (reply_.content.size() > 0 && reply_.headers.size() > 0)
                {
                    boost::asio::async_write(socket_, reply_.to_buffers(),
                        strand_.wrap(
                            boost::bind(&connection::handle_write, shared_from_this(),
                            boost::asio::placeholders::error)));
                }
            }
        }
        else if (!result)
        {
            reply_ = reply::stock_reply(reply::bad_request);
            boost::asio::async_write(socket_, reply_.to_buffers(),
                strand_.wrap(
                boost::bind(&connection::handle_write, shared_from_this(),
                    boost::asio::placeholders::error)));
        }
        else
        {
            socket_.async_read_some(boost::asio::buffer(buffer_),
                strand_.wrap(
                boost::bind(&connection::handle_read, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)));
        }
    }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_write(boost::system::error_code const &e)
{
    if (e)
        LOG_MEM << "*** Error " << e.value() << " @ " << __FILE__ << " (" << __LINE__ << "):  " <<  e.message();
    else
    {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }

    // No new asynchronous operations are started. This means that all shared_ptr
    // references to the connection object will disappear and the object will be
    // destroyed automatically after this handler returns. The connection class's
    // destructor closes the socket.
}

std::size_t const connection::sync_send_data(void const *data, size_t const len)
{
    try
    {
        return boost::asio::write(socket_, boost::asio::buffer(data, len));
    }
    catch (std::exception &e)
    {
        LOG_MEM << "Exception sending synchronous data: " << e.what();
    }

    return 0;
}

} // namespace server3
} // namespace http
