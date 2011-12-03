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

#ifndef DCPLUSPLUS_DCPP_HTTP_CONNECTION_H
#define DCPLUSPLUS_DCPP_HTTP_CONNECTION_H

#include "BufferedSocketListener.h"
#include "HttpConnectionListener.h"
#include "Speaker.h"

namespace dcpp {

using std::string;

class HttpConnection : BufferedSocketListener, public Speaker<HttpConnectionListener>, boost::noncopyable
{
public:
	HttpConnection(bool coralize = true);
	virtual ~HttpConnection();

	void downloadFile(const string& aUrl);

private:
	enum CoralizeState { CST_DEFAULT, CST_CONNECTED, CST_NOCORALIZE };

	string currentUrl;
	string file;
	string server;
	bool ok;
	string port;
	int64_t size;
	bool moved302;

	CoralizeState coralizeState;

	BufferedSocket* socket;

	// BufferedSocketListener
	void on(Connected) noexcept;
	void on(Line, const string&) noexcept;
	void on(Data, uint8_t*, size_t) noexcept;
	void on(ModeChange) noexcept;
	void on(Failed, const string&) noexcept;

	void onConnected();
	void onLine(const string& aLine);
};

} // namespace dcpp

#endif // !defined(HTTP_CONNECTION_H)
