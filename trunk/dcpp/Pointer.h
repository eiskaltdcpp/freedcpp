/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_POINTER_H
#define DCPLUSPLUS_DCPP_POINTER_H

#include <boost/intrusive_ptr.hpp>
#include "Thread.h"

namespace dcpp {

class intrusive_ptr_base
{
public:
	void inc() throw() {
		dcassert(ref>=0);
		Thread::safeInc(ref);
	}

	void dec() throw() {
		dcassert(ref>0);

		if ( Thread::safeDec(ref) == 0 ) {
			delete this;
		}
	}
	bool unique() throw() {
		return (ref == 1);
	}

protected:
	intrusive_ptr_base() throw() : ref(0) { }

	virtual ~intrusive_ptr_base() throw() {
		dcassert(!ref);
	}

private:
	volatile long ref;
};

inline void intrusive_ptr_add_ref(intrusive_ptr_base* p) { p->inc(); }
inline void intrusive_ptr_release(intrusive_ptr_base* p) { p->dec(); }

struct DeleteFunction {
	template<typename T>
	void operator()(const T& p) const { delete p; }
};

} // namespace dcpp

#endif // !defined(POINTER_H)
