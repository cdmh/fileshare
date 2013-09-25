// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <iostream>
#include <time.h>
#include <map>

#include "../http_server/header.hpp"
#include "../http_server/server.hpp"

#include <windows.h>
#include <winhttp.h>    // WinHttpCloseHandle etc
#include "..\..\DbgLib\DbgLib.h"

#pragma comment(lib, "ws2_32.lib")

#undef max
#undef min

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
