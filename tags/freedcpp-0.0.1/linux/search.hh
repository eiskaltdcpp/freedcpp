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

#ifndef WULFOR_SEARCH_HH
#define WULFOR_SEARCH_HH

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include <dcpp/ClientManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/SearchResult.h>

#include "bookentry.hh"
#include "treeview.hh"

using namespace dcpp;

class UserCommandMenu;

class Search:
	public BookEntry,
	public SearchManagerListener,
	public ClientManagerListener
{
	public:
		Search();
		virtual ~Search();
		virtual void show();

		void putValue_gui(const std::string &str, int64_t size, SearchManager::SizeModes mode, SearchManager::TypeModes type);

	private:
		// Keep these and the items in .glade file in same order, otherwise it will break
		typedef enum
		{
			NOGROUPING = 0,
			FILENAME,
			FILEPATH,
			SIZE,
			CONNECTION,
			TTH,
			NICK,
			HUB,
			TYPE
		} GroupType;

		// GUI functions
		void initHubs_gui();
		void addHub_gui(std::string name, std::string url, bool op);
		void modifyHub_gui(std::string name, std::string url, bool op);
		void removeHub_gui(std::string url);
		void popupMenu_gui();
		void setStatus_gui(std::string statusBar, std::string text);
		void search_gui();
		void parseSearchResult(SearchResult *result, StringMap &resultMap, GdkPixbuf **icon, int *actualSlots);
		void addResult_gui(SearchResult *result, bool inShare);
		void clearList_gui();
		void updateParentRow_gui(GtkTreeIter *parent, GtkTreeIter *child = NULL);
		void ungroup_gui();
		void regroup_gui();
		std::string getGroupingColumn(GroupType groupBy);

		// GUI callbacks
		static gboolean onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data);
		static gboolean onButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onKeyReleased_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static gboolean onSearchEntryKeyPressed_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static gboolean searchFilterFunc_gui(GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
		static void onComboBoxChanged_gui(GtkWidget *widget, gpointer data);
		static void onGroupByComboBoxChanged_gui(GtkWidget* widget, gpointer data);
		static void onSearchButtonClicked_gui(GtkWidget *widget, gpointer data);
		static void onFilterButtonToggled_gui(GtkToggleButton *button, gpointer data);
		static void onSlotsButtonToggled_gui(GtkToggleButton *button, gpointer data);
		static void onOpButtonToggled_gui(GtkToggleButton *button, gpointer data);
		static void onSharedButtonToggled_gui(GtkToggleButton *button, gpointer data);
		static void onToggledClicked_gui(GtkCellRendererToggle *cell, gchar *path, gpointer data);
		static void onDownloadClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadFavoriteClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadToClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadToMatchClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadDirClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadFavoriteDirClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadDirToClicked_gui(GtkMenuItem *item, gpointer data);
		static void onSearchByTTHClicked_gui(GtkMenuItem *item, gpointer data);
		static void onCopyMagnetClicked_gui(GtkMenuItem *item, gpointer data);
		static void onGetFileListClicked_gui(GtkMenuItem *item, gpointer data);
		static void onMatchQueueClicked_gui(GtkMenuItem *item, gpointer data);
		static void onPrivateMessageClicked_gui(GtkMenuItem *item, gpointer data);
		static void onAddFavoriteUserClicked_gui(GtkMenuItem *item, gpointer data);
		static void onGrantExtraSlotClicked_gui(GtkMenuItem *item, gpointer data);
		static void onRemoveUserFromQueueClicked_gui(GtkMenuItem *item, gpointer data);
		static void onRemoveClicked_gui(GtkMenuItem *item, gpointer data);

		// Client functions
		void download_client(std::string target, SearchResult *result);
		void downloadDir_client(std::string target, SearchResult *result);
		void addSource_client(std::string source, SearchResult *result);
		void getFileList_client(std::string cid, std::string dir, bool match);
		void addFavUser_client(std::string cid);
		void grantSlot_client(std::string cid);
		void removeSource_client(std::string cid);

		// Client callbacks
		virtual void on(ClientManagerListener::ClientConnected, Client *client) throw();
	 	virtual void on(ClientManagerListener::ClientUpdated, Client *client) throw();
		virtual void on(ClientManagerListener::ClientDisconnected, Client *client) throw();
		virtual void on(SearchManagerListener::SR, const SearchResultPtr& result) throw();

		TreeView hubView, resultView;
		GtkListStore *hubStore;
		GtkTreeStore *resultStore;
		GtkTreeModel *searchFilterModel;
		GtkTreeModel *sortedFilterModel;
		GtkTreeSelection *selection;
		GdkEventType oldEventType;
		GtkWidget *searchEntry;
		TStringList searchlist;
		static GtkTreeModel *searchEntriesModel;
		GdkPixbuf *iconFile;
		GdkPixbuf *iconDirectory;
		GdkPixbuf *iconGroup;
		int droppedResult;
		int searchHits;
		bool isHash;
		bool onlyFree;
		static bool onlyOp;
		UserCommandMenu *userCommandMenu;
		GroupType previousGrouping;
};

#else
class Search;
#endif
