/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_STDINC_H
#define DCPLUSPLUS_DCPP_STDINC_H

// This enables stlport's debug mode (and slows it down to a crawl...)
//#define _STLP_DEBUG 1
//#define _STLP_USE_NEWALLOC 1

// --- Shouldn't have to change anything under here...

#ifndef _REENTRANT
# define _REENTRANT 1
#endif

#ifndef BZ_NO_STDIO
#define BZ_NO_STDIO 1
#endif

#ifdef _MSC_VER

//disable the deprecated warnings for the CRT functions.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1

# ifndef CDECL
#  define CDECL _cdecl
# endif

#else // _MSC_VER

# ifndef CDECL
#  define CDECL
# endif

#endif // _MSC_VER

#ifdef _WIN32
# define _WIN32_WINNT 0x0502
# define _WIN32_IE	0x0501
# define WINVER 0x501

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>

#include <windows.h>
#include <mmsystem.h>

#include <tchar.h>
#include <shlobj.h>

#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#include <crtdbg.h>
#else
#include <assert.h>
#endif

#include <cctype>
#include <clocale>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>

#include <memory.h>
#include <sys/types.h>

#ifdef __MINGW32__
/* the shared_ptr implementation provided by MinGW / GCC 4.5's libstdc++ consumes too many
semaphores, so we prefer boost's one. see <https://bugs.launchpad.net/dcplusplus/+bug/654040>. */
#define _SHARED_PTR_H 1 // skip libstdc++'s bits/shared_ptr.h
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
using boost::shared_ptr;
using boost::enable_shared_from_this;
using boost::make_shared;
#endif

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <list>
#include <utility>
#include <functional>
#include <memory>
#include <numeric>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include <libintl.h>

#include <boost/format.hpp>
#include <boost/scoped_array.hpp>
#include <boost/noncopyable.hpp>

#include "nullptr.h"

namespace dcpp {
using namespace std;
}

#endif // !defined(STDINC_H)
