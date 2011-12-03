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

#ifndef DCPLUSPLUS_DCPP_TYPEDEFS_H_
#define DCPLUSPLUS_DCPP_TYPEDEFS_H_

#include "forward.h"

#include <cstdint>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/variant.hpp>

namespace dcpp {

using std::pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::wstring;

typedef vector<string> StringList;
typedef StringList::iterator StringIter;
typedef StringList::const_iterator StringIterC;

typedef pair<string, string> StringPair;
typedef vector<StringPair> StringPairList;
typedef StringPairList::iterator StringPairIter;

typedef unordered_map<string, string> StringMap;
typedef StringMap::iterator StringMapIter;

typedef unordered_set<string> StringSet;
typedef StringSet::iterator StringSetIter;

typedef vector<wstring> WStringList;
typedef WStringList::iterator WStringIter;
typedef WStringList::const_iterator WStringIterC;

typedef pair<wstring, wstring> WStringPair;
typedef vector<WStringPair> WStringPairList;
typedef WStringPairList::iterator WStringPairIter;

typedef unordered_map<wstring, wstring> WStringMap;
typedef WStringMap::iterator WStringMapIter;

typedef vector<uint8_t> ByteVector;

#ifdef UNICODE

typedef wstring tstring;
typedef WStringList TStringList;
typedef WStringIter TStringIter;
typedef WStringIterC TStringIterC;

typedef WStringPair TStringPair;
typedef WStringPairIter TStringPairIter;
typedef WStringPairList TStringPairList;

typedef WStringMap TStringMap;
typedef WStringMapIter TStringMapIter;

#else

typedef string tstring;
typedef StringList TStringList;
typedef StringIter TStringIter;
typedef StringIterC TStringIterC;

typedef StringPair TStringPair;
typedef StringPairIter TStringPairIter;
typedef StringPairList TStringPairList;

typedef StringMap TStringMap;
typedef StringMapIter TStringMapIter;

#endif

typedef vector<DownloadPtr> DownloadList;
typedef vector<FavoriteHubEntryPtr> FavoriteHubEntryList;
typedef vector<HintedUser> HintedUserList;
typedef vector<HubEntry> HubEntryList;
typedef vector<OnlineUserPtr> OnlineUserList;
typedef vector<SearchResultPtr> SearchResultList;
typedef vector<UploadPtr> UploadList;
typedef vector<UserPtr> UserList;
typedef vector<UserConnectionPtr> UserConnectionList;

typedef unordered_map<string, boost::variant<string, std::function<string ()>>> ParamMap;

}

#endif /* TYPEDEFS_H_ */
