/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_CRITICAL_SECTION_H
#define DCPLUSPLUS_DCPP_CRITICAL_SECTION_H

#include <boost/thread/recursive_mutex.hpp>

namespace dcpp {

// TODO: At some point (probably when c++0x support is solid enough), remove these old names
typedef boost::recursive_mutex CriticalSection;
typedef boost::detail::spinlock	FastCriticalSection;
typedef boost::unique_lock<boost::recursive_mutex> Lock;
typedef boost::lock_guard<boost::detail::spinlock> FastLock;

} // namespace dcpp

#endif // DCPLUSPLUS_DCPP_CRITICAL_SECTION_H
