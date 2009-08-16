/*
 * Copyright © 2004-2008 Jens Oknelid, paskharen@gmail.com
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 */

#include "settingsmanager.hh"

#include <glib.h>
#include <dcpp/File.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/Util.h>
#include "WulforUtil.hh"

using namespace std;

WulforSettingsManager::WulforSettingsManager():
	configFile(Util::getConfigPath() + "LinuxDC++.xml")
{
	defaultInt["main-window-maximized"] = 0;
	defaultInt["main-window-size-x"] = 875;
	defaultInt["main-window-size-y"] = 685;
	defaultInt["main-window-pos-x"] = 100;
	defaultInt["main-window-pos-y"] = 100;
	defaultInt["transfer-pane-position"] = 482;
	defaultInt["nick-pane-position"] = 500;
	defaultInt["downloadqueue-pane-position"] = 200;
	defaultInt["sharebrowser-pane-position"] = 200;
	defaultInt["use-stock-icons"] = 0;
	defaultInt["tab-position"] = 0;
	defaultInt["toolbar-style"] = 5;
	defaultString["downloadqueue-order"] = "";
	defaultString["downloadqueue-width"] = "";
	defaultString["downloadqueue-visibility"] = "";
	defaultString["favoritehubs-order"] = "";
	defaultString["favoritehubs-width"] = "";
	defaultString["favoritehubs-visibility"] = "";
	defaultString["finished-order"] = "";
	defaultString["finished-width"] = "";
	defaultString["finished-visibility"] = "";
	defaultString["hub-order"] = "";
	defaultString["hub-width"] = "";
	defaultString["hub-visibility"] = "";
	defaultString["main-order"] = "";
	defaultString["main-width"] = "";
	defaultString["main-visibility"] = "";
	defaultString["publichubs-order"] = "";
	defaultString["publichubs-width"] = "";
	defaultString["publichubs-visibility"] = "";
	defaultString["search-order"] = "";
	defaultString["search-width"] = "";
	defaultString["search-visibility"] = "";
	defaultString["sharebrowser-order"] = "";
	defaultString["sharebrowser-width"] = "";
	defaultString["sharebrowser-visibility"] = "";
	defaultString["default-charset"] = WulforUtil::ENCODING_SYSTEM_DEFAULT;

	load();
}

WulforSettingsManager::~WulforSettingsManager()
{
	save();
}

int WulforSettingsManager::getInt(const string &key)
{
	dcassert(intMap.find(key) != intMap.end() || defaultInt.find(key) != defaultInt.end());

	if (intMap.find(key) == intMap.end())
		return defaultInt[key];
	else
		return intMap[key];
}

string WulforSettingsManager::getString(const string &key)
{
	dcassert(stringMap.find(key) != stringMap.end() || defaultString.find(key) != defaultString.end());

	if (stringMap.find(key) == stringMap.end())
		return defaultString[key];
	else
		return stringMap[key];
}

void WulforSettingsManager::set(const string &key, int value)
{
	dcassert(defaultInt.find(key) != defaultInt.end());
	intMap[key] = value;
}

void WulforSettingsManager::set(const string &key, const string &value)
{
	dcassert(defaultString.find(key) != defaultString.end());
	stringMap[key] = value;
}

void WulforSettingsManager::load()
{
	try
	{
		SimpleXML xml;
		xml.fromXML(File(configFile, File::READ, File::OPEN).read());
		xml.resetCurrentChild();
		xml.stepIn();

		if (xml.findChild("Settings"))
		{
			xml.stepIn();

			map<string, int>::iterator iit;
			for (iit = defaultInt.begin(); iit != defaultInt.end(); ++iit)
			{
				if (xml.findChild(iit->first))
					intMap[iit->first] = Util::toInt(xml.getChildData());
				xml.resetCurrentChild();
			}

			map<string, string>::iterator sit;
			for (sit = defaultString.begin(); sit != defaultString.end(); ++sit)
			{
				if (xml.findChild(sit->first))
					stringMap[sit->first] = xml.getChildData();
				xml.resetCurrentChild();
			}

			xml.stepOut();
		}
	}
	catch (const Exception&)
	{
	}
}

void WulforSettingsManager::save()
{
	SimpleXML xml;
	xml.addTag(g_get_application_name());
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	map<std::string, int>::iterator iit;
	for (iit = intMap.begin(); iit != intMap.end(); ++iit)
	{
		xml.addTag(iit->first, iit->second);
		xml.addChildAttrib(string("type"), string("int"));
	}

	map<std::string, std::string>::iterator sit;
	for (sit = stringMap.begin(); sit != stringMap.end(); ++sit)
	{
		xml.addTag(sit->first, sit->second);
		xml.addChildAttrib(string("type"), string("string"));
	}

	xml.stepOut();

	try
	{
		File out(configFile + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(configFile);
		File::renameFile(configFile + ".tmp", configFile);
	}
	catch (const FileException &)
	{
	}
}
