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

#include "mainwindow.hh"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <dcpp/FavoriteManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/Text.h>
#include <dcpp/Upload.h>
#include <dcpp/Download.h>
#include <dcpp/ClientManager.h>
#include "downloadqueue.hh"
#include "favoritehubs.hh"
#include "finishedtransfers.hh"
#include "func.hh"
#include "hub.hh"
#include "privatemessage.hh"
#include "publichubs.hh"
#include "search.hh"
#include "settingsmanager.hh"
#include "sharebrowser.hh"
#include "emoticons.hh"
#include "UserCommandMenu.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

MainWindow::MainWindow():
	Entry(Entry::MAIN_WINDOW, "mainwindow.glade"),
	transfers(NULL),
	lastUpdate(0),
	lastUp(0),
	lastDown(0),
	minimized(FALSE),
	timer(0),
	statusFrame(1)
{
	window = GTK_WINDOW(getWidget("mainWindow"));
	gtk_window_set_role(window, getID().c_str());

	// Configure the dialogs
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("exitDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("connectDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("flistDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("ucLineDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("exitDialog")), window);
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("connectDialog")), window);
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("flistDialog")), window);
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("ucLineDialog")), window);
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("magnetDialog")), window);

	// set logo 96x96 about dialog
	GtkIconTheme *iconTheme = gtk_icon_theme_get_default();
	GdkPixbuf *logo = gtk_icon_theme_load_icon(iconTheme, g_get_prgname(), 96, GTK_ICON_LOOKUP_FORCE_SVG, NULL);

	if (logo != NULL)
	{
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(getWidget("aboutDialog")), logo);
		g_object_unref(logo);
	}

	gtk_about_dialog_set_email_hook((GtkAboutDialogActivateLinkFunc)onAboutDialogActivateLink_gui, (gpointer)this, NULL);
	gtk_about_dialog_set_url_hook((GtkAboutDialogActivateLinkFunc)onAboutDialogActivateLink_gui, (gpointer)this, NULL);
	// This has to be set in code in order to activate the link
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(getWidget("aboutDialog")), "http://freedcpp.narod.ru");
	gtk_window_set_transient_for(GTK_WINDOW(getWidget("aboutDialog")), window);

	// Set all windows to the default icon
	gtk_window_set_default_icon_name(g_get_prgname());

	// Disable un-implemented menu items.
	gtk_widget_set_sensitive(getWidget("favoriteUsersMenuItem"), FALSE);

	// All notebooks created in glade need one page.
	// In our case, this is just a placeholder, so we remove it.
	gtk_notebook_remove_page(GTK_NOTEBOOK(getWidget("book")), -1);
	g_object_set_data(G_OBJECT(getWidget("book")), "page-rotation-list", NULL);
	gtk_widget_set_sensitive(getWidget("closeMenuItem"), FALSE);

	// Connect the signals to their callback functions.
	g_signal_connect(window, "delete-event", G_CALLBACK(onCloseWindow_gui), (gpointer)this);
	g_signal_connect(window, "window-state-event", G_CALLBACK(onWindowState_gui), (gpointer)this);
	g_signal_connect(window, "focus-in-event", G_CALLBACK(onFocusIn_gui), (gpointer)this);
	g_signal_connect(window, "key-press-event", G_CALLBACK(onKeyPressed_gui), (gpointer)this);
	g_signal_connect(getWidget("book"), "switch-page", G_CALLBACK(onPageSwitched_gui), (gpointer)this);
	g_signal_connect_after(getWidget("pane"), "realize", G_CALLBACK(onPaneRealized_gui), (gpointer)this);
	g_signal_connect(getWidget("connect"), "clicked", G_CALLBACK(onConnectClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("favHubs"), "clicked", G_CALLBACK(onFavoriteHubsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("publicHubs"), "clicked", G_CALLBACK(onPublicHubsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("settings"), "clicked", G_CALLBACK(onPreferencesClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("hash"), "clicked", G_CALLBACK(onHashClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("search"), "clicked", G_CALLBACK(onSearchClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("queue"), "clicked", G_CALLBACK(onDownloadQueueClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("quit"), "clicked", G_CALLBACK(onQuitClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("finishedDownloads"), "clicked", G_CALLBACK(onFinishedDownloadsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("finishedUploads"), "clicked", G_CALLBACK(onFinishedUploadsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openFileListMenuItem"), "activate", G_CALLBACK(onOpenFileListClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openOwnListMenuItem"), "activate", G_CALLBACK(onOpenOwnListClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("refreshFileListMenuItem"), "activate", G_CALLBACK(onRefreshFileListClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("quickConnectMenuItem"), "activate", G_CALLBACK(onConnectClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("reconnectMenuItem"), "activate", G_CALLBACK(onReconnectClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("settingsMenuItem"), "activate", G_CALLBACK(onPreferencesClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("closeMenuItem"), "activate", G_CALLBACK(onCloseClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("exitMenuItem"), "activate", G_CALLBACK(onQuitClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("favoriteHubsMenuItem"), "activate", G_CALLBACK(onFavoriteHubsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("publicHubsMenuItem"), "activate", G_CALLBACK(onPublicHubsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("indexingProgressMenuItem"), "activate", G_CALLBACK(onHashClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("searchMenuItem"), "activate", G_CALLBACK(onSearchClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("downloadQueueMenuItem"), "activate", G_CALLBACK(onDownloadQueueClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("finishedDownloadsMenuItem"), "activate", G_CALLBACK(onFinishedDownloadsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("finishedUploadsMenuItem"), "activate", G_CALLBACK(onFinishedUploadsClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("previousTabMenuItem"), "activate", G_CALLBACK(onPreviousTabClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("nextTabMenuItem"), "activate", G_CALLBACK(onNextTabClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("aboutMenuItem"), "activate", G_CALLBACK(onAboutClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("homeMenuItem"), "activate", G_CALLBACK(onLinkClicked_gui), (gchar*)"http://freedcpp.narod.ru");
	g_signal_connect(getWidget("sourceMenuItem"), "activate", G_CALLBACK(onLinkClicked_gui), (gchar*)"http://code.google.com/p/freedcpp/source/checkout");

	// Load window state and position from settings manager
	gint posX = WGETI("main-window-pos-x");
	gint posY = WGETI("main-window-pos-y");
	gint sizeX = WGETI("main-window-size-x");
	gint sizeY = WGETI("main-window-size-y");

	gtk_window_move(window, posX, posY);
	gtk_window_resize(window, sizeX, sizeY);
	if (WGETI("main-window-maximized"))
		gtk_window_maximize(window);

	setMainStatus_gui(_("Welcome to ") + string(g_get_application_name()));

	loadIcons_gui();
	showTransfersPane_gui();

	// Putting this after all the resizing and moving makes the window appear
	// in the correct position instantly, looking slightly more cool 
	// (seems we have rather poor standards for cool?)
	gtk_widget_show_all(GTK_WIDGET(window));

	setTabPosition_gui(WGETI("tab-position"));
	setToolbarStyle_gui(WGETI("toolbar-style"));

	createStatusIcon_gui();

	Sound::start();
	Emoticons::start();
	Notify::start();
}

MainWindow::~MainWindow()
{
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	LogManager::getInstance()->removeListener(this);

	GList *list = (GList *)g_object_get_data(G_OBJECT(getWidget("book")), "page-rotation-list");
	g_list_free(list);

	// Save window state and position
	gint posX, posY, sizeX, sizeY, transferPanePosition;
	bool maximized = TRUE;
	GdkWindowState gdkState;

	gtk_window_get_position(window, &posX, &posY);
	gtk_window_get_size(window, &sizeX, &sizeY);
	gdkState = gdk_window_get_state(GTK_WIDGET(window)->window);
	transferPanePosition = sizeY - gtk_paned_get_position(GTK_PANED(getWidget("pane")));

	if (!(gdkState & GDK_WINDOW_STATE_MAXIMIZED))
	{
		maximized = FALSE;
		// The get pos/size functions return junk when window is maximized
		WSET("main-window-pos-x", posX);
		WSET("main-window-pos-y", posY);
		WSET("main-window-size-x", sizeX);
		WSET("main-window-size-y", sizeY);
	}

	WSET("main-window-maximized", maximized);
	if (transferPanePosition > 10)
		WSET("transfer-pane-position", transferPanePosition);

	if (timer > 0)
		g_source_remove(timer);

	gtk_widget_destroy(GTK_WIDGET(window));
	g_object_unref(statusIcon);

	Sound::stop();
	Emoticons::stop();
	Notify::stop();
}

GtkWidget *MainWindow::getContainer()
{
	return getWidget("mainWindow");
}

void MainWindow::show()
{
	QueueManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);

	typedef Func0<MainWindow> F0;
	F0 *f0 = new F0(this, &MainWindow::startSocket_client);
	WulforManager::get()->dispatchClientFunc(f0);

	f0 = new F0(this, &MainWindow::autoConnect_client);
	WulforManager::get()->dispatchClientFunc(f0);

	autoOpen_gui();
}

void MainWindow::setTitle(const string& text)
{
	string title;

	if (!text.empty())
		title = text + " - " + g_get_application_name();
	else
		title = g_get_application_name();

	gtk_window_set_title(window, title.c_str());
}

bool MainWindow::isActive_gui()
{
	return gtk_window_is_active(window);
}

void MainWindow::setUrgent_gui()
{
	gtk_window_set_urgency_hint(window, true);
}

/*
 * Create and show Transfers pane
 */
void MainWindow::showTransfersPane_gui()
{
	dcassert(transfers == NULL);

	transfers = new Transfers();
	gtk_paned_pack2(GTK_PANED(getWidget("pane")), transfers->getContainer(), TRUE, TRUE);
	addChild(transfers);
	transfers->show();
}

/*
 * Load the custom icons or the stock icons as per the setting
 */
void MainWindow::loadIcons_gui()
{
	WulforUtil::registerIcons();

	// Reset the stock IDs manually to force the icon to refresh
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("favHubs")), "freedcpp-favorite-hubs");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("publicHubs")), "freedcpp-public-hubs");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("settings")), "freedcpp-preferences");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("hash")), "freedcpp-hash");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("search")), "freedcpp-search");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("queue")), "freedcpp-queue");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("finishedDownloads")), "freedcpp-finished-downloads");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("finishedUploads")), "freedcpp-finished-uploads");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("quit")), "freedcpp-quit");
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(getWidget("connect")), "freedcpp-connect");
	gtk_image_set_from_stock(GTK_IMAGE(getWidget("imageHubs")), "freedcpp-public-hubs", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_image_set_from_stock(GTK_IMAGE(getWidget("imageDownloadSpeed")), "freedcpp-download", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_image_set_from_stock(GTK_IMAGE(getWidget("imageUploadSpeed")), "freedcpp-upload", GTK_ICON_SIZE_SMALL_TOOLBAR);
}

void MainWindow::autoOpen_gui()
{
	if (BOOLSETTING(OPEN_PUBLIC))
		showPublicHubs_gui();
	if (BOOLSETTING(OPEN_QUEUE))
		showDownloadQueue_gui();
	if (BOOLSETTING(OPEN_FAVORITE_HUBS))
		showFavoriteHubs_gui();
	if (BOOLSETTING(OPEN_FINISHED_DOWNLOADS))
		showFinishedDownloads_gui();
	if (BOOLSETTING(OPEN_FINISHED_UPLOADS))
		showFinishedUploads_gui();
}

void MainWindow::addBookEntry_gui(BookEntry *entry)
{
	addChild(entry);

	GtkWidget *page = entry->getContainer();
	GtkWidget *label = entry->getLabelBox();
	GtkWidget *closeButton = entry->getCloseButton();
	GtkWidget *tabMenuItem = entry->getTabMenuItem();

	addTabMenuItem_gui(tabMenuItem, page);

	gtk_notebook_append_page(GTK_NOTEBOOK(getWidget("book")), page, label);

	g_signal_connect(label, "button-release-event", G_CALLBACK(onButtonReleasePage_gui), (gpointer)entry);
	g_signal_connect(closeButton, "button-release-event", G_CALLBACK(onButtonReleasePage_gui), (gpointer)entry);
	g_signal_connect(closeButton, "clicked", G_CALLBACK(onCloseBookEntry_gui), (gpointer)entry);

	gtk_widget_set_sensitive(getWidget("closeMenuItem"), TRUE);

	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(getWidget("book")), page, TRUE);

	entry->show();
}

GtkWidget *MainWindow::currentPage_gui()
{
	int pageNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(getWidget("book")));

	if (pageNum == -1)
		return NULL;
	else
		return gtk_notebook_get_nth_page(GTK_NOTEBOOK(getWidget("book")), pageNum);
}

void MainWindow::raisePage_gui(GtkWidget *page)
{
	int num = gtk_notebook_page_num(GTK_NOTEBOOK(getWidget("book")), page);
	int currentNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(getWidget("book")));

	if (num != -1 && num != currentNum)
		gtk_notebook_set_current_page(GTK_NOTEBOOK(getWidget("book")), num);
}

void MainWindow::removeBookEntry_gui(BookEntry *entry)
{
	string entryID = entry->getID();

	string::size_type pos = entryID.find(':');
	if (pos != string::npos) entryID.erase(0, pos + 1);

	StringIter it = find(EntryList.begin(), EntryList.end(), entryID);

	if (it != EntryList.end())
		EntryList.erase(it);

	GtkNotebook *book = GTK_NOTEBOOK(getWidget("book"));
	GtkWidget *page = entry->getContainer();
	GtkWidget* menuItem = entry->getTabMenuItem();
	int num = gtk_notebook_page_num(book, page);
	removeChild(entry);

	if (num != -1)
	{
		GList *list = (GList *)g_object_get_data(G_OBJECT(book), "page-rotation-list");
		list = g_list_remove(list, (gpointer)page);
		g_object_set_data(G_OBJECT(book), "page-rotation-list", (gpointer)list);

		// if removing the current page, switch to the previous page in the rotation list
		if (num == gtk_notebook_get_current_page(book))
		{
			GList *prev = g_list_first(list);
			if (prev != NULL)
			{
				gint childNum = gtk_notebook_page_num(book, GTK_WIDGET(prev->data));
				gtk_notebook_set_current_page(book, childNum);
			}
		}
		gtk_notebook_remove_page(book, num);

		removeTabMenuItem_gui(menuItem);

		if (gtk_notebook_get_n_pages(book) == 0)
		{
			gtk_widget_set_sensitive(getWidget("closeMenuItem"), FALSE);
			setTitle(""); // Reset window title to default
		}
	}
}

void MainWindow::previousTab_gui()
{
	GtkNotebook *book = GTK_NOTEBOOK(getWidget("book"));

	if (gtk_notebook_get_current_page(book) == 0)
		gtk_notebook_set_current_page(book, -1);
	else
		gtk_notebook_prev_page(book);
}

void MainWindow::nextTab_gui()
{
	GtkNotebook *book = GTK_NOTEBOOK(getWidget("book"));

	if (gtk_notebook_get_n_pages(book) - 1 == gtk_notebook_get_current_page(book))
		gtk_notebook_set_current_page(book, 0);
	else
		gtk_notebook_next_page(book);
}

void MainWindow::addTabMenuItem_gui(GtkWidget* menuItem, GtkWidget* page)
{
	g_signal_connect(menuItem, "activate", G_CALLBACK(onRaisePage_gui), (gpointer)page);
	gtk_menu_shell_append(GTK_MENU_SHELL(getWidget("tabsMenu")), menuItem);
	gtk_widget_show_all(getWidget("tabsMenu"));

	gtk_widget_set_sensitive(getWidget("previousTabMenuItem"), TRUE);
	gtk_widget_set_sensitive(getWidget("nextTabMenuItem"), TRUE);
	gtk_widget_set_sensitive(getWidget("tabMenuSeparator"), TRUE);
}

void MainWindow::removeTabMenuItem_gui(GtkWidget *menuItem)
{
	GtkNotebook *book = GTK_NOTEBOOK(getWidget("book"));

	gtk_container_remove(GTK_CONTAINER(getWidget("tabsMenu")), menuItem);

	if (gtk_notebook_get_n_pages(book) == 0)
	{
		gtk_widget_set_sensitive(getWidget("previousTabMenuItem"), FALSE);
		gtk_widget_set_sensitive(getWidget("nextTabMenuItem"), FALSE);
		gtk_widget_set_sensitive(getWidget("tabMenuSeparator"), FALSE);
	}
}

/*
 * Create status icon.
 */
void MainWindow::createStatusIcon_gui()
{
	statusIcon = gtk_status_icon_new_from_icon_name(g_get_prgname());

	g_signal_connect(getWidget("statusIconQuitItem"), "activate", G_CALLBACK(onQuitClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("statusIconShowInterfaceItem"), "toggled", G_CALLBACK(onShowInterfaceToggled_gui), (gpointer)this);
	g_signal_connect(statusIcon, "activate", G_CALLBACK(onStatusIconActivated_gui), (gpointer)this);
	g_signal_connect(statusIcon, "popup-menu", G_CALLBACK(onStatusIconPopupMenu_gui), (gpointer)this);

	if (BOOLSETTING(ALWAYS_TRAY))
		gtk_status_icon_set_visible(statusIcon, TRUE);
	else
		gtk_status_icon_set_visible(statusIcon, FALSE);
}

void MainWindow::updateStatusIconTooltip_gui(string download, string upload)
{
	ostringstream toolTip;
	toolTip << g_get_application_name() << endl << _("Download: ") << download << endl << _("Upload: ") << upload;
	gtk_status_icon_set_tooltip(statusIcon, toolTip.str().c_str());
}

void MainWindow::setMainStatus_gui(string text, time_t t)
{
	if (!text.empty())
	{
		text = "[" + Util::getShortTimeString(t) + "] " + text;
		gtk_label_set_text(GTK_LABEL(getWidget("labelStatus")), text.c_str());
	}
}

void MainWindow::showNotification_gui(string head, string body, Notify::TypeNotify notify)
{
	Notify::get()->showNotify(head, body, notify);
}

void MainWindow::setStats_gui(string hubs, string downloadSpeed,
	string downloaded, string uploadSpeed, string uploaded)
{
	gtk_label_set_text(GTK_LABEL(getWidget("labelHubs")), hubs.c_str());
	gtk_label_set_text(GTK_LABEL(getWidget("labelDownloadSpeed")), downloadSpeed.c_str());
	gtk_label_set_text(GTK_LABEL(getWidget("labelDownloaded")), downloaded.c_str());
	gtk_label_set_text(GTK_LABEL(getWidget("labelUploadSpeed")), uploadSpeed.c_str());
	gtk_label_set_text(GTK_LABEL(getWidget("labelUploaded")), uploaded.c_str());
}

BookEntry* MainWindow::findBookEntry(const EntryType type, const string &id)
{
	Entry *entry = getChild(type, id);
	return dynamic_cast<BookEntry*>(entry);
}

void MainWindow::showDownloadQueue_gui()
{
	BookEntry *entry = findBookEntry(Entry::DOWNLOAD_QUEUE);

	if (entry == NULL)
	{
		entry = new DownloadQueue();
		addBookEntry_gui(entry);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::showFavoriteHubs_gui()
{
	BookEntry *entry = findBookEntry(Entry::FAVORITE_HUBS);

	if (entry == NULL)
	{
		entry = new FavoriteHubs();
		addBookEntry_gui(entry);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::showFinishedDownloads_gui()
{
	BookEntry *entry = findBookEntry(Entry::FINISHED_DOWNLOADS);

	if (entry == NULL)
	{
		entry = FinishedTransfers::createFinishedDownloads();
		addBookEntry_gui(entry);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::showFinishedUploads_gui()
{
	BookEntry *entry = findBookEntry(Entry::FINISHED_UPLOADS);

	if (entry == NULL)
	{
		entry = FinishedTransfers::createFinishedUploads();
		addBookEntry_gui(entry);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::showHub_gui(string address, string encoding)
{
	BookEntry *entry = findBookEntry(Entry::HUB, address);

	if (entry == NULL)
	{
		entry = new Hub(address, encoding);
		addBookEntry_gui(entry);

		EntryList.push_back(address);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::addPrivateMessage_gui(Msg::TypeMsg typemsg, string cid, string hubUrl, string message, bool useSetting)
{
	BookEntry *entry = findBookEntry(Entry::PRIVATE_MESSAGE, cid);
	bool raise = TRUE;

	// If PM is initiated by another user, use setting except if tab is already open.
	if (useSetting)
		raise = (entry == NULL) ? !BOOLSETTING(POPUNDER_PM) : FALSE;

	if (entry == NULL)
	{
		entry = new PrivateMessage(cid, hubUrl);
		addBookEntry_gui(entry);

		EntryList.push_back(cid);
	}

	if (!message.empty())
	{
		dynamic_cast<PrivateMessage*>(entry)->addMessage_gui(message, typemsg);

		bool show = FALSE;
		bool anim = FALSE;

		if (!isActive_gui())
		{
			show = TRUE;
			anim = TRUE;
		}
		else if (currentPage_gui() != entry->getContainer() && !WGETI("notify-only-not-active"))
		{
			show = TRUE;
		}

		if (anim && timer == 0)
		{
			timer = g_timeout_add(1000, animationStatusIcon_gui, (gpointer)this);
		}

		if (show)
		{
			const int length = WGETI("notify-pm-length");

			if (length > 0 && g_utf8_strlen(message.c_str(), -1) > length)
			{
				const gchar *p = message.c_str();
				int ii = 0;

				while (*p)
				{
					p = g_utf8_next_char(p);

					if (++ii >= length)
						break;
				}

				string::size_type i = string(p).size();
				string::size_type j = message.size();

				Notify::get()->showNotify("", message.substr(0, j - i) + "...", Notify::PRIVATE_MESSAGE);
			}
			else
				Notify::get()->showNotify("", message, Notify::PRIVATE_MESSAGE);
		}
	}

	if (raise)
		raisePage_gui(entry->getContainer());
}

void MainWindow::addPrivateStatusMessage_gui(Msg::TypeMsg typemsg, string cid, string message)
{
	BookEntry *entry = findBookEntry(Entry::PRIVATE_MESSAGE, cid);

	if (entry != NULL)
		dynamic_cast<PrivateMessage*>(entry)->addStatusMessage_gui(message, typemsg);
}

void MainWindow::showPublicHubs_gui()
{
	BookEntry *entry = findBookEntry(Entry::PUBLIC_HUBS);

	if (entry == NULL)
	{
		entry = new PublicHubs();
		addBookEntry_gui(entry);
	}

	raisePage_gui(entry->getContainer());
}

void MainWindow::showShareBrowser_gui(UserPtr user, string filename, string dir, bool useSetting)
{
	bool raise = useSetting ? !BOOLSETTING(POPUNDER_FILELIST) : TRUE;
	BookEntry *entry = findBookEntry(Entry::SHARE_BROWSER, user->getCID().toBase32());

	if (entry == NULL)
	{
		entry = new ShareBrowser(user, filename, dir);
		addBookEntry_gui(entry);
	}

	if (raise)
		raisePage_gui(entry->getContainer());
}

Search *MainWindow::addSearch_gui()
{
	Search *entry = new Search();
	addBookEntry_gui(entry);
	raisePage_gui(entry->getContainer());
	return entry;
}

void MainWindow::addSearch_gui(string magnet)
{
	string name;
	int64_t size;
	string tth;

	if (WulforUtil::splitMagnet(magnet, name, size, tth))
	{
		Search *s = addSearch_gui();
		s->putValue_gui(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
	}
}

void MainWindow::setTabPosition_gui(int position)
{
	GtkPositionType tabPosition;

	switch (position)
	{
		case 0:
			tabPosition = GTK_POS_TOP;
			break;
		case 1:
			tabPosition = GTK_POS_LEFT;
			break;
		case 2:
			tabPosition = GTK_POS_RIGHT;
			break;
		case 3:
			tabPosition = GTK_POS_BOTTOM;
			break;
		default:
			tabPosition = GTK_POS_TOP;
	}

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(getWidget("book")), tabPosition);
}

void MainWindow::setToolbarStyle_gui(int style)
{
	GtkToolbarStyle toolbarStyle;

	switch (style)
	{
		case 0:
			toolbarStyle = GTK_TOOLBAR_ICONS;
			break;
		case 1:
			toolbarStyle = GTK_TOOLBAR_TEXT;
			break;
		case 2:
			toolbarStyle = GTK_TOOLBAR_BOTH;
			break;
		case 3:
			toolbarStyle = GTK_TOOLBAR_BOTH_HORIZ;
			break;
		case 4:
			gtk_widget_hide(getWidget("toolbar1"));
			break;
		case 5:
			return;
		default:
			toolbarStyle = GTK_TOOLBAR_BOTH;
	}

	if (style != 4)
	{
		gtk_widget_show(getWidget("toolbar1"));
		gtk_toolbar_set_style(GTK_TOOLBAR(getWidget("toolbar1")), toolbarStyle);
	}
}

bool MainWindow::getUserCommandLines_gui(const string &command, StringMap &ucParams)
{
	string name;
	string label;
	string line;
	StringMap done;
	string::size_type i = 0;
	string::size_type j = 0;
	string text = string("<b>") + _("Enter value for ") + "\'";

	while ((i = command.find("%[line:", i)) != string::npos)
	{
		i += 7;
		j = command.find(']', i);
		if (j == string::npos)
			break;

		name = command.substr(i, j - i);
		if (done.find(name) == done.end())
		{
			line.clear();
			label = text + name + "\'</b>";

			gtk_label_set_label(GTK_LABEL(getWidget("ucLabel")), label.c_str());
			gtk_entry_set_text(GTK_ENTRY(getWidget("ucLineEntry")), "");
			gtk_widget_grab_focus(getWidget("ucLineEntry"));

			gint response = gtk_dialog_run(GTK_DIALOG(getWidget("ucLineDialog")));
			gtk_widget_hide(getWidget("ucLineDialog"));

			if (response == GTK_RESPONSE_OK)
				line = gtk_entry_get_text(GTK_ENTRY(getWidget("ucLineEntry")));

			if (!line.empty())
			{
				ucParams["line:" + name] = line;
				done[name] = line;
			}
			else
				return false;
		}
		i = j + 1;
	}

	return true;
}

void MainWindow::openMagnetDialog_gui(const string &magnet)
{
	string name;
	int64_t size;
	string tth;

	WulforUtil::splitMagnet(magnet, name, size, tth);

	gtk_entry_set_text(GTK_ENTRY(getWidget("magnetEntry")), magnet.c_str());
	gtk_entry_set_text(GTK_ENTRY(getWidget("magnetNameEntry")), name.c_str());
	gtk_entry_set_text(GTK_ENTRY(getWidget("magnetSizeEntry")), Util::formatBytes(size).c_str());
	gtk_entry_set_text(GTK_ENTRY(getWidget("exactSizeEntry")), Util::formatExactSize(size).c_str());
	gtk_entry_set_text(GTK_ENTRY(getWidget("tthEntry")), tth.c_str());

	gtk_dialog_run(GTK_DIALOG(getWidget("magnetDialog")));
	gtk_widget_hide(getWidget("magnetDialog"));
}

void MainWindow::showMessageDialog_gui(const string primaryText, const string secondaryText)
{
	if (primaryText.empty())
		return;

	GtkWidget* dialog = gtk_message_dialog_new(window, GTK_DIALOG_DESTROY_WITH_PARENT, 
		GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", primaryText.c_str());

	if (!secondaryText.empty())
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondaryText.c_str());

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gboolean MainWindow::onWindowState_gui(GtkWidget *widget, GdkEventWindowState *event, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;

	if (!mw->minimized && event->new_window_state & (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN))
	{
		mw->minimized = TRUE;
		if (BOOLSETTING(SettingsManager::AUTO_AWAY) && !Util::getAway())
			Util::setAway(TRUE);
	}
	else if (mw->minimized && (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED ||
		event->new_window_state == 0))
	{
		mw->minimized = FALSE;
		if (BOOLSETTING(SettingsManager::AUTO_AWAY) && !Util::getManualAway())
			Util::setAway(FALSE);
	}

	return TRUE;
}

gboolean MainWindow::onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkWidget *child = mw->currentPage_gui();

	if (child != NULL)
	{
		BookEntry *entry = (BookEntry *)g_object_get_data(G_OBJECT(child), "entry");
		entry->setActive_gui();
	}

	gtk_window_set_urgency_hint(mw->window, FALSE);
	return FALSE;
}

gboolean MainWindow::onCloseWindow_gui(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;

	if (!BOOLSETTING(CONFIRM_EXIT))
	{
		WulforManager::get()->deleteMainWindow();
		return FALSE;
	}

	int response = gtk_dialog_run(GTK_DIALOG(mw->getWidget("exitDialog")));
	gtk_widget_hide(mw->getWidget("exitDialog"));

	if (response == GTK_RESPONSE_OK)
	{
		WulforManager::get()->deleteMainWindow();
		return FALSE;
	}

	return TRUE;
}

gboolean MainWindow::onKeyPressed_gui(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;

	if (event->state & GDK_CONTROL_MASK)
	{
		if (event->state & GDK_SHIFT_MASK && event->keyval == GDK_ISO_Left_Tab)
		{
			mw->previousTab_gui();
			return TRUE;
		}
		else if (event->keyval == GDK_Tab)
		{
			mw->nextTab_gui();
			return TRUE;
		}
	}

	return FALSE;
}

gboolean MainWindow::onButtonReleasePage_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gint width, height;
	gdk_drawable_get_size(event->window, &width, &height);

	// If middle mouse button was released when hovering over tab label
	if (event->button == 2 && event->x >= 0 && event->y >= 0
		&& event->x < width && event->y < height)
	{
		BookEntry *entry = (BookEntry *)data;
		WulforManager::get()->getMainWindow()->removeBookEntry_gui(entry);
		return TRUE;
	}

	return FALSE;
}

gboolean MainWindow::animationStatusIcon_gui(gpointer data)
{
	MainWindow *mw = (MainWindow *) data;

	if (mw->isActive_gui())
	{
		gtk_status_icon_set_from_icon_name(mw->statusIcon, g_get_prgname());
		mw->timer = 0;

		return FALSE;
	}

	gtk_status_icon_set_from_icon_name(mw->statusIcon, (mw->statusFrame *= -1) > 0 ? "freedcpp" : "freedcpp-normal");

	return TRUE;
}

void MainWindow::onRaisePage_gui(GtkMenuItem *item, gpointer data)
{
	WulforManager::get()->getMainWindow()->raisePage_gui((GtkWidget *)data);
}

void MainWindow::onPageSwitched_gui(GtkNotebook *notebook, GtkNotebookPage *page, guint num, gpointer data)
{
	MainWindow* mw = (MainWindow*)data;
	GtkWidget *child = gtk_notebook_get_nth_page(notebook, num);
	BookEntry *entry = (BookEntry *)g_object_get_data(G_OBJECT(child), "entry");

	if (entry)
	{
		// Disable "activate" signal on the tab menu item since it can cause
		// onPageSwitched_gui to be called multiple times
		GtkWidget *item = entry->getTabMenuItem();
		g_signal_handlers_block_by_func(item, (gpointer)onRaisePage_gui, child);

		entry->setActive_gui();
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(entry->getTabMenuItem()), TRUE);
		mw->setTitle(entry->getLabelText()); // Update window title with selected tab label

		g_signal_handlers_unblock_by_func(item, (gpointer)onRaisePage_gui, (gpointer)child);
	}

	GList *list = (GList *)g_object_get_data(G_OBJECT(notebook), "page-rotation-list");
	list = g_list_remove(list, (gpointer)child);
	list = g_list_prepend(list, (gpointer)child);
	g_object_set_data(G_OBJECT(notebook), "page-rotation-list", (gpointer)list);

	// Focus the tab so it will focus its children (e.g. a text entry box)
	gtk_widget_grab_focus(child);
}

void MainWindow::onPaneRealized_gui(GtkWidget *pane, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	gint position = WGETI("transfer-pane-position");

	if (position > 10)
	{
		// @todo: fix get window height when maximized
		gint height;
		gtk_window_get_size(mw->window, NULL, &height);
		gtk_paned_set_position(GTK_PANED(pane), height - position);
	}
}

void MainWindow::onConnectClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;

	gtk_editable_select_region(GTK_EDITABLE(mw->getWidget("connectEntry")), 0, -1);
	gtk_widget_grab_focus(mw->getWidget("connectEntry"));
	int response = gtk_dialog_run(GTK_DIALOG(mw->getWidget("connectDialog")));
	gtk_widget_hide(mw->getWidget("connectDialog"));

	if (response == GTK_RESPONSE_OK)
	{
		string address = gtk_entry_get_text(GTK_ENTRY(mw->getWidget("connectEntry")));
		mw->showHub_gui(address);
	}
}

void MainWindow::onFavoriteHubsClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->showFavoriteHubs_gui();
}

void MainWindow::onPublicHubsClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->showPublicHubs_gui();
}

void MainWindow::onPreferencesClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	typedef Func0<MainWindow> F0;

	unsigned short tcpPort = (unsigned short)SETTING(TCP_PORT);
	unsigned short udpPort = (unsigned short)SETTING(UDP_PORT);
	int lastConn = SETTING(INCOMING_CONNECTIONS);

	gint response = WulforManager::get()->openSettingsDialog_gui();

	if (response == GTK_RESPONSE_OK)
	{
		if (SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != tcpPort || SETTING(UDP_PORT) != udpPort)
		{
			F0 *func = new F0(mw, &MainWindow::startSocket_client);
			WulforManager::get()->dispatchClientFunc(func);
		}

		if (BOOLSETTING(ALWAYS_TRAY))
			gtk_status_icon_set_visible(mw->statusIcon, TRUE);
		else
			gtk_status_icon_set_visible(mw->statusIcon, FALSE);

		mw->setTabPosition_gui(WGETI("tab-position"));
		mw->setToolbarStyle_gui(WGETI("toolbar-style"));

		// Reload the icons only if the setting has changed
		mw->loadIcons_gui();

		for (StringIterC it = mw->EntryList.begin(); it != mw->EntryList.end(); ++it)
		{
			BookEntry *entry = mw->findBookEntry(Entry::HUB, *it);

			if (entry != NULL)
				dynamic_cast<Hub*>(entry)->updateTags_gui();
			else
			{
				entry = mw->findBookEntry(Entry::PRIVATE_MESSAGE, *it);

				if (entry != NULL)
					dynamic_cast<PrivateMessage*>(entry)->updateTags_gui();
			}
		}
	}
}

void MainWindow::onHashClicked_gui(GtkWidget *widget, gpointer data)
{
	WulforManager::get()->openHashDialog_gui();
}

void MainWindow::onSearchClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->addSearch_gui();
}

void MainWindow::onDownloadQueueClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->showDownloadQueue_gui();
}

void MainWindow::onFinishedDownloadsClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->showFinishedDownloads_gui();
}

void MainWindow::onFinishedUploadsClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->showFinishedUploads_gui();
}

void MainWindow::onQuitClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	gboolean retVal; // Not interested in the value, though.
	g_signal_emit_by_name(mw->window, "delete-event", NULL, &retVal);
}

void MainWindow::onOpenFileListClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(mw->getWidget("flistDialog")), Text::fromUtf8(Util::getListPath()).c_str());

 	int ret = gtk_dialog_run(GTK_DIALOG(mw->getWidget("flistDialog")));
	gtk_widget_hide(mw->getWidget("flistDialog"));

	if (ret == GTK_RESPONSE_OK)
	{
		gchar *temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(mw->getWidget("flistDialog")));
		if (temp)
		{
			string path = Text::toUtf8(temp);
			g_free(temp);

			UserPtr user = DirectoryListing::getUserFromFilename(path);
			if (user)
				mw->showShareBrowser_gui(user, path, "", FALSE);
			else
				mw->setMainStatus_gui(_("Unable to load file list: Invalid file list name"));
		}
	}
}

void MainWindow::onOpenOwnListClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	typedef Func1<MainWindow, bool> F1;
	F1 *func = new F1(mw, &MainWindow::openOwnList_client, FALSE);
	WulforManager::get()->dispatchClientFunc(func);

	mw->setMainStatus_gui(_("Loading file list"));
}

void MainWindow::onRefreshFileListClicked_gui(GtkWidget *widget, gpointer data)
{
	typedef Func0<MainWindow> F0;
	F0 *func = new F0((MainWindow *)data, &MainWindow::refreshFileList_client);
	WulforManager::get()->dispatchClientFunc(func);
}

void MainWindow::onReconnectClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkWidget *entryWidget = mw->currentPage_gui();

	if (entryWidget)
	{
		BookEntry *entry = (BookEntry *)g_object_get_data(G_OBJECT(entryWidget), "entry");

		if (entry && entry->getType() == Entry::HUB)
		{
			Func0<Hub> *func = new Func0<Hub>(dynamic_cast<Hub *>(entry), &Hub::reconnect_client);
			WulforManager::get()->dispatchClientFunc(func);
		}
	}
}

void MainWindow::onCloseClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkWidget *entryWidget = mw->currentPage_gui();

	if (entryWidget)
	{
		BookEntry *entry = (BookEntry *)g_object_get_data(G_OBJECT(entryWidget), "entry");

		if (entry)
			mw->removeBookEntry_gui(entry);
	}
}

void MainWindow::onPreviousTabClicked_gui(GtkWidget* widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->previousTab_gui();
}

void MainWindow::onNextTabClicked_gui(GtkWidget* widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	mw->nextTab_gui();
}

void MainWindow::onAboutClicked_gui(GtkWidget *widget, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	gtk_dialog_run(GTK_DIALOG(mw->getWidget("aboutDialog")));
	gtk_widget_hide(mw->getWidget("aboutDialog"));
}

void MainWindow::onAboutDialogActivateLink_gui(GtkAboutDialog *dialog, const gchar *link, gpointer data)
{
	WulforUtil::openURI(link);
}

void MainWindow::onCloseBookEntry_gui(GtkWidget *widget, gpointer data)
{
	BookEntry *entry = (BookEntry *)data;
	WulforManager::get()->getMainWindow()->removeBookEntry_gui(entry);
}

void MainWindow::onStatusIconActivated_gui(GtkStatusIcon *statusIcon, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(mw->getWidget("statusIconShowInterfaceItem"));

	// Toggle the "Show Interface" check menu item. This will in turn invoke its callback.
	gboolean active = gtk_check_menu_item_get_active(item);
	gtk_check_menu_item_set_active(item, !active);
}

void MainWindow::onStatusIconPopupMenu_gui(GtkStatusIcon *statusIcon, guint button, guint time, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkMenu *menu = GTK_MENU(mw->getWidget("statusIconMenu"));
	gtk_menu_popup(menu, NULL, NULL, gtk_status_icon_position_menu, statusIcon, button, time);
}

void MainWindow::onShowInterfaceToggled_gui(GtkCheckMenuItem *item, gpointer data)
{
	MainWindow *mw = (MainWindow *)data;
	GtkWindow *win = mw->window;
	static int x, y;
	static bool isMaximized, isIconified;

	if (GTK_WIDGET_VISIBLE(win))
	{
		GdkWindowState state;
		gtk_window_get_position(win, &x, &y);
		state = gdk_window_get_state(GTK_WIDGET(win)->window);
		isMaximized = (state & GDK_WINDOW_STATE_MAXIMIZED);
		isIconified = (state & GDK_WINDOW_STATE_ICONIFIED);
		gtk_widget_hide(GTK_WIDGET(win));
	}
	else
	{
		gtk_window_move(win, x, y);
		if (isMaximized) gtk_window_maximize(win);
		if (isIconified) gtk_window_iconify(win);
		gtk_widget_show(GTK_WIDGET(win));
	}
}

void MainWindow::onLinkClicked_gui(GtkWidget *widget, const gchar *link)
{
	WulforUtil::openURI(link);
}

void MainWindow::autoConnect_client()
{
	FavoriteHubEntry *hub;
	FavoriteHubEntryList &l = FavoriteManager::getInstance()->getFavoriteHubs();
	typedef Func2<MainWindow, string, string> F2;
	F2 *func;

	for (FavoriteHubEntryList::const_iterator it = l.begin(); it != l.end(); ++it)
	{
		hub = *it;

		if (hub->getConnect())
		{
			func = new F2(this, &MainWindow::showHub_gui, hub->getServer(), hub->getEncoding());
			WulforManager::get()->dispatchGuiFunc(func);
		}
	}

	string link = WulforManager::get()->getURL();

	if (link.empty()) return;

	typedef Func1<MainWindow, string> F1;
	F1 *func1;

	if (WulforUtil::isHubURL(link) && BOOLSETTING(URL_HANDLER))
	{
		func = new F2(this, &MainWindow::showHub_gui, link, "");
		WulforManager::get()->dispatchGuiFunc(func);
	}
	else if (WulforUtil::isMagnet(link) && BOOLSETTING(MAGNET_REGISTER))
	{
		func1 = new F1(this, &MainWindow::addSearch_gui, link);
		WulforManager::get()->dispatchGuiFunc(func1);
	}
}

void MainWindow::startSocket_client()
{
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();

	if (ClientManager::getInstance()->isActive())
	{
		try
		{
			ConnectionManager::getInstance()->listen();
		}
		catch (const Exception &e)
		{
			string primaryText = _("Unable to open TCP/TLS port");
			string secondaryText = _("File transfers will not work correctly until you change settings or turn off any application that might be using the TCP/TLS port.");
			typedef Func2<MainWindow, string, string> F2;
			F2* func = new F2(this, &MainWindow::showMessageDialog_gui, primaryText, secondaryText);
			WulforManager::get()->dispatchGuiFunc(func);

		}

		try
		{
			SearchManager::getInstance()->listen();
		}
		catch (const Exception &e)
		{
			string primaryText = _("Unable to open UDP port");
			string secondaryText = _("Searching will not work correctly until you change settings or turn off any application that might be using the UDP port.");
			typedef Func2<MainWindow, string, string> F2;
			F2* func = new F2(this, &MainWindow::showMessageDialog_gui, primaryText, secondaryText);
			WulforManager::get()->dispatchGuiFunc(func);
		}
	}

	ClientManager::getInstance()->infoUpdated();
}

void MainWindow::refreshFileList_client()
{
	try
	{
		ShareManager::getInstance()->setDirty();
		ShareManager::getInstance()->refresh(TRUE, TRUE, FALSE);
	}
	catch (const ShareException&)
	{
	}
}

void MainWindow::openOwnList_client(bool useSetting)
{
	UserPtr user = ClientManager::getInstance()->getMe();
	string path = ShareManager::getInstance()->getOwnListFile();

	typedef Func4<MainWindow, UserPtr, string, string, bool> F4;
	F4 *func = new F4(this, &MainWindow::showShareBrowser_gui, user, path, "", useSetting);
	WulforManager::get()->dispatchGuiFunc(func);
}

string MainWindow::getFilename_client(Transfer *t)
{
	string filename;

 	if (t->getType() == Transfer::TYPE_FULL_LIST || t->getType() == Transfer::TYPE_PARTIAL_LIST)
		filename = _("File list");
	else if (t->getType() == Transfer::TYPE_TREE)
		filename = _("TTH: ") + Util::getFileName(t->getPath());
	else 
		filename = Util::getFileName(t->getPath());

	return filename;
}

void MainWindow::on(LogManagerListener::Message, time_t t, const string &message) throw()
{
	typedef Func2<MainWindow, string, time_t> F2;
	F2 *func = new F2(this, &MainWindow::setMainStatus_gui, message, t);
	WulforManager::get()->dispatchGuiFunc(func);
}

void MainWindow::on(QueueManagerListener::Finished, QueueItem *item, const string& dir, int64_t avSpeed) throw()
{
	typedef Func3<MainWindow, string, string, Notify::TypeNotify> F3;

	if (item->isSet(QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_USER_LIST))
	{
		UserPtr user = item->getDownloads()[0]->getUser();
		string listName = item->getListName();

		F3 *f3 = new F3(this, &MainWindow::showNotification_gui, _("file list from "), WulforUtil::getNicks(user),
			Notify::DOWNLOAD_FINISHED_USER_LIST);
		WulforManager::get()->dispatchGuiFunc(f3);

		typedef Func4<MainWindow, UserPtr, string, string, bool> F4;
		F4 *func = new F4(this, &MainWindow::showShareBrowser_gui, user, listName, dir, TRUE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
	else if (!item->isSet(QueueItem::FLAG_XML_BZLIST))
	{
		F3 *f3 = new F3(this, &MainWindow::showNotification_gui, _("<b>file:</b> "), item->getTarget(), Notify::DOWNLOAD_FINISHED);
		WulforManager::get()->dispatchGuiFunc(f3);
	}
}

void MainWindow::on(TimerManagerListener::Second, uint32_t ticks) throw()
{
	// Avoid calculating status update if it's not needed
	if (!BOOLSETTING(ALWAYS_TRAY) && minimized)
		return;

	int64_t diff = (int64_t)((lastUpdate == 0) ? ticks - 1000 : ticks - lastUpdate);
	int64_t downBytes = 0;
	int64_t upBytes = 0;

	if (diff > 0)
	{
		int64_t downDiff = Socket::getTotalDown() - lastDown;
		int64_t upDiff = Socket::getTotalUp() - lastUp;
		downBytes = (downDiff * 1000) / diff;
		upBytes = (upDiff * 1000) / diff;
	}

	string hubs = Client::getCounts();
	string downloadSpeed = Util::formatBytes(downBytes) + "/s";
	string downloaded = Util::formatBytes(Socket::getTotalDown());
	string uploadSpeed = Util::formatBytes(upBytes) + "/s";
	string uploaded = Util::formatBytes(Socket::getTotalUp());

	lastUpdate = ticks;
	lastUp = Socket::getTotalUp();
	lastDown = Socket::getTotalDown();

	typedef Func5<MainWindow, string, string, string, string, string> F5;
	F5 *func = new F5(this, &MainWindow::setStats_gui, hubs, downloadSpeed, downloaded, uploadSpeed, uploaded);
	WulforManager::get()->dispatchGuiFunc(func);

	if (BOOLSETTING(ALWAYS_TRAY) && !downloadSpeed.empty() && !uploadSpeed.empty())
	{
		typedef Func2<MainWindow, string, string> F2;
		F2 *f2 = new F2(this, &MainWindow::updateStatusIconTooltip_gui, downloadSpeed, uploadSpeed);
		WulforManager::get()->dispatchGuiFunc(f2);
	}
}
