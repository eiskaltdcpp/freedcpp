/*
 * Copyright © 2009, freedcpp
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
#include "WulforUtil.hh"
#include "wulformanager.hh"
#include "previewmenu.hh"

using namespace std;
using namespace dcpp;

void PreviewMenu::cleanMenu_gui()
{
	gtk_container_foreach(GTK_CONTAINER(appsPreviewMenu), (GtkCallback)gtk_widget_destroy, NULL);
}

bool PreviewMenu::buildMenu_gui(const string &target)
{
	if (target == "none" || target.empty())
		return FALSE;

	string file = Util::getFileName(target);
	string ext = Util::getFileExt(file);

	if (ext == ".dctmp")
	{
		string::size_type i = file.find('.');
		string::size_type j = file.find('.', i + 1);

		j != string::npos ? ext = file.substr(i, j - i) : ext = "";
	}

	if (ext.empty() || ext == ".")
		return FALSE;

	ext.erase(0, 1);

	GtkWidget* itemApp;
	string appExtensions = "";
	ext = Text::toLower(ext);

	const PreviewApp::List &Apps = WulforSettingsManager::getInstance()->getPreviewApps();

	for (PreviewApp::Iter item = Apps.begin(); item != Apps.end(); ++item)
	{
		appExtensions = Text::toLower((*item)->ext);

		if (appExtensions.find(ext) != string::npos)
		{
			itemApp = gtk_menu_item_new_with_label(((*item)->name).c_str());

			gtk_menu_shell_append(GTK_MENU_SHELL(appsPreviewMenu), itemApp);

			g_signal_connect(itemApp, "activate", G_CALLBACK(onPreviewAppClicked_gui), (gpointer) this);

			g_object_set_data_full(G_OBJECT(itemApp), "command", g_strdup("application"), g_free);
			g_object_set_data_full(G_OBJECT(itemApp), "application", g_strdup(((*item)->app).c_str()), g_free);
			g_object_set_data_full(G_OBJECT(itemApp), "target", g_strdup(target.c_str()), g_free);
		}
	}

	itemApp = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(appsPreviewMenu), itemApp);

	itemApp = gtk_menu_item_new_with_label(_("Default"));
	gtk_menu_shell_append(GTK_MENU_SHELL(appsPreviewMenu), itemApp);

	g_signal_connect(itemApp, "activate", G_CALLBACK(onPreviewAppClicked_gui), (gpointer) this);

	g_object_set_data_full(G_OBJECT(itemApp), "command", g_strdup("default"), g_free);
	g_object_set_data_full(G_OBJECT(itemApp), "application", g_strdup(""), g_free);
	g_object_set_data_full(G_OBJECT(itemApp), "target", g_strdup(target.c_str()), g_free);

	gtk_widget_show_all(appsPreviewMenu);

	return TRUE;
}

void PreviewMenu::onPreviewAppClicked_gui(GtkMenuItem *menuItem, gpointer data)
{
	string command = (gchar *) g_object_get_data(G_OBJECT(menuItem), "command");
	string application = (gchar *) g_object_get_data(G_OBJECT(menuItem), "application");
	string target = (gchar *) g_object_get_data(G_OBJECT(menuItem), "target");

	if(command == "default") WulforUtil::openURI(target);
	else
	{
		string cmd = application + " \"" + target + "\"";
		WulforUtil::openURItoApp(cmd);
	}
}
