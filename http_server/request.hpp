//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HPP
#define HTTP_SERVER3_REQUEST_HPP

#include <string>
#include <map>
#include <windows.h>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "header.hpp"

namespace http {
namespace server3 {

class file_store
{
  public:
    file_store();
    ~file_store();
    void flush(void);
    void operator()(char const ch);

    std::string     filename(void) const   { return filename_; }

  private:
    std::string     filename_;
    char            buffer_[1024*500];
    unsigned        buffer_position_;
    HANDLE          store_;
};

/// A request received from a client.
struct request : public std::enable_shared_from_this<request>
{
    request()
      : content_length(0),
        content_transferred_so_far(0),
        offset_(0)
    {
    }
    ~request();

    short const upload_percent_complete(void) const
    {
        return (short)((content_transferred_so_far * 100) / content_length);
    }

    boost::uint64_t uploaded_file_length(void) const
    {
        return content_length - offset_;
    }

    void increment_offset(void)
    {
        if (!file_store_.get())
            ++offset_;
    }

    boost::uint64_t const offset(void) const
    {
        return offset_;
    }

    void reset_offset(void)
    {
        offset_ = 0;
    }

    std::string const &filename(void) const
    {
        return filename_;
    }

    std::string const &id(void) const
    {
        return id_;
    }

    std::string const &link(void) const
    {
        return link_;
    }
  
    void close(void);

    void fail(void);

    void operator()(char const &ch);
    void on_form_data(void);

    std::string method;
    std::string uri;
    std::string boundary_marker;
    int http_version_major;
    int http_version_minor;
    headers_t headers;
    
    std::string     uploaded_file_mime_type;
    std::string     uploaded_file_original_filename;
    boost::uint64_t content_length;
    boost::uint64_t content_transferred_so_far;

    typedef std::map<std::string, std::shared_ptr<request> > requests_t;
    static bool const add_request(std::string const &id, std::shared_ptr<request> const &req);
    static bool const clear_requests(void);
    static bool const delete_request(std::string const &id);
    static bool const get_request(std::string const &id, std::shared_ptr<request> &req);
    static bool const is_valid_request(std::string const &id);

  private:
    boost::uint64_t           offset_;
    std::string               id_;
    std::string               twitter_screen_name_;
    std::string               tweet_;
    std::string               link_;
    std::string               filename_;
    std::auto_ptr<file_store> file_store_;

    static boost::mutex  requests_lock_;
    static requests_t    requests_;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REQUEST_HPP
