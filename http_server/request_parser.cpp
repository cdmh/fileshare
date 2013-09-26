//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_parser.hpp"
#include "request.hpp"
#include <iostream> // !!!

namespace http {
namespace server3 {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

boost::tribool request_parser::consume(std::shared_ptr<request> const &req, char const &input)
{
  req->increment_offset();
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      state_ = method;
      req->method.push_back(input);
      return boost::indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req->method.push_back(input);
      return boost::indeterminate;
    }
  case uri_start:
    if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = uri;
      req->uri.push_back(input);
      return boost::indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      req->uri.push_back(input);
      return boost::indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_slash:
    if (input == '/')
    {
      req->http_version_major = 0;
      req->http_version_minor = 0;
      state_ = http_version_major_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req->http_version_major = req->http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req->http_version_major = req->http_version_major * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req->http_version_minor = req->http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req->http_version_minor = req->http_version_minor * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return boost::indeterminate;
    }
    else if (!req->headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req->headers.push_back(header());
      req->headers.back().name.push_back(input);
      state_ = header_name;
      return boost::indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = header_value;
      (*req)(input);
      return boost::indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req->headers.back().name.push_back(input);
      return boost::indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_value:
    if (input == '\r')
    {
      LOG_MEM << "Header\t" << req->headers.back().name << ":" << req->headers.back().value;
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      (*req)(input);
      return boost::indeterminate;
    }

    case expecting_newline_2:
        if (input == '\n')
        {
            state_ = header_line_start;
            return boost::indeterminate;
        }
        return false;

    case expecting_newline_3:
        if (req->boundary_marker.length() == 0)
            return true;
        else if (input == '\n')
        {
            state_ = search_boundary;
            req->on_form_data();
            return boost::indeterminate;
        }
        state_ = inspecting_boundary;
        boundary_char_ = 2;
        req->reset_offset();
        // fall through

    case inspecting_boundary:
    {
        std::string::size_type const len = req->boundary_marker.length();
        if (input == req->boundary_marker[boundary_char_])
        {
            ++boundary_char_;
            if (boundary_char_ == len)
                state_ = header_line_start;
        }
        else if (boundary_char_ == len-2  &&  input == '-')
            state_ = boundary_terminator_1;
        else
        {
            for (std::string::const_iterator it=req->boundary_marker.begin();
                 it !=req->boundary_marker.begin()+boundary_char_;
                 ++it)
            {
                (*req)(*it);
            }
            (*req)(input);
            state_ = search_boundary;
        }
        return boost::indeterminate;
    }

    case search_boundary:
        if (input == req->boundary_marker[0])
        {
            boundary_char_ = 1;
            state_ = inspecting_boundary;
        }
        else
            (*req)(input);
        return boost::indeterminate;

    case boundary_terminator_1:
        if (input == '-')
        {
            state_ = boundary_terminator_2;
            return boost::indeterminate;
        }
        return false;

    case boundary_terminator_2:
        if (input == '\r')
        {
            state_ = boundary_terminator_3;
            return boost::indeterminate;
        }
        return false;

    case boundary_terminator_3:
        if (input == '\n')
        {
            state_ = header_line_start;
            return boost::indeterminate;
        }
        return false;

    default:
        return false;
    }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server3
} // namespace http
