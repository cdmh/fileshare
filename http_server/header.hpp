//
// header.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_HEADER_HPP
#define HTTP_SERVER3_HEADER_HPP

#include <vector>
#include <iomanip>
#include <boost/function.hpp>
#include "logging.h"

namespace http {
namespace server3 {

struct header
{
  std::string name;
  std::string value;

  header()
  {
  }
  
  header(std::string const &name, std::string const &value)
    : name(name), value(value)
  {
  }
  
  header(std::string const &name, char const &chr)
    : name(name)
  {
    value = chr;
  }
  
  header(std::string const &name)
    : name(name)
  {
  }
};

typedef std::vector<header> headers_t;

class connection;

typedef
boost::function<bool const (headers_t const &, std::string const &, headers_t &, std::string &, boost::shared_ptr<connection> const &)>
http_query_callback;

} // namespace server3

} // namespace http

#endif // HTTP_SERVER3_HEADER_HPP
