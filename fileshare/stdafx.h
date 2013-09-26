// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "../http_server/platform.h"

#include <iostream>
#include <time.h>
#include <map>

#include "../http_server/header.hpp"
#include "../http_server/server.hpp"

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

inline void trace(char const *, ...) {}
#define TRACE trace
