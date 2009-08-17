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

#include "search.hh"

#include <dcpp/FavoriteManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/Text.h>
#include <dcpp/UserCommand.h>
#include "UserCommandMenu.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

GtkTreeModel* Search::searchEntriesModel = NULL;

Search::Search():
	BookEntry(Entry::SEARCH, _("Search: "), "search.glade", generateID()),
	previousGrouping(NOGROUPING)
{
	// Initialize the search entries combo box
	if (searchEntriesModel == NULL)
		searchEntriesModel = gtk_combo_box_get_model(GTK_COMBO_BOX(getWidget("comboboxentrySearch")));
	gtk_combo_box_set_model(GTK_COMBO_BOX(getWidget("comboboxentrySearch")), searchEntriesModel);
	searchEntry = gtk_bin_get_child(GTK_BIN(getWidget("comboboxentrySearch")));
	gtk_widget_grab_focus(getWidget("comboboxentrySearch"));

	// Configure the dialog
	File::ensureDirectory(SETTING(DOWNLOAD_DIRECTORY));
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(getWidget("dirChooserDialog")), Text::fromUtf8(SETTING(DOWNLOAD_DIRECTORY)).c_str());
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("dirChooserDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	// Initialize check button options.
	onlyFree = BOOLSETTING(SEARCH_ONLY_FREE_SLOTS);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(getWidget("checkbuttonSlots")), onlyFree);
	gtk_widget_set_sensitive(GTK_WIDGET(getWidget("checkbuttonSlots")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(getWidget("checkbuttonShared")), FALSE);

	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxSize")), 1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxUnit")), 2);
	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxFile")), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxGroupBy")), (int)NOGROUPING);

	// Load icons
	iconFile = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		GTK_STOCK_FILE, 16, (GtkIconLookupFlags)0, NULL);
	iconDirectory = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		GTK_STOCK_DIRECTORY, 16, (GtkIconLookupFlags)0, NULL);
	iconGroup = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
		GTK_STOCK_DND_MULTIPLE, 16, (GtkIconLookupFlags)0, NULL);

	// Initialize hub list treeview
	hubView.setView(GTK_TREE_VIEW(getWidget("treeviewHubs")));
	hubView.insertColumn("Search", G_TYPE_BOOLEAN, TreeView::BOOL, -1);
	hubView.insertColumn("Name", G_TYPE_STRING, TreeView::STRING, -1);
	hubView.insertHiddenColumn("Url", G_TYPE_STRING);
	hubView.finalize();
	hubStore = gtk_list_store_newv(hubView.getColCount(), hubView.getGTypes());
	gtk_tree_view_set_model(hubView.get(), GTK_TREE_MODEL(hubStore));
	g_object_unref(hubStore);
	GtkTreeViewColumn *col = gtk_tree_view_get_column(hubView.get(), hubView.col("Search"));
	GList *list = gtk_tree_view_column_get_cell_renderers(col);
	GtkCellRenderer *renderer = (GtkCellRenderer *)g_list_nth_data(list, 0);
	g_list_free(list);

	// Initialize search result treeview
	resultView.setView(GTK_TREE_VIEW(getWidget("treeviewResult")), TRUE, "search");
	resultView.insertColumn("Filename", G_TYPE_STRING, TreeView::PIXBUF_STRING, 250, "Icon");
	resultView.insertColumn("Nick", G_TYPE_STRING, TreeView::STRING, 100);
	resultView.insertColumn("Type", G_TYPE_STRING, TreeView::STRING, 65);
	resultView.insertColumn("Size", G_TYPE_STRING, TreeView::STRING, 80);
	resultView.insertColumn("Path", G_TYPE_STRING, TreeView::STRING, 100);
	resultView.insertColumn("Slots", G_TYPE_STRING, TreeView::STRING, 50);
	resultView.insertColumn("Connection", G_TYPE_STRING, TreeView::STRING, 90);
	resultView.insertColumn("Hub", G_TYPE_STRING, TreeView::STRING, 150);
	resultView.insertColumn("Exact Size", G_TYPE_STRING, TreeView::STRING, 80);
	resultView.insertColumn("IP", G_TYPE_STRING, TreeView::STRING, 100);
	resultView.insertColumn("TTH", G_TYPE_STRING, TreeView::STRING, 125);
	resultView.insertHiddenColumn("Icon", GDK_TYPE_PIXBUF);
	resultView.insertHiddenColumn("Real Size", G_TYPE_INT64);
	resultView.insertHiddenColumn("Slots Order", G_TYPE_INT);
	resultView.insertHiddenColumn("File Order", G_TYPE_STRING);
	resultView.insertHiddenColumn("SearchResult", G_TYPE_POINTER);
	resultView.insertHiddenColumn("Hub URL", G_TYPE_STRING);
	resultView.insertHiddenColumn("CID", G_TYPE_STRING);
	resultView.insertHiddenColumn("Shared", G_TYPE_BOOLEAN);
	resultView.insertHiddenColumn("Grouping String", G_TYPE_STRING);
	resultView.finalize();
	resultStore = gtk_tree_store_newv(resultView.getColCount(), resultView.getGTypes());
	searchFilterModel = gtk_tree_model_filter_new(GTK_TREE_MODEL(resultStore), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(searchFilterModel), &Search::searchFilterFunc_gui, (gpointer)this, NULL);
	sortedFilterModel = gtk_tree_model_sort_new_with_model(searchFilterModel);
	gtk_tree_view_set_model(resultView.get(), sortedFilterModel);
	g_object_unref(resultStore);
	g_object_unref(searchFilterModel);
	g_object_unref(sortedFilterModel);
	selection = gtk_tree_view_get_selection(resultView.get());
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	resultView.setSortColumn_gui("Size", "Real Size");
	resultView.setSortColumn_gui("Exact Size", "Real Size");
	resultView.setSortColumn_gui("Slots", "Slots Order");
	resultView.setSortColumn_gui("Filename", "File Order");
	gtk_tree_view_set_fixed_height_mode(resultView.get(), TRUE);

	// Initialize the user command menu
	userCommandMenu = new UserCommandMenu(getWidget("usercommandMenu"), ::UserCommand::CONTEXT_SEARCH);
	addChild(userCommandMenu);

	// Connect the signals to their callback functions.
	g_signal_connect(getContainer(), "focus-in-event", G_CALLBACK(onFocusIn_gui), (gpointer)this);
	g_signal_connect(getWidget("checkbuttonFilter"), "toggled", G_CALLBACK(onFilterButtonToggled_gui), (gpointer)this);
	g_signal_connect(getWidget("checkbuttonSlots"), "toggled", G_CALLBACK(onSlotsButtonToggled_gui), (gpointer)this);
	g_signal_connect(getWidget("checkbuttonShared"), "toggled", G_CALLBACK(onSharedButtonToggled_gui), (gpointer)this);
	g_signal_connect(renderer, "toggled", G_CALLBACK(onToggledClicked_gui), (gpointer)this);
	g_signal_connect(resultView.get(), "button-press-event", G_CALLBACK(onButtonPressed_gui), (gpointer)this);
	g_signal_connect(resultView.get(), "button-release-event", G_CALLBACK(onButtonReleased_gui), (gpointer)this);
	g_signal_connect(resultView.get(), "key-release-event", G_CALLBACK(onKeyReleased_gui), (gpointer)this);
	g_signal_connect(searchEntry, "key-press-event", G_CALLBACK(onSearchEntryKeyPressed_gui), (gpointer)this);
	g_signal_connect(searchEntry, "key-release-event", G_CALLBACK(onKeyReleased_gui), (gpointer)this);
	g_signal_connect(getWidget("entrySize"), "key-release-event", G_CALLBACK(onKeyReleased_gui), (gpointer)this);
	g_signal_connect(getWidget("buttonSearch"), "clicked", G_CALLBACK(onSearchButtonClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("downloadItem"), "activate", G_CALLBACK(onDownloadClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("downloadWholeDirItem"), "activate", G_CALLBACK(onDownloadDirClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("searchByTTHItem"), "activate", G_CALLBACK(onSearchByTTHClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyMagnetItem"), "activate", G_CALLBACK(onCopyMagnetClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("getFileListItem"), "activate", G_CALLBACK(onGetFileListClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("matchQueueItem"), "activate", G_CALLBACK(onMatchQueueClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("sendPrivateMessageItem"), "activate", G_CALLBACK(onPrivateMessageClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("addToFavoritesItem"), "activate", G_CALLBACK(onAddFavoriteUserClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("grantExtraSlotItem"), "activate", G_CALLBACK(onGrantExtraSlotClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("removeUserFromQueueItem"), "activate", G_CALLBACK(onRemoveUserFromQueueClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("removeItem"), "activate", G_CALLBACK(onRemoveClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("comboboxSize"), "changed", G_CALLBACK(onComboBoxChanged_gui), (gpointer)this);
	g_signal_connect(getWidget("comboboxentrySearch"), "changed", G_CALLBACK(onComboBoxChanged_gui), (gpointer)this);
	g_signal_connect(getWidget("comboboxUnit"), "changed", G_CALLBACK(onComboBoxChanged_gui), (gpointer)this);
	g_signal_connect(getWidget("comboboxFile"), "changed", G_CALLBACK(onComboBoxChanged_gui), (gpointer)this);
	g_signal_connect(getWidget("comboboxGroupBy"), "changed", G_CALLBACK(onGroupByComboBoxChanged_gui), (gpointer)this);
}

Search::~Search()
{
	ClientManager::getInstance()->removeListener(this);
	SearchManager::getInstance()->removeListener(this);

	clearList_gui();
	gtk_widget_destroy(getWidget("dirChooserDialog"));

	if (iconFile)
		g_object_unref(iconFile);
	if (iconDirectory)
		g_object_unref(iconDirectory);
	if (iconGroup)
		g_object_unref(iconGroup);
}

void Search::show()
{
	initHubs_gui();
	ClientManager::getInstance()->addListener(this);
	SearchManager::getInstance()->addListener(this);
}

void Search::putValue_gui(const string &str, int64_t size, SearchManager::SizeModes mode, SearchManager::TypeModes type)
{
	gtk_entry_set_text(GTK_ENTRY(searchEntry), str.c_str());
	gtk_entry_set_text(GTK_ENTRY(getWidget("entrySize")), Util::toString(size).c_str());
	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxSize")), (int)mode);
	gtk_combo_box_set_active(GTK_COMBO_BOX(getWidget("comboboxFile")), (int)type);

	search_gui();
}

void Search::initHubs_gui()
{
	ClientManager::getInstance()->lock();

	Client::List& clients = ClientManager::getInstance()->getClients();

	Client *client = NULL;
	for (Client::List::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		client = *it;
		if (client->isConnected())
			addHub_gui(client->getHubName(), client->getHubUrl());
	}

	ClientManager::getInstance()->unlock();
}

void Search::addHub_gui(string name, string url)
{
	GtkTreeIter iter;
	gtk_list_store_append(hubStore, &iter);
	gtk_list_store_set(hubStore, &iter,
		hubView.col("Search"), TRUE,
		hubView.col("Name"), name.empty() ? url.c_str() : name.c_str(),
		hubView.col("Url"), url.c_str(),
		-1);
}

void Search::modifyHub_gui(string name, string url)
{
	GtkTreeIter iter;
	GtkTreeModel *m = GTK_TREE_MODEL(hubStore);
	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);

	while (valid)
	{
		if (url == hubView.getString(&iter, "Url"))
		{
			gtk_list_store_set(hubStore, &iter,
				hubView.col("Name"), name.empty() ? url.c_str() : name.c_str(),
				hubView.col("Url"), url.c_str(),
				-1);
			return;
		}
		valid = gtk_tree_model_iter_next(m, &iter);
	}
}

void Search::removeHub_gui(string url)
{
	GtkTreeIter iter;
	GtkTreeModel *m = GTK_TREE_MODEL(hubStore);
	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);

	while (valid)
	{
		if (url == hubView.getString(&iter, "Url"))
		{
			gtk_list_store_remove(hubStore, &iter);
			return;
		}
		valid = gtk_tree_model_iter_next(m, &iter);
	}
}

void Search::popupMenu_gui()
{
	GtkTreeIter iter;
	GtkTreePath *path;
	GList *list = gtk_tree_selection_get_selected_rows(selection, NULL);
	guint count = g_list_length(list);

	if (count == 1)
	{
		path = (GtkTreePath*)list->data; // This will be freed later

		// If it is a parent effectively more than one row is selected
		if (gtk_tree_model_get_iter(sortedFilterModel, &iter, path) &&
		    gtk_tree_model_iter_has_child(sortedFilterModel, &iter))
		{
			gtk_widget_set_sensitive(getWidget("searchByTTHItem"), FALSE);
		}
		else
		{
			gtk_widget_set_sensitive(getWidget("searchByTTHItem"), TRUE);
		}
	}
	else if (count > 1)
	{
		gtk_widget_set_sensitive(getWidget("searchByTTHItem"), FALSE);
	}

	GtkWidget *menuItem;
	string tth;
	bool firstTTH;
	bool hasTTH;

	// Clean menus
	gtk_container_foreach(GTK_CONTAINER(getWidget("downloadMenu")), (GtkCallback)gtk_widget_destroy, NULL);
	gtk_container_foreach(GTK_CONTAINER(getWidget("downloadDirMenu")), (GtkCallback)gtk_widget_destroy, NULL);
	userCommandMenu->cleanMenu_gui();

	// Build "Download to..." submenu

	// Add favorite download directories
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if (spl.size() > 0)
	{
		for (StringPairIter i = spl.begin(); i != spl.end(); i++)
		{
			menuItem = gtk_menu_item_new_with_label(i->second.c_str());
			g_object_set_data_full(G_OBJECT(menuItem), "fav", g_strdup(i->first.c_str()), g_free);
			g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadFavoriteClicked_gui), (gpointer)this);
			gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
		}
		menuItem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
	}

	// Add Browse item
	menuItem = gtk_menu_item_new_with_label(_("Browse..."));
	g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadToClicked_gui), (gpointer)this);
	gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);

	// Add search results with the same TTH to menu
	firstTTH = TRUE;
	hasTTH = FALSE;

	for (GList *i = list; i; i = i->next)
	{
		path = (GtkTreePath *)i->data;
		if (gtk_tree_model_get_iter(sortedFilterModel, &iter, path))
		{
			userCommandMenu->addHub(resultView.getString(&iter, "Hub URL"));
			userCommandMenu->addFile(resultView.getString(&iter, "CID"),
				resultView.getString(&iter, "Filename"),
				resultView.getValue<int64_t>(&iter, "Real Size"),
				resultView.getString(&iter, "TTH"));

			if (firstTTH)
			{
				tth = resultView.getString(&iter, "TTH");
				firstTTH = FALSE;
				hasTTH = TRUE;
			}
			else if (hasTTH)
			{
				if (tth.empty() || tth != resultView.getString(&iter, "TTH"))
					hasTTH = FALSE; // Can't break here since we have to free all the paths
			}
		}
		gtk_tree_path_free(path);
	}
	g_list_free(list);

	if (hasTTH)
	{
		StringList targets;
		QueueManager::getInstance()->getTargets(TTHValue(tth), targets);

		if (targets.size() > static_cast<size_t>(0))
		{
			menuItem = gtk_separator_menu_item_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
			for (StringIter i = targets.begin(); i != targets.end(); ++i)
			{
				menuItem = gtk_menu_item_new_with_label(i->c_str());
				g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadToMatchClicked_gui), (gpointer)this);
				gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadMenu")), menuItem);
			}
		}
	}

	// Build "Download whole directory to..." submenu

	spl.clear();
	spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if (spl.size() > 0)
	{
		for (StringPairIter i = spl.begin(); i != spl.end(); i++)
		{
			menuItem = gtk_menu_item_new_with_label(i->second.c_str());
			g_object_set_data_full(G_OBJECT(menuItem), "fav", g_strdup(i->first.c_str()), g_free);
			g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadFavoriteDirClicked_gui), (gpointer)this);
			gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);
		}
		menuItem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);
	}

	menuItem = gtk_menu_item_new_with_label(_("Browse..."));
	g_signal_connect(menuItem, "activate", G_CALLBACK(onDownloadDirToClicked_gui), (gpointer)this);
	gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("downloadDirMenu")), menuItem);

	// Build user command menu
	userCommandMenu->buildMenu_gui();

	gtk_menu_popup(GTK_MENU(getWidget("mainMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	gtk_widget_show_all(getWidget("mainMenu"));
}

void Search::setStatus_gui(string statusBar, string text)
{
	gtk_statusbar_pop(GTK_STATUSBAR(getWidget(statusBar)), 0);
	gtk_statusbar_push(GTK_STATUSBAR(getWidget(statusBar)), 0, text.c_str());
}

void Search::search_gui()
{
	StringList clients;
	GtkTreeIter iter;

	string text = gtk_entry_get_text(GTK_ENTRY(searchEntry));
	if (text.empty())
		return;

	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(hubStore), &iter);
 	while (valid)
 	{
 		if (hubView.getValue<gboolean>(&iter, "Search"))
 			clients.push_back(hubView.getString(&iter, "Url"));
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(hubStore), &iter);
	}

	if (clients.size() < 1)
		return;

	double lsize = Util::toDouble(gtk_entry_get_text(GTK_ENTRY(getWidget("entrySize"))));

	switch (gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxUnit"))))
	{
		case 1:
			lsize *= 1024.0;
			break;
		case 2:
			lsize *= 1024.0 * 1024.0;
			break;
		case 3:
			lsize *= 1024.0 * 1024.0 * 1024.0;
			break;
	}

	clearList_gui();
	int64_t llsize = static_cast<int64_t>(lsize);
	searchlist = StringTokenizer<string>(text, ' ').getTokens();

	// Strip out terms beginning with -
	text.clear();
	for (StringList::const_iterator si = searchlist.begin(); si != searchlist.end(); ++si)
		if ((*si)[0] != '-')
			text += *si + ' ';
	text = text.substr(0, std::max(text.size(), static_cast<string::size_type>(1)) - 1);

	SearchManager::SizeModes mode((SearchManager::SizeModes)gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxSize"))));
	if (llsize == 0)
		mode = SearchManager::SIZE_DONTCARE;

	int ftype = gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxFile")));
	isHash = (ftype == SearchManager::TYPE_TTH);

	// Add new searches to the dropdown list
	GtkListStore *store = GTK_LIST_STORE(searchEntriesModel);
	size_t max = std::max(SETTING(SEARCH_HISTORY) - 1, 0);
	size_t count = 0;
	gchar *entry;
	valid = gtk_tree_model_get_iter_first(searchEntriesModel, &iter);
	while (valid)
	{
		gtk_tree_model_get(searchEntriesModel, &iter, 0, &entry, -1);
		if (text == string(entry) || count >= max)
			valid = gtk_list_store_remove(store, &iter);
		else
			valid = gtk_tree_model_iter_next(searchEntriesModel, &iter);
		count++;
		g_free(entry);
	}

	gtk_list_store_prepend(store, &iter);
	gtk_list_store_set(store, &iter, 0, text.c_str(), -1);

	droppedResult = 0;
	searchHits = 0;
	setStatus_gui("statusbar1", _("Searching for ") + text + " ...");
	setStatus_gui("statusbar2", _("0 items"));
	setStatus_gui("statusbar3", _("0 filtered"));
	setLabel_gui(_("Search: ") + text);

	if (SearchManager::getInstance()->okToSearch())
	{
		SearchManager::getInstance()->search(clients, text, llsize, (SearchManager::TypeModes)ftype, mode, "manual");

		if (BOOLSETTING(CLEAR_SEARCH)) // Only clear if the search was sent.
			gtk_entry_set_text(GTK_ENTRY(searchEntry), "");
	}
	else
	{
		int32_t waitFor = SearchManager::getInstance()->timeToSearch();
		string line = _("Searching too soon, retry in ") + Util::toString(waitFor) + " s";
		setStatus_gui("statusbar1", line);
		setStatus_gui("statusbar2", "");
		setStatus_gui("statusbar3", "");
	}
}

void Search::parseSearchResult(SearchResult *result, StringMap &resultMap, GdkPixbuf **icon, int *actualSlots)
{
	if (result->getType() == SearchResult::TYPE_FILE)
	{
		string file = WulforUtil::linuxSeparator(result->getFile());
		if (file.rfind('/') == tstring::npos)
		{
			resultMap["Filename"] = file;
		}
		else
		{
			resultMap["Filename"] = Util::getFileName(file);
			resultMap["Path"] = Util::getFilePath(file);
		}

		resultMap["File Order"] = "f" + resultMap["Filename"];
		resultMap["Type"] = Util::getFileExt(resultMap["Filename"]);
		if (!resultMap["Type"].empty() && resultMap["Type"][0] == '.')
			resultMap["Type"].erase(0, 1);
		resultMap["Size"] = Util::formatBytes(result->getSize());
		resultMap["Exact Size"] = Util::formatExactSize(result->getSize());
		*icon = iconFile;
	}
	else
	{
		resultMap["Filename"] = WulforUtil::linuxSeparator(result->getFileName());
		resultMap["Path"] = WulforUtil::linuxSeparator(result->getFile());
		resultMap["File Order"] = "d" + resultMap["Filename"];
		resultMap["Type"] = _("Directory");
		*icon = iconDirectory;
		if (result->getSize() > 0)
		{
			resultMap["Size"] = Util::formatBytes(result->getSize());
			resultMap["Exact Size"] = Util::formatExactSize(result->getSize());
		}
	}

	resultMap["Nick"] = WulforUtil::getNicks(result->getUser());
	resultMap["CID"] = result->getUser()->getCID().toBase32();
	resultMap["Slots"] = result->getSlotString();
	resultMap["Connection"] = ClientManager::getInstance()->getConnection(result->getUser()->getCID());
	resultMap["Hub"] = result->getHubName().empty() ? result->getHubURL().c_str() : result->getHubName().c_str();
	resultMap["Hub URL"] = result->getHubURL();
	resultMap["IP"] = result->getIP();
	if (result->getType() == SearchResult::TYPE_FILE)
		resultMap["TTH"] = result->getTTH().toBase32();

	// assumption: total slots is never above 999
	*actualSlots = -1000 * result->getFreeSlots() - result->getSlots();
}

void Search::addResult_gui(SearchResult *result, bool isShared)
{
	if (!result)
		return;

	StringMap resultMap;
	GdkPixbuf *icon;
	int actualSlots;

	parseSearchResult(result, resultMap, &icon, &actualSlots);

	GroupType groupBy = (GroupType)gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxGroupBy")));
	string groupColumn = getGroupingColumn(groupBy);
	string groupStr = resultMap[groupColumn];

	// Check that it's not a duplicate and find parent for grouping
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreeIter child;
	GtkTreeModel *m = GTK_TREE_MODEL(resultStore);
	bool foundParent = FALSE;
	bool createParent = FALSE;

	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);
	while (valid)
	{
		// Check if it's a duplicate
		SearchResult *result2 = resultView.getValue<SearchResult *>(&iter, "SearchResult", m);

		if (result2 != NULL && result->getUser()->getCID() == result2->getUser()->getCID() &&
		    result->getFile() == result2->getFile())
		{
			result->dec();
			return;
		}

		// Find grouping parent
		if (!foundParent && groupBy != NOGROUPING && !groupStr.empty() &&
		    resultView.getString(&iter, "Grouping String", m) == groupStr)
		{
			// Parent row
			if (gtk_tree_model_iter_has_child(m, &iter))
			{
				parent = iter;
				foundParent = TRUE;
			}
			// If two rows that match the grouping criteria
			// are found, group them under a new parent row
			else if (!foundParent)
			{
				child = iter;
				createParent = TRUE;
				foundParent = TRUE;
			}
		}

		valid = WulforUtil::getNextIter_gui(m, &iter);
	}

	// Move top level row to be under newly created grouping parent.
	// This needs to be done outside of the loop so that we don't modify the 
	// tree until after the duplication check.
	if (createParent)
	{
		// Insert the new parent row
		gtk_tree_store_insert_with_values(resultStore, &parent, NULL, -1,
				resultView.col("SearchResult"), NULL,
				resultView.col("Icon"), iconGroup,
				resultView.col("Grouping String"), groupStr.c_str(),
				-1);

		// Move the row to be a child of the new parent
		WulforUtil::copyRow_gui(resultStore, &child, &parent);
		gtk_tree_store_remove(resultStore, &child);
	}

	// Have to use insert with values since appending would cause searchFilterFunc to be
	// called with empty row which in turn will cause assert failure in treeview::getString
	gtk_tree_store_insert_with_values(resultStore, &iter, foundParent ? &parent : NULL, -1,
		resultView.col("Nick"), resultMap["Nick"].c_str(),
		resultView.col("Filename"), resultMap["Filename"].c_str(),
		resultView.col("Slots"), resultMap["Slots"].c_str(),
		resultView.col("Size"), resultMap["Size"].c_str(),
		resultView.col("Path"), resultMap["Path"].c_str(),
		resultView.col("Type"), resultMap["Type"].c_str(),
		resultView.col("Connection"), resultMap["Connection"].c_str(),
		resultView.col("Hub"), resultMap["Hub"].c_str(),
		resultView.col("Exact Size"), resultMap["Exact Size"].c_str(),
		resultView.col("IP"), resultMap["IP"].c_str(),
		resultView.col("TTH"), resultMap["TTH"].c_str(),
		resultView.col("Icon"), icon,
		resultView.col("File Order"), resultMap["File Order"].c_str(),
		resultView.col("Real Size"), result->getSize(),
		resultView.col("Slots Order"), actualSlots,
		resultView.col("SearchResult"), (gpointer)result,
		resultView.col("Hub URL"), resultMap["Hub URL"].c_str(),
		resultView.col("CID"), resultMap["CID"].c_str(),
		resultView.col("Shared"), isShared,
		resultView.col("Grouping String"), groupStr.c_str(),
		-1);

	if (foundParent)
		updateParentRow_gui(&parent, &iter);

	++searchHits;
	setStatus_gui("statusbar2", Util::toString(searchHits) + _(" items"));

	if (BOOLSETTING(BOLD_SEARCH))
		setBold_gui();
}

void Search::clearList_gui()
{
	GtkTreeIter iter;
	SearchResult *result = NULL;
	GtkTreeModel *m = GTK_TREE_MODEL(resultStore);
	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);
	int cleared = 0;

	while (valid)
	{
		result = resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult", m);

		if (result)
		{
			result->dec();
			result = NULL;
			cleared++;
		}
		valid = WulforUtil::getNextIter_gui(m, &iter, TRUE);
	}

	dcdebug("Search::clearList_gui(), decReffed: %d results\n", cleared);
	gtk_tree_store_clear(resultStore);
}

void Search::updateParentRow_gui(GtkTreeIter *parent, GtkTreeIter *child)
{
	// Let's make sure we really have children...
	gint children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(resultStore), parent);
	dcassert(children != 0);

	string groupStr = resultView.getString(parent, "Grouping String", GTK_TREE_MODEL(resultStore));
	string groupDesc = "[" + Util::toString(children) + "] " + groupStr;
	gtk_tree_store_set(resultStore, parent, resultView.col("Filename"), groupDesc.c_str(), -1);

	if (child == NULL)
		return;

	GroupType groupType = (GroupType)gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxGroupBy")));

	switch (groupType)
	{
		case NOGROUPING:
			break;
		case FILENAME:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("File Order"));
			break;
		case FILEPATH:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Path"));
			break;
		case SIZE:
		{
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Exact Size"));
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Size"));
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Real Size"));
			break;
		}
		case CONNECTION:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Connection"));
			break;
		case TTH:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("TTH"));
			break;
		case NICK:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Nick"));
			break;
		case HUB:
		{
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Hub"));
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Hub URL"));
			break;
		}
		case TYPE:
			WulforUtil::copyValue_gui(resultStore, child, parent, resultView.col("Type"));
			break;
		default:
			///@todo: throw an exception
			break;
	}
}

void Search::ungroup_gui()
{
	GtkTreeIter iter;
	gint position = 0;
	GtkTreeModel *m = GTK_TREE_MODEL(resultStore);
	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);

	while (valid)
	{
		// Ungroup parent rows and remove them
		if (gtk_tree_model_iter_has_child(m, &iter))
		{
			GtkTreeIter child = iter;
			valid = WulforUtil::getNextIter_gui(m, &child, TRUE, FALSE);

			// Move all children out from under the old grouping parent
			while (valid)
			{
				WulforUtil::copyRow_gui(resultStore, &child, NULL, position++);
				valid = gtk_tree_store_remove(resultStore, &child);
			}

			// Delete the parent row
			valid = gtk_tree_store_remove(resultStore, &iter);
		}
		else // Non-parent row
		{
			++position;
			valid = WulforUtil::getNextIter_gui(m, &iter);
		}
	}
}

void Search::regroup_gui() 
{
	unordered_map<string, GtkTreeIter> iterMap; // Maps group string -> parent tree iter
	GtkTreeIter iter;
	gint position = 0;
	GtkTreeModel *m = GTK_TREE_MODEL(resultStore);
	GroupType groupBy = (GroupType)gtk_combo_box_get_active(GTK_COMBO_BOX(getWidget("comboboxGroupBy")));
	string groupColumn = getGroupingColumn(groupBy);
	gboolean valid = gtk_tree_model_get_iter_first(m, &iter);

	while (valid)
	{
		string groupStr;
		if (!groupColumn.empty())
			groupStr = resultView.getString(&iter, groupColumn, m);

		// Don't add parent rows
		if (gtk_tree_model_iter_has_child(m, &iter) || groupStr.empty())
		{
			++position;
			valid = WulforUtil::getNextIter_gui(m, &iter);
			continue;
		}

		unordered_map<std::string, GtkTreeIter>::iterator mapIter = iterMap.find(groupStr);

		// New non-parent, top-level item
		if (mapIter == iterMap.end())
		{
			++position;
			iterMap[groupStr] = iter;
			valid = WulforUtil::getNextIter_gui(m, &iter);
		}
		else // Insert as a child under the grouping parent
		{
			GtkTreeIter parent = mapIter->second;
			GtkTreeIter groupParent = mapIter->second;

			// If this is the first child to be appended, create a new parent row.
			if (!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(resultStore), &groupParent))
			{
				gtk_tree_store_insert_with_values(resultStore, &parent, NULL, position,
					resultView.col("SearchResult"), NULL,
					resultView.col("Icon"), iconGroup,
					resultView.col("Grouping String"), groupStr.c_str(),
					-1);

				// Move the previously top-level row to be under the new parent
				GtkTreeIter child = WulforUtil::copyRow_gui(resultStore, &groupParent, &parent);
				gtk_tree_store_set(resultStore, &child, resultView.col("Grouping String"), groupStr.c_str(), -1);
				gtk_tree_store_remove(resultStore, &groupParent);
				updateParentRow_gui(&parent, &child);

				mapIter->second = parent;
			}

			// Insert the row as a child
			GtkTreeIter child = WulforUtil::copyRow_gui(resultStore, &iter, &parent);
			gtk_tree_store_set(resultStore, &child, resultView.col("Grouping String"), groupStr.c_str(), -1);
			valid = gtk_tree_store_remove(resultStore, &iter);
			updateParentRow_gui(&parent);
		}
	}
}

/*
 * We can't rely on the string from the text box since it will be internationalized.
 */
std::string Search::getGroupingColumn(GroupType groupBy)
{
	string column;

	switch (groupBy)
	{
		case Search::NOGROUPING:
			break;
		case Search::FILENAME:
			column = "Filename";
			break;
		case Search::FILEPATH:
			column = "Path";
			break;
		case Search::SIZE:
			column = "Size";
			break;
		case Search::CONNECTION:
			column = "Connection";
			break;
		case Search::TTH:
			column = "TTH";
			break;
		case Search::NICK:
			column = "Nick";
			break;
		case Search::HUB: 
			column = "Hub";
			break;
		case Search::TYPE:
			column = "Type";
			break;
		default:
			///@todo: throw an exception
			break;
	}

	return column;
}

gboolean Search::onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	Search *s = (Search *)data;

	gtk_widget_grab_focus(s->getWidget("comboboxentrySearch"));

	return TRUE;
}

gboolean Search::onButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	Search *s = (Search *)data;
	s->oldEventType = event->type;

	if (event->button == 3)
	{
		GtkTreePath *path;
		if (gtk_tree_view_get_path_at_pos(s->resultView.get(), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL))
		{
			bool selected = gtk_tree_selection_path_is_selected(s->selection, path);
			gtk_tree_path_free(path);

			if (selected)
				return TRUE;
		}
	}
	return FALSE;
}

gboolean Search::onButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	Search *s = (Search *)data;
	gint count = gtk_tree_selection_count_selected_rows(s->selection);

	if (count > 0 && event->type == GDK_BUTTON_RELEASE && event->button == 3)
		s->popupMenu_gui();
	else if (count == 1 && s->oldEventType == GDK_2BUTTON_PRESS && event->button == 1)
		s->onDownloadClicked_gui(NULL, data);

	return FALSE;
}

gboolean Search::onKeyReleased_gui(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	Search *s = (Search *)data;
	if (widget == GTK_WIDGET(s->resultView.get()))
	{
		gint count = gtk_tree_selection_count_selected_rows(s->selection);

		if (count > 0)
		{
			if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
				s->onDownloadClicked_gui(NULL, data);
			else if (event->keyval == GDK_Delete || event->keyval == GDK_BackSpace)
				s->onRemoveClicked_gui(NULL, data);
			else if (event->keyval == GDK_Menu || (event->keyval == GDK_F10 && event->state & GDK_SHIFT_MASK))
				s->popupMenu_gui();
		}
	}
	else
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
	}

	return FALSE;
}

gboolean Search::onSearchEntryKeyPressed_gui(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	Search *s = (Search *)data;

	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		s->search_gui();
	}
	else if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
	{
		gtk_combo_box_popup(GTK_COMBO_BOX(s->getWidget("comboboxentrySearch")));
		return TRUE;
	}

	return FALSE;
}

void Search::onComboBoxChanged_gui(GtkWidget* widget, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
}

void Search::onGroupByComboBoxChanged_gui(GtkWidget *comboBox, gpointer data)
{
	Search *s = (Search*)data;
	GroupType groupBy = (GroupType)gtk_combo_box_get_active(GTK_COMBO_BOX(comboBox));

	s->ungroup_gui();

	if (groupBy != NOGROUPING)
	{
		gtk_widget_set_sensitive(s->getWidget("checkbuttonFilter"), FALSE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonSlots"), FALSE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonShared"), FALSE);
		s->regroup_gui();
	}
	else
	{
		gtk_widget_set_sensitive(s->getWidget("checkbuttonFilter"), TRUE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonSlots"), FALSE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonShared"), FALSE);
	}
}

void Search::onSearchButtonClicked_gui(GtkWidget *widget, gpointer data)
{
	Search *s = (Search *)data;
	s->search_gui();
}

void Search::onFilterButtonToggled_gui(GtkToggleButton *button, gpointer data)
{
	Search *s = (Search *)data;
	GtkComboBox *comboBox = GTK_COMBO_BOX(s->getWidget("comboboxGroupBy"));

	// Disable grouping when filtering within local results
	if (gtk_toggle_button_get_active(button))
	{
		s->previousGrouping = (GroupType)gtk_combo_box_get_active(comboBox);
		gtk_combo_box_set_active(comboBox, (int)NOGROUPING);
		gtk_widget_set_sensitive(GTK_WIDGET(comboBox), FALSE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonSlots"), TRUE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonShared"), TRUE);
	}
	else
	{
		gtk_combo_box_set_active(comboBox, (int)s->previousGrouping);
		gtk_widget_set_sensitive(GTK_WIDGET(comboBox), TRUE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonSlots"), FALSE);
		gtk_widget_set_sensitive(s->getWidget("checkbuttonShared"), FALSE);

	}

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
}

void Search::onSlotsButtonToggled_gui(GtkToggleButton *button, gpointer data)
{
	Search *s = (Search *)data;

	s->onlyFree = gtk_toggle_button_get_active(button);
	if (s->onlyFree != BOOLSETTING(SEARCH_ONLY_FREE_SLOTS))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_ONLY_FREE_SLOTS, s->onlyFree);

	// Refilter current view only if "Search within local results" is enabled
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
}

void Search::onSharedButtonToggled_gui(GtkToggleButton *button, gpointer data)
{
	Search *s = (Search *)data;

	// Refilter current view only if "Search within local results" is enabled
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
}

void Search::onToggledClicked_gui(GtkCellRendererToggle *cell, gchar *path, gpointer data)
{
	Search *s = (Search *)data;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(s->hubStore), &iter, path))
	{
		gboolean toggled = s->hubView.getValue<gboolean>(&iter, "Search");
		gtk_list_store_set(s->hubStore, &iter, s->hubView.col("Search"), !toggled, -1);
	}

	// Refilter current view only if "Search within local results" is enabled
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(s->searchFilterModel));
}

void Search::onDownloadClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		SearchResult *result;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func2<Search, string, SearchResult *> F2;
		F2 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
					{
						func = new F2(s, &Search::download_client, SETTING(DOWNLOAD_DIRECTORY), result);
						WulforManager::get()->dispatchClientFunc(func);
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onDownloadFavoriteClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;
	string fav = string((gchar *)g_object_get_data(G_OBJECT(item), "fav"));

	if (!fav.empty() && gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		SearchResult *result;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func2<Search, string, SearchResult *> F2;
		F2 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
					{
						func = new F2(s, &Search::download_client, fav, result);
						WulforManager::get()->dispatchClientFunc(func);
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onDownloadToClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	int response = gtk_dialog_run(GTK_DIALOG(s->getWidget("dirChooserDialog")));
	gtk_widget_hide(s->getWidget("dirChooserDialog"));

	if (response == GTK_RESPONSE_OK)
	{
		int count = gtk_tree_selection_count_selected_rows(s->selection);
		gchar *temp = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(s->getWidget("dirChooserDialog")));

		if (temp && count > 0)
		{
			string target = Text::toUtf8(temp);
			g_free(temp);

			if (target[target.length() - 1] != PATH_SEPARATOR)
				target += PATH_SEPARATOR;

			SearchResult *result;
			GtkTreeIter iter;
			GtkTreePath *path;
			GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
			typedef Func2<Search, string, SearchResult *> F2;
			F2 *func;

			for (GList *i = list; i; i = i->next)
			{
				path = (GtkTreePath *)i->data;
				if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
				{
					bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

					do
					{
						result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
						if (result)
						{
							func = new F2(s, &Search::download_client, target, result);
							WulforManager::get()->dispatchClientFunc(func);
						}
					}
					while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
				}
				gtk_tree_path_free(path);
			}
			g_list_free(list);
		}
	}
}

void Search::onDownloadToMatchClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string fileName = WulforUtil::getTextFromMenu(item);
		SearchResult *result;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func2<Search, string, SearchResult *> F2;
		F2 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
					{
						func = new F2(s, &Search::addSource_client, fileName, result);
						WulforManager::get()->dispatchClientFunc(func);
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onDownloadDirClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		SearchResult *result;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func2<Search, string, SearchResult *> F2;
		F2 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
					{
						func = new F2(s, &Search::downloadDir_client, SETTING(DOWNLOAD_DIRECTORY), result);
						WulforManager::get()->dispatchClientFunc(func);
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onDownloadFavoriteDirClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;
	string fav = (gchar *)g_object_get_data(G_OBJECT(item), "fav");

	if (!fav.empty() && gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		SearchResult *result;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func2<Search, string, SearchResult *> F2;
		F2 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
					{
						func = new F2(s, &Search::downloadDir_client, fav, result);
						WulforManager::get()->dispatchClientFunc(func);
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onDownloadDirToClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	gint response = gtk_dialog_run(GTK_DIALOG(s->getWidget("dirChooserDialog")));
	gtk_widget_hide(s->getWidget("dirChooserDialog"));

	if (response == GTK_RESPONSE_OK)
	{
		int count = gtk_tree_selection_count_selected_rows(s->selection);
		gchar *temp = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(s->getWidget("dirChooserDialog")));

		if (temp && count > 0)
		{
			string target = Text::toUtf8(temp);
			g_free(temp);

			if (target[target.length() - 1] != PATH_SEPARATOR)
				target += PATH_SEPARATOR;

			SearchResult *result;
			GtkTreeIter iter;
			GtkTreePath *path;
			GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
			typedef Func2<Search, string, SearchResult *> F2;
			F2 *func;

			for (GList *i = list; i; i = i->next)
			{
				path = (GtkTreePath *)i->data;
				if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
				{
					bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

					do
					{
						result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
						if (result)
						{
							func = new F2(s, &Search::downloadDir_client, target, result);
							WulforManager::get()->dispatchClientFunc(func);
						}
					}
					while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
				}
				gtk_tree_path_free(path);
			}
			g_list_free(list);
		}
	}
}

void Search::onSearchByTTHClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				string tth = s->resultView.getString(&iter, "TTH");
				if (!tth.empty())
					s->putValue_gui(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onGetFileListClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid, dir;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func3<Search, string, string, bool> F3;
		F3 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					dir = s->resultView.getString(&iter, "Path");
					func = new F3(s, &Search::getFileList_client, cid, dir, FALSE);
					WulforManager::get()->dispatchClientFunc(func);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onMatchQueueClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func3<Search, string, string, bool> F3;
		F3 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					func = new F3(s, &Search::getFileList_client, cid, "", TRUE);
					WulforManager::get()->dispatchClientFunc(func);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onPrivateMessageClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					if (!cid.empty())
						WulforManager::get()->getMainWindow()->addPrivateMessage_gui(cid);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onAddFavoriteUserClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func1<Search, string> F1;
		F1 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					func = new F1(s, &Search::addFavUser_client, cid);
					WulforManager::get()->dispatchClientFunc(func);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onGrantExtraSlotClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func1<Search, string> F1;
		F1 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					func = new F1(s, &Search::grantSlot_client, cid);
					WulforManager::get()->dispatchClientFunc(func);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onRemoveUserFromQueueClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);
		typedef Func1<Search, string> F1;
		F1 *func;

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					cid = s->resultView.getString(&iter, "CID");
					func = new F1(s, &Search::removeSource_client, cid);
					WulforManager::get()->dispatchClientFunc(func);
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onRemoveClicked_gui(GtkMenuItem *item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		SearchResult *result;
		GtkTreeIter iter, filterIter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);
				GtkTreeIter parentIter = iter;

				// Decrease the SearchResult reference for every non-parent row
				do
				{
					result = s->resultView.getValue<gpointer, SearchResult *>(&iter, "SearchResult");
					if (result)
						result->dec();
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));

				// Remove the top-level node and it will remove any children nodes (if applicable)
				gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(s->sortedFilterModel), &filterIter, &parentIter);
				gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(s->searchFilterModel), &iter, &filterIter);
				gtk_tree_store_remove(s->resultStore, &iter);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Search::onCopyMagnetClicked_gui(GtkMenuItem* item, gpointer data)
{
	Search *s = (Search *)data;

	if (gtk_tree_selection_count_selected_rows(s->selection) > 0)
	{
		int64_t size;
		string magnets, magnet, filename, tth;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(s->selection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(s->sortedFilterModel, &iter, path))
			{
				bool parent = gtk_tree_model_iter_has_child(s->sortedFilterModel, &iter);

				do
				{
					filename = s->resultView.getString(&iter, "Filename");
					size = s->resultView.getValue<int64_t>(&iter, "Real Size");
					tth = s->resultView.getString(&iter, "TTH");
					magnet = WulforUtil::makeMagnet(filename, size, tth);

					if (!magnet.empty())
					{
						if (!magnets.empty())
							magnets += '\n';
						magnets += magnet;
					}
				}
				while (parent && WulforUtil::getNextIter_gui(s->sortedFilterModel, &iter, TRUE, FALSE));
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);

		if (!magnets.empty())
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), magnets.c_str(), magnets.length());
	}
}

void Search::download_client(string target, SearchResult *result)
{
	try
	{
		if (result->getType() == SearchResult::TYPE_FILE)
		{
			string subdir = Util::getFileName(WulforUtil::linuxSeparator(result->getFile()));
			QueueManager::getInstance()->add(target + subdir, result->getSize(), result->getTTH(), result->getUser());
		}
		else
		{
			QueueManager::getInstance()->addDirectory(result->getFile(), result->getUser(), target);
		}
	}
	catch (const Exception&)
	{
	}
}

void Search::downloadDir_client(string target, SearchResult *result)
{
	try
	{
		if (result->getType() == SearchResult::TYPE_FILE)
		{
			string path = WulforUtil::windowsSeparator(Util::getFilePath(WulforUtil::linuxSeparator(result->getFile())));
			QueueManager::getInstance()->addDirectory(path, result->getUser(), target);
		}
		else
		{
			QueueManager::getInstance()->addDirectory(result->getFile(), result->getUser(), target);
		}
	}
	catch (const Exception&)
	{
	}
}

void Search::addSource_client(string source, SearchResult *result)
{
	try
	{
		if (result->getType() == SearchResult::TYPE_FILE)
		{
			QueueManager::getInstance()->add(source, result->getSize(), result->getTTH(), result->getUser());
		}
	}
	catch (const Exception&)
	{
	}
}

void Search::getFileList_client(string cid, string dir, bool match)
{
	if (!cid.empty())
	{
		try
		{
			UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
			if (user)
			{
				QueueItem::FileFlags flags;
				if (match)
					flags = QueueItem::FLAG_MATCH_QUEUE;
				else
					flags = QueueItem::FLAG_CLIENT_VIEW;

				QueueManager::getInstance()->addList(user, flags, dir);
			}
		}
		catch (const Exception&)
		{
		}
	}
}

void Search::grantSlot_client(string cid)
{
	if (!cid.empty())
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
			UploadManager::getInstance()->reserveSlot(user);
	}
}

void Search::addFavUser_client(string cid)
{
	if (!cid.empty())
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
			FavoriteManager::getInstance()->addFavoriteUser(user);
	}
}

void Search::removeSource_client(string cid)
{
	if (!cid.empty())
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
			QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	}
}

void Search::on(ClientManagerListener::ClientConnected, Client *client) throw()
{
	if (client)
	{
		typedef Func2<Search, string, string> F2;
		F2 *func = new F2(this, &Search::addHub_gui, client->getHubName(), client->getHubUrl());
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Search::on(ClientManagerListener::ClientUpdated, Client *client) throw()
{
	if (client)
	{
		typedef Func2<Search, string, string> F2;
		F2 *func = new F2(this, &Search::modifyHub_gui, client->getHubName(), client->getHubUrl());
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Search::on(ClientManagerListener::ClientDisconnected, Client *client) throw()
{
	if (client)
	{
		typedef Func1<Search, string> F1;
		F1 *func = new F1(this, &Search::removeHub_gui, client->getHubUrl());
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Search::on(SearchManagerListener::SR, const SearchResultPtr& result) throw()
{
	bool isShared = FALSE;
	if (searchlist.empty() || result == NULL)
		return;

	typedef Func2<Search, string, string> F2;
	F2 *func;

	if (isHash)
	{
		if (result->getType() != SearchResult::TYPE_FILE || TTHValue(searchlist[0]) != result->getTTH())
		{
			++droppedResult;
			func = new F2(this, &Search::setStatus_gui, "statusbar3", Util::toString(droppedResult) + _(" filtered"));
			WulforManager::get()->dispatchGuiFunc(func);
			return;
		}
	}
	else
	{
		for (TStringIter i = searchlist.begin(); i != searchlist.end(); ++i)
		{
			if ((*i->begin() != '-' && Util::findSubString(result->getFile(), *i) == (string::size_type)-1) ||
			    (*i->begin() == '-' && i->size() != 1 && Util::findSubString(result->getFile(), i->substr(1)) != (string::size_type)-1))
			{
				++droppedResult;
				func = new F2(this, &Search::setStatus_gui, "statusbar3", Util::toString(droppedResult) + _(" filtered"));
				WulforManager::get()->dispatchGuiFunc(func);
				return;
			}
		}
	}

	if (result->getType() != SearchResult::TYPE_DIRECTORY)
		isShared = ShareManager::getInstance()->isTTHShared(result->getTTH());

	result->inc(); // I'm going to hell for this, yes?

	typedef Func2<Search, SearchResult *, bool> F2_2;
	F2_2 *func2 = new F2_2(this, &Search::addResult_gui, result.get(), isShared);
	WulforManager::get()->dispatchGuiFunc(func2);
}

// Filtering causes Gtk-CRITICAL assertion failure, when last item is removed
// see. http://bugzilla.gnome.org/show_bug.cgi?id=464173
gboolean Search::searchFilterFunc_gui(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	Search *s = (Search *)data;
	dcassert(model == GTK_TREE_MODEL(s->resultStore));

	// Enabler filtering only if search within local results is checked
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonFilter"))))
		return TRUE;

	// Filter based on selected hubs.
	SearchResult *result = s->resultView.getValue<gpointer, SearchResult *>(iter, "SearchResult", model);

	dcassert(result);	// Filtering isn't enabled with grouping, so we shouldn't have rows without resultptr
	if (!result)		// in case this isn't a debug build, show the row instead of crashing
		return TRUE;	// other actions on the row might still crash.

	string hub = result->getHubURL();
	GtkTreeIter hubIter;
	bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(s->hubStore), &hubIter);
	while (valid)
	{
		if (hub == s->hubView.getString(&hubIter, "Url"))
		{
			if (!s->hubView.getValue<gboolean>(&hubIter, "Search"))
				return FALSE;
			else
				break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(s->hubStore), &hubIter);
	}

	// Filter based on free slots.
	if (s->onlyFree && result->getFreeSlots() < 1)
		return FALSE;

	// Hide results already in share
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(s->getWidget("checkbuttonShared"))) &&
		s->resultView.getValue<gboolean>(iter, "Shared", model) == TRUE)
		return FALSE;

	// Filter based on search terms.
	string filter = Text::toLower(gtk_entry_get_text(GTK_ENTRY(s->searchEntry)));
	TStringList filterList = StringTokenizer<tstring>(filter, ' ').getTokens();
	string filename = Text::toLower(s->resultView.getString(iter, "Filename", model));
	string path = Text::toLower(s->resultView.getString(iter, "Path", model));
	for (TStringList::const_iterator term = filterList.begin(); term != filterList.end(); ++term)
	{
		if ((*term)[0] == '-')
		{
			if (filename.find((*term).substr(1)) != string::npos)
				return FALSE;
			else if (path.find((*term).substr(1)) != string::npos)
				return FALSE;
		}
		else if (filename.find(*term) == string::npos && path.find(*term) == string::npos)
			return FALSE;
	}

	// Filter based on file size.
	double filterSize = Util::toDouble(gtk_entry_get_text(GTK_ENTRY(s->getWidget("entrySize"))));
	if (filterSize > 0)
	{
		switch (gtk_combo_box_get_active(GTK_COMBO_BOX(s->getWidget("comboboxUnit"))))
		{
			case 1:
				filterSize *= 1024.0;
				break;
			case 2:
				filterSize *= 1024.0 * 1024.0;
				break;
			case 3:
				filterSize *= 1024.0 * 1024.0 * 1024.0;
				break;
		}

		switch (gtk_combo_box_get_active(GTK_COMBO_BOX(s->getWidget("comboboxSize"))))
		{
			case 0:
				if (result->getSize() != filterSize)
					return FALSE;
				break;
			case 1:
				if (result->getSize() < filterSize)
					return FALSE;
				break;
			case 2:
				if (result->getSize() > filterSize)
					return FALSE;
		}
	}

	int type = gtk_combo_box_get_active(GTK_COMBO_BOX(s->getWidget("comboboxFile")));
	if (type != SearchManager::TYPE_ANY && type != ShareManager::getInstance()->getType(filename))
		return FALSE;

	return TRUE;
}

