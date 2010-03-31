/*
 * Copyright (C) 2001-2009 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SETTINGS_MANAGER_H
#define DCPLUSPLUS_DCPP_SETTINGS_MANAGER_H

#include "Util.h"
#include "Speaker.h"
#include "Singleton.h"

namespace dcpp {

class SimpleXML;

class SettingsManagerListener {
public:
	virtual ~SettingsManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Load;
	typedef X<1> Save;

	virtual void on(Load, SimpleXML&) throw() { }
	virtual void on(Save, SimpleXML&) throw() { }
};

class SettingsManager : public Singleton<SettingsManager>, public Speaker<SettingsManagerListener>
{
public:

	static StringList connectionSpeeds;

	enum StrSetting { STR_FIRST,
		NICK = STR_FIRST, UPLOAD_SPEED, DESCRIPTION, DOWNLOAD_DIRECTORY, EMAIL, EXTERNAL_IP,
		TEXT_FONT, CONNECTIONS_ORDER, CONNECTIONS_WIDTHS, HUBFRAME_ORDER, HUBFRAME_WIDTHS,
		SEARCHFRAME_ORDER, SEARCHFRAME_WIDTHS, FAVHUBSFRAME_ORDER, FAVHUBSFRAME_WIDTHS,
		HUBLIST_SERVERS, QUEUEFRAME_ORDER, QUEUEFRAME_WIDTHS, PUBLICHUBSFRAME_ORDER, PUBLICHUBSFRAME_WIDTHS,
		FINISHED_DL_FILES_ORDER, FINISHED_DL_FILES_WIDTHS, FINISHED_DL_USERS_ORDER, FINISHED_DL_USERS_WIDTHS,
		FINISHED_UL_FILES_ORDER, FINISHED_UL_FILES_WIDTHS, FINISHED_UL_USERS_ORDER, FINISHED_UL_USERS_WIDTHS,
		USERSFRAME_ORDER, USERSFRAME_WIDTHS, HTTP_PROXY, LOG_DIRECTORY, LOG_FORMAT_POST_DOWNLOAD,
		LOG_FORMAT_POST_UPLOAD, LOG_FORMAT_MAIN_CHAT, LOG_FORMAT_PRIVATE_CHAT,
		TEMP_DOWNLOAD_DIRECTORY, BIND_ADDRESS, SOCKS_SERVER, SOCKS_USER, SOCKS_PASSWORD, CONFIG_VERSION,
		DEFAULT_AWAY_MESSAGE, TIME_STAMPS_FORMAT, ADLSEARCHFRAME_ORDER, ADLSEARCHFRAME_WIDTHS,
		PRIVATE_ID, SPYFRAME_WIDTHS, SPYFRAME_ORDER, LOG_FILE_MAIN_CHAT,
		LOG_FILE_PRIVATE_CHAT, LOG_FILE_STATUS, LOG_FILE_UPLOAD, LOG_FILE_DOWNLOAD, LOG_FILE_SYSTEM,
		LOG_FORMAT_SYSTEM, LOG_FORMAT_STATUS, DIRECTORYLISTINGFRAME_ORDER, DIRECTORYLISTINGFRAME_WIDTHS,
		TLS_PRIVATE_KEY_FILE, TLS_CERTIFICATE_FILE, TLS_TRUSTED_CERTIFICATES_PATH,
		LANGUAGE, DOWNLOADS_ORDER, DOWNLOADS_WIDTHS,
		SOUND_MAIN_CHAT, SOUND_PM, SOUND_PM_WINDOW,
		STR_LAST };

	enum IntSetting { INT_FIRST = STR_LAST + 1,
		INCOMING_CONNECTIONS = INT_FIRST, TCP_PORT, SLOTS, AUTO_FOLLOW, CLEAR_SEARCH,
		BACKGROUND_COLOR, TEXT_COLOR, USE_OEM_MONOFONT, SHARE_HIDDEN, FILTER_MESSAGES, MINIMIZE_TRAY, ALWAYS_TRAY,
		AUTO_SEARCH, TIME_STAMPS, POPUP_HUB_PMS, POPUP_BOT_PMS, IGNORE_HUB_PMS, IGNORE_BOT_PMS,
		LIST_DUPES, BUFFER_SIZE, DOWNLOAD_SLOTS, MAX_DOWNLOAD_SPEED, LOG_MAIN_CHAT, LOG_PRIVATE_CHAT,
		LOG_DOWNLOADS, LOG_UPLOADS, STATUS_IN_CHAT, SHOW_JOINS,
		USE_SYSTEM_ICONS, POPUP_PMS, MIN_UPLOAD_SPEED, GET_USER_INFO, URL_HANDLER, MAIN_WINDOW_STATE,
		MAIN_WINDOW_SIZE_X, MAIN_WINDOW_SIZE_Y, MAIN_WINDOW_POS_X, MAIN_WINDOW_POS_Y, AUTO_AWAY,
		SOCKS_PORT, SOCKS_RESOLVE, KEEP_LISTS, AUTO_KICK, QUEUEFRAME_SHOW_TREE,
		COMPRESS_TRANSFERS, SFV_CHECK,
		MAX_COMPRESSION, NO_AWAYMSG_TO_BOTS, SKIP_ZERO_BYTE, ADLS_BREAK_ON_FIRST,
		HUB_USER_COMMANDS, AUTO_SEARCH_AUTO_MATCH, UPLOAD_BAR_COLOR, DOWNLOAD_BAR_COLOR, LOG_SYSTEM,
		LOG_FILELIST_TRANSFERS, SEND_UNKNOWN_COMMANDS, MAX_HASH_SPEED, OPEN_USER_CMD_HELP,
		GET_USER_COUNTRY, FAV_SHOW_JOINS, LOG_STATUS_MESSAGES, SHOW_STATUSBAR,
		SHOW_TOOLBAR, SHOW_TRANSFERVIEW, POPUNDER_PM, POPUNDER_FILELIST, MAGNET_ASK, MAGNET_ACTION, MAGNET_REGISTER,
		ADD_FINISHED_INSTANTLY, DONT_DL_ALREADY_SHARED, USE_CTRL_FOR_LINE_HISTORY,
		JOIN_OPEN_NEW_WINDOW, UDP_PORT, SHOW_LAST_LINES_LOG,
		ADC_DEBUG, TOGGLE_ACTIVE_WINDOW, SEARCH_HISTORY, SET_MINISLOT_SIZE, MAX_FILELIST_SIZE,
		PRIO_HIGHEST_SIZE, PRIO_HIGH_SIZE, PRIO_NORMAL_SIZE, PRIO_LOW_SIZE, PRIO_LOWEST,
		AUTODROP_SPEED, AUTODROP_INTERVAL, AUTODROP_ELAPSED, AUTODROP_INACTIVITY, AUTODROP_MINSOURCES, AUTODROP_FILESIZE,
		AUTODROP_ALL, AUTODROP_FILELISTS, AUTODROP_DISCONNECT,
		OPEN_PUBLIC, OPEN_FAVORITE_HUBS, OPEN_FAVORITE_USERS, OPEN_QUEUE, OPEN_FINISHED_DOWNLOADS,
		OPEN_FINISHED_UPLOADS, OPEN_SEARCH_SPY, OPEN_NETWORK_STATISTICS, OPEN_NOTEPAD, OUTGOING_CONNECTIONS,
		NO_IP_OVERRIDE, SEARCH_ONLY_FREE_SLOTS, LAST_SEARCH_TYPE, BOLD_FINISHED_DOWNLOADS, BOLD_FINISHED_UPLOADS, BOLD_QUEUE,
		BOLD_HUB, BOLD_PM, BOLD_SEARCH, BOLD_SEARCH_SPY, SOCKET_IN_BUFFER, SOCKET_OUT_BUFFER,
		OPEN_WAITING_USERS, BOLD_WAITING_USERS, OPEN_SYSTEM_LOG, BOLD_SYSTEM_LOG, AUTO_REFRESH_TIME,
		USE_TLS, AUTO_SEARCH_LIMIT, ALT_SORT_ORDER, AUTO_KICK_NO_FAVS, PROMPT_PASSWORD, SPY_FRAME_IGNORE_TTH_SEARCHES,
		DONT_DL_ALREADY_QUEUED, MAX_COMMAND_LENGTH, ALLOW_UNTRUSTED_HUBS, ALLOW_UNTRUSTED_CLIENTS,
		TLS_PORT, FAST_HASH, SORT_FAVUSERS_FIRST, SEGMENTED_DL, FOLLOW_LINKS,
		SEND_BLOOM, OWNER_DRAWN_MENUS, CORAL, SEARCH_FILTER_SHARED, MAX_TAB_CHARS, FINISHED_DL_ONLY_FULL,
		CONFIRM_EXIT, CONFIRM_HUB_CLOSING, CONFIRM_HUB_REMOVAL, CONFIRM_USER_REMOVAL, CONFIRM_ITEM_REMOVAL, CONFIRM_ADLS_REMOVAL,
		SEARCH_MERGE,
		INT_LAST };

	enum Int64Setting { INT64_FIRST = INT_LAST + 1,
		TOTAL_UPLOAD = INT64_FIRST, TOTAL_DOWNLOAD,
		INT64_LAST };

	enum FloatSetting { FLOAT_FIRST = INT64_LAST +1,
		TRANSFERS_PANED_POS = FLOAT_FIRST, QUEUE_PANED_POS, SEARCH_PANED_POS,
		FLOAT_LAST, SETTINGS_LAST = FLOAT_LAST };

	enum {	INCOMING_DIRECT, INCOMING_FIREWALL_UPNP, INCOMING_FIREWALL_NAT,
		INCOMING_FIREWALL_PASSIVE };
	enum {	OUTGOING_DIRECT, OUTGOING_SOCKS5 };

	enum {	MAGNET_AUTO_SEARCH, MAGNET_AUTO_DOWNLOAD };

	const string& get(StrSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? strSettings[key - STR_FIRST] : strDefaults[key - STR_FIRST];
	}

	int get(IntSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? intSettings[key - INT_FIRST] : intDefaults[key - INT_FIRST];
	}
	int64_t get(Int64Setting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? int64Settings[key - INT64_FIRST] : int64Defaults[key - INT64_FIRST];
	}
	float get(FloatSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? floatSettings[key - FLOAT_FIRST] : floatDefaults[key - FLOAT_FIRST];
	}

	bool getBool(IntSetting key, bool useDefault = true) const {
		return (get(key, useDefault) != 0);
	}

	void set(StrSetting key, string const& value) {
		if(((key == DESCRIPTION) || (key == NICK)) && (value.size() > 35)) {
			strSettings[key - STR_FIRST] = value.substr(0, 35);
		} else {
			strSettings[key - STR_FIRST] = value;
		}
		isSet[key] = !value.empty();
	}

	void set(IntSetting key, int value) {
		if((key == SLOTS) && (value <= 0)) {
			value = 1;
		}
		intSettings[key - INT_FIRST] = value;
		isSet[key] = true;
	}

	void set(IntSetting key, const string& value) {
		if(value.empty()) {
			intSettings[key - INT_FIRST] = 0;
			isSet[key] = false;
		} else {
			intSettings[key - INT_FIRST] = Util::toInt(value);
			isSet[key] = true;
		}
	}

	void set(Int64Setting key, int64_t value) {
		int64Settings[key - INT64_FIRST] = value;
		isSet[key] = true;
	}

	void set(Int64Setting key, const string& value) {
		if(value.empty()) {
			int64Settings[key - INT64_FIRST] = 0;
			isSet[key] = false;
		} else {
			int64Settings[key - INT64_FIRST] = Util::toInt64(value);
			isSet[key] = true;
		}
	}

	void set(IntSetting key, bool value) { set(key, (int)value); }

	void set(FloatSetting key, float value) {
		floatSettings[key - FLOAT_FIRST] = value;
		isSet[key] = true;
	}
	void set(FloatSetting key, double value) {
		// yes, we loose precision here, but we're gonna loose even more when saving to the XML file...
		floatSettings[key - FLOAT_FIRST] = static_cast<float>(value);
		isSet[key] = true;
	}

	void setDefault(StrSetting key, string const& value) {
		strDefaults[key - STR_FIRST] = value;
	}

	void setDefault(IntSetting key, int value) {
		intDefaults[key - INT_FIRST] = value;
	}
	void setDefault(Int64Setting key, int64_t value) {
		int64Defaults[key - INT64_FIRST] = value;
	}
	void setDefault(FloatSetting key, float value) {
		floatDefaults[key - FLOAT_FIRST] = value;
	}

	bool isDefault(int aSet) { return !isSet[aSet]; }

	void load() {
		Util::migrate(getConfigFile());
		load(getConfigFile());
	}
	void save() {
		save(getConfigFile());
	}

	void load(const string& aFileName);
	void save(const string& aFileName);

private:
	friend class Singleton<SettingsManager>;
	SettingsManager();
	virtual ~SettingsManager() throw() { }

	static const string settingTags[SETTINGS_LAST+1];

	string strSettings[STR_LAST - STR_FIRST];
	int    intSettings[INT_LAST - INT_FIRST];
	int64_t int64Settings[INT64_LAST - INT64_FIRST];
	float floatSettings[FLOAT_LAST - FLOAT_FIRST];

	string strDefaults[STR_LAST - STR_FIRST];
	int    intDefaults[INT_LAST - INT_FIRST];
	int64_t int64Defaults[INT64_LAST - INT64_FIRST];
	float floatDefaults[FLOAT_LAST - FLOAT_FIRST];

	bool isSet[SETTINGS_LAST];

	string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "DCPlusPlus.xml"; }
};

// Shorthand accessor macros
#define SETTING(k) (SettingsManager::getInstance()->get(SettingsManager::k, true))
#define BOOLSETTING(k) (SettingsManager::getInstance()->getBool(SettingsManager::k, true))

} // namespace dcpp

#endif // !defined(SETTINGS_MANAGER_H)
