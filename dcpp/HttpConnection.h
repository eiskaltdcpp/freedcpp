/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 * Copyright (C) 2019 Boris Pek <tehnick-8@yandex.ru>
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

#pragma once

#include "BufferedSocket.h"
#include "HttpConnectionListener.h"
#include "Speaker.h"
#include "Util.h"

#include <boost/noncopyable.hpp>

namespace dcpp {

using std::string;
class HttpConnection;

class HttpConnection : BufferedSocketListener, public Speaker<HttpConnectionListener>, private boost::noncopyable
{
public:
	HttpConnection(const string& aUserAgent = Util::emptyString);
	virtual ~HttpConnection();

	void downloadFile(const string& aUrl);
	void postData(const string& aUrl, const StringMap& aData);

	const string& getMimeType() const { return mimeType; }

	int64_t getSize() const { return size; }
	int64_t getDone() const { return done; }

private:
	enum RequestType { TYPE_GET, TYPE_POST };
	enum ConnectionStates { CONN_UNKNOWN, CONN_OK, CONN_FAILED, CONN_MOVED, CONN_CHUNKED };

	string currentUrl;
	string userAgent;
	string method;
	string file;
	string server;
	string port;

	string requestBody;

	string mimeType;
	int64_t size;
	int64_t done;

	ConnectionStates connState;
	RequestType connType;

	BufferedSocket* socket;

	void prepareRequest(RequestType type);
	void abortRequest(bool disconnect);

	// BufferedSocketListener
	void on(Connected) noexcept;
	void on(Line, const string&) noexcept;
	void on(Data, uint8_t*, size_t) noexcept;
	void on(ModeChange) noexcept;
	void on(Failed, const string&) noexcept;
};

} // namespace dcpp

