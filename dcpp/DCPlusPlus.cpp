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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "ADLSearch.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "DownloadManager.h"
#include "FavoriteManager.h"
#include "FinishedManager.h"
#include "GeoManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "QueueManager.h"
#include "ResourceManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "WindowManager.h"

#ifdef _STLP_DEBUG
void __stl_debug_terminate() {
	int* x = 0;
	*x = 0;
}
#endif

extern "C" int _nl_msg_cat_cntr;

namespace dcpp {

void startup(void (*f)(void*, const string&), void* p) {
	// "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
	// Nev's great contribution to dc++
	while(1) break;


#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");

	ResourceManager::newInstance();
	SettingsManager::newInstance();

	LogManager::newInstance();
	TimerManager::newInstance();
	HashManager::newInstance();
	CryptoManager::newInstance();
	SearchManager::newInstance();
	ClientManager::newInstance();
	ConnectionManager::newInstance();
	DownloadManager::newInstance();
	UploadManager::newInstance();
	ThrottleManager::newInstance();
	QueueManager::newInstance();
	ShareManager::newInstance();
	FavoriteManager::newInstance();
	FinishedManager::newInstance();
	ADLSearchManager::newInstance();
	ConnectivityManager::newInstance();
	MappingManager::newInstance();
///[-]	GeoManager::newInstance();
	WindowManager::newInstance();

	SettingsManager::getInstance()->load();

#ifdef _WIN32
	if(!SETTING(LANGUAGE).empty()) {
		string language = "LANGUAGE=" + SETTING(LANGUAGE);
		putenv(language.c_str());

		// Apparently this is supposted to make gettext reload the message catalog...
		_nl_msg_cat_cntr++;
	}
#endif

	auto announce = [&f, &p](const string& str) {
		if(f)
			(*f)(p, str);
	};

	announce(_("Users"));
	ClientManager::getInstance()->loadUsers();
	FavoriteManager::getInstance()->load();

	announce(_("Security certificates"));
	CryptoManager::getInstance()->loadCertificates();

	announce(_("Hash database"));
	HashManager::getInstance()->startup();

	announce(_("Shared Files"));
	ShareManager::getInstance()->refresh(true, false, true);

	announce(_("Download Queue"));
	QueueManager::getInstance()->loadQueue();

///[-]	if(BOOLSETTING(GET_USER_COUNTRY)) {
///[-]		announce(_("Country information"));
///[-]		GeoManager::getInstance()->init();
///[-]	}
}

void shutdown() {

#ifndef _WIN32 //*nix system

	ThrottleManager::getInstance()->shutdown();
#endif
	TimerManager::getInstance()->shutdown();
	HashManager::getInstance()->shutdown();
#ifdef _WIN32
	ThrottleManager::getInstance()->shutdown();
#endif
///	TimerManager::getInstance()->shutdown();
///	HashManager::getInstance()->shutdown();
///	ThrottleManager::getInstance()->shutdown();
	ConnectionManager::getInstance()->shutdown();
	MappingManager::getInstance()->close();
///[-]	GeoManager::getInstance()->close();
	BufferedSocket::waitShutdown();

	WindowManager::getInstance()->prepareSave();
	QueueManager::getInstance()->saveQueue(true);
	ClientManager::getInstance()->saveUsers();
	SettingsManager::getInstance()->save();

	WindowManager::deleteInstance();
///[-] 	GeoManager::deleteInstance();
	MappingManager::deleteInstance();
	ConnectivityManager::deleteInstance();
	ADLSearchManager::deleteInstance();
	FinishedManager::deleteInstance();
	ShareManager::deleteInstance();
	CryptoManager::deleteInstance();
	ThrottleManager::deleteInstance();
	DownloadManager::deleteInstance();
	UploadManager::deleteInstance();
	QueueManager::deleteInstance();
	ConnectionManager::deleteInstance();
	SearchManager::deleteInstance();
	FavoriteManager::deleteInstance();
	ClientManager::deleteInstance();
	HashManager::deleteInstance();
	LogManager::deleteInstance();
	SettingsManager::deleteInstance();
	TimerManager::deleteInstance();
	ResourceManager::deleteInstance();

#ifdef _WIN32
	::WSACleanup();
#endif
}

} // namespace dcpp
