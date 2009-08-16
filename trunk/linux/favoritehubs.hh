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

#ifndef WULFOR_FAVORITE_HUBS_HH
#define WULFOR_FAVORITE_HUBS_HH

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include <dcpp/FavoriteManager.h>

#include "bookentry.hh"
#include "treeview.hh"

using namespace dcpp;

class FavoriteHubs:
	public BookEntry,
	public FavoriteManagerListener
{
	public:
		FavoriteHubs();
		virtual ~FavoriteHubs();
		virtual void show();

	private:
		// GUI functions
		void addEntry_gui(StringMap params);
		void editEntry_gui(StringMap &params, GtkTreeIter *iter);
		void removeEntry_gui(std::string address);
		void showErrorDialog_gui(const std::string &description);
		void popupMenu_gui();

		// GUI callbacks
		static gboolean onButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onKeyReleased_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static void onAddEntry_gui(GtkWidget *widget, gpointer data);
		static void onEditEntry_gui(GtkWidget *widget, gpointer data);
		static void onRemoveEntry_gui(GtkWidget *widget, gpointer data);
		static void onConnect_gui(GtkButton *widget, gpointer data);
		static void onToggledClicked_gui(GtkCellRendererToggle *cell, gchar *path, gpointer data);

		// Client functions
		void initializeList_client();
		void getFavHubParams_client(const FavoriteHubEntry *entry, StringMap &params);
		void addEntry_client(StringMap params);
		void editEntry_client(string address, StringMap params);
		void removeEntry_client(string address);
		void setConnect_client(string address, bool active);

		// Client callbacks
		virtual void on(FavoriteManagerListener::FavoriteAdded, const FavoriteHubEntryPtr entry) throw();
		virtual void on(FavoriteManagerListener::FavoriteRemoved, const FavoriteHubEntryPtr entry) throw();

		TreeView favoriteView;
		GtkListStore *favoriteStore;
		GtkTreeSelection *favoriteSelection;
		GdkEventType previous;
};

#else
class FavoriteHubs;
#endif
