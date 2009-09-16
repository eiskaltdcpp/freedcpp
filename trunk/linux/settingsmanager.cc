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
using namespace dcpp;

WulforSettingsManager::WulforSettingsManager():
	configFile(Util::getConfigPath() + "FreeDC++.xml")
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
	defaultInt["sound-pm-open"] = 0;
	defaultInt["sound-pm"] = 1;
	defaultInt["use-magnet-split"] = 1;
	defaultInt["text-general-bold"] = TEXT_WEIGHT_NORMAL;
	defaultInt["text-general-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-myown-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-myown-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-private-bold"] = TEXT_WEIGHT_NORMAL;
	defaultInt["text-private-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-system-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-system-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-status-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-status-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-timestamp-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-timestamp-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-mynick-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-mynick-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-fav-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-fav-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-op-bold"] = TEXT_WEIGHT_BOLD;
	defaultInt["text-op-italic"] = TEXT_STYLE_NORMAL;
	defaultInt["text-url-bold"] = TEXT_WEIGHT_NORMAL;
	defaultInt["text-url-italic"] = TEXT_STYLE_NORMAL;
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
	defaultString["transfers-order"] = "";
	defaultString["transfers-width"] = "";
	defaultString["transfers-visibility"] = "";
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
	defaultString["sound-download-begins"] = "";
	defaultString["sound-download-finished"] = "";
	defaultString["sound-upload-finished"] = "";
	defaultString["sound-private-message"] = "";
	defaultString["sound-hub-connect"] = "";
	defaultString["sound-hub-disconnect"] = "";
	defaultString["text-general-back-color"] = "#FFFFFF";
	defaultString["text-general-fore-color"] = "#4D4D4D";
	defaultString["text-myown-back-color"] = "#FFFFFF";
	defaultString["text-myown-fore-color"] = "#207505";
	defaultString["text-private-back-color"] = "#FFFFFF";
	defaultString["text-private-fore-color"] = "#2763CE";
	defaultString["text-system-back-color"] = "#FFFFFF";
	defaultString["text-system-fore-color"] = "#1A1A1A";
	defaultString["text-status-back-color"] = "#FFFFFF";
	defaultString["text-status-fore-color"] = "#7F7F7F";
	defaultString["text-timestamp-back-color"] = "#FFFFFF";
	defaultString["text-timestamp-fore-color"] = "#43629A";
	defaultString["text-mynick-back-color"] = "#FFFFFF";
	defaultString["text-mynick-fore-color"] = "#A52A2A";
	defaultString["text-fav-back-color"] = "#FFFFFF";
	defaultString["text-fav-fore-color"] = "#FFA500";
	defaultString["text-op-back-color"] = "#FFFFFF";
	defaultString["text-op-fore-color"] = "#0000FF";
	defaultString["text-url-back-color"] = "#FFFFFF";
	defaultString["text-url-fore-color"] = "#0000FF";

	load();
}

WulforSettingsManager::~WulforSettingsManager()
{
	save();

	for_each(previewApps.begin(), previewApps.end(), DeleteFunction());
}

int WulforSettingsManager::getInt(const string &key, bool useDefault)
{
	dcassert(intMap.find(key) != intMap.end() || defaultInt.find(key) != defaultInt.end());

	if (useDefault)
		return defaultInt[key];

	if (intMap.find(key) == intMap.end())
		return defaultInt[key];
	else
		return intMap[key];
}

string WulforSettingsManager::getString(const string &key, bool useDefault)
{
	dcassert(stringMap.find(key) != stringMap.end() || defaultString.find(key) != defaultString.end());

	if (useDefault)
		return defaultString[key];

	if (stringMap.find(key) == stringMap.end())
		return defaultString[key];
	else
		return stringMap[key];
}

bool WulforSettingsManager::getBool(const string &key, bool useDefault)
{
	return (getInt(key, useDefault) != 0);
}

void WulforSettingsManager::set(const string &key, int value)
{
	dcassert(defaultInt.find(key) != defaultInt.end());
	intMap[key] = value;
}

void WulforSettingsManager::set(const string &key, bool value)
{
	set(key, (int)value);
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

		if (xml.findChild("PreviewApps"))
		{
			xml.stepIn();

			for (;xml.findChild("Application");)
				addPreviewApp(xml.getChildAttrib("Name"), xml.getChildAttrib("Application"), xml.getChildAttrib("Extension"));

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

	xml.addTag("PreviewApps");
	xml.stepIn();

	for(PreviewApp::Iter i = previewApps.begin(); i != previewApps.end(); ++i)
	{
		xml.addTag("Application");
		xml.addChildAttrib("Name", (*i)->name);
		xml.addChildAttrib("Application", (*i)->app);
		xml.addChildAttrib("Extension", (*i)->ext);
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

PreviewApp* WulforSettingsManager::addPreviewApp(string name, string app, string ext)
{
	PreviewApp* pa = new PreviewApp(name, app, ext);
	previewApps.push_back(pa);

	return pa;
}

bool WulforSettingsManager::removePreviewApp(string &name)
{
	PreviewApp::size index;

	if (getPreviewApp(name, index))
	{
		delete previewApps[index];
		previewApps.erase(previewApps.begin() + index);

		return true;
	}

	return false;
}

PreviewApp* WulforSettingsManager::applyPreviewApp(string &oldName, string &newName, string &app, string &ext)
{
	PreviewApp::size index;
	PreviewApp *pa = NULL;

	if(getPreviewApp(oldName, index))
	{
		delete previewApps[index];
		pa = new PreviewApp(newName, app, ext);
		previewApps[index] = pa;
	}

	return pa;
}

bool WulforSettingsManager::getPreviewApp(string &name)
{
	PreviewApp::size index;
	return getPreviewApp(name, index);
}

bool WulforSettingsManager::getPreviewApp(string &name, PreviewApp::size &index)
{
	index = 0;

	for (PreviewApp::Iter item = previewApps.begin(); item != previewApps.end(); ++item, ++index)
		if((*item)->name == name) return true;

	return false;
}
