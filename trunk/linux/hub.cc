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

#include "hub.hh"

#include <dcpp/FavoriteManager.h>
#include <dcpp/HashManager.h>
#include <dcpp/SearchManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/UserCommand.h>
#include "privatemessage.hh"
#include "search.hh"
#include "settingsmanager.hh"
#include "UserCommandMenu.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"

using namespace std;
using namespace dcpp;

Hub::Hub(const string &address, const string &encoding):
	BookEntry(Entry::HUB, _("Hub: ") + address, "hub.glade", address),
	client(NULL),
	historyIndex(0),
	totalShared(0),
	selectedTag(NULL),
	address(address),
	encoding(encoding),
	scrollToBottom(TRUE)
{
	// Configure the dialog
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(getWidget("passwordDialog")), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	// Initialize nick treeview
	nickView.setView(GTK_TREE_VIEW(getWidget("nickView")), true, "hub");
	nickView.insertColumn("Nick", G_TYPE_STRING, TreeView::PIXBUF_STRING, 100, "Icon");
	nickView.insertColumn("Shared", G_TYPE_INT64, TreeView::BYTE, 75);
	nickView.insertColumn("Description", G_TYPE_STRING, TreeView::STRING, 85);
	nickView.insertColumn("Tag", G_TYPE_STRING, TreeView::STRING, 100);
	nickView.insertColumn("Connection", G_TYPE_STRING, TreeView::STRING, 85);
	nickView.insertColumn("eMail", G_TYPE_STRING, TreeView::STRING, 90);
	nickView.insertHiddenColumn("Icon", GDK_TYPE_PIXBUF);
	nickView.insertHiddenColumn("Nick Order", G_TYPE_STRING);
	nickView.insertHiddenColumn("CID", G_TYPE_STRING);
	nickView.finalize();
	nickStore = gtk_list_store_newv(nickView.getColCount(), nickView.getGTypes());
	gtk_tree_view_set_model(nickView.get(), GTK_TREE_MODEL(nickStore));
	g_object_unref(nickStore);
	nickSelection = gtk_tree_view_get_selection(nickView.get());
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(nickView.get()), GTK_SELECTION_MULTIPLE);
	nickView.setSortColumn_gui("Nick", "Nick Order");
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(nickStore), nickView.col("Nick Order"), GTK_SORT_ASCENDING);
	gtk_tree_view_column_set_sort_indicator(gtk_tree_view_get_column(nickView.get(), nickView.col("Nick")), TRUE);
	gtk_tree_view_set_fixed_height_mode(nickView.get(), TRUE);

	// Initialize the chat window
	if (BOOLSETTING(USE_OEM_MONOFONT))
	{
		PangoFontDescription *fontDesc = pango_font_description_new();
		pango_font_description_set_family(fontDesc, "Mono");
		gtk_widget_modify_font(getWidget("chatText"), fontDesc);
		pango_font_description_free(fontDesc);
	}
	chatBuffer = gtk_text_buffer_new(NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(getWidget("chatText")), chatBuffer);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(chatBuffer, &iter);
	chatMark = gtk_text_buffer_create_mark(chatBuffer, NULL, &iter, FALSE);
	handCursor = gdk_cursor_new(GDK_HAND2);

	// Load the icons for the nick list
	string path = WulforManager::get()->getPath() + "/pixmaps/";
	string icon = path + "normal.png";
	userIcons["normal"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "normal-op.png";
	userIcons["normal-op"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "normal-fw.png";
	userIcons["normal-fw"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "normal-fw-op.png";
	userIcons["normal-fw-op"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "dc++.png";
	userIcons["dc++"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "dc++-op.png";
	userIcons["dc++-op"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "dc++-fw.png";
	userIcons["dc++-fw"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);
	icon = path + "dc++-fw-op.png";
	userIcons["dc++-fw-op"] = gdk_pixbuf_new_from_file(icon.c_str(), NULL);

	// Initialize the user command menu
	userCommandMenu = new UserCommandMenu(getWidget("usercommandMenu"), ::UserCommand::CONTEXT_CHAT);
	addChild(userCommandMenu);

	GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(getWidget("chatScroll")));

	// Connect the signals to their callback functions.
	g_signal_connect(getContainer(), "focus-in-event", G_CALLBACK(onFocusIn_gui), (gpointer)this);
	g_signal_connect(nickView.get(), "button-press-event", G_CALLBACK(onNickListButtonPress_gui), (gpointer)this);
	g_signal_connect(nickView.get(), "button-release-event", G_CALLBACK(onNickListButtonRelease_gui), (gpointer)this);
	g_signal_connect(nickView.get(), "key-release-event", G_CALLBACK(onNickListKeyRelease_gui), (gpointer)this);
	g_signal_connect(getWidget("chatEntry"), "activate", G_CALLBACK(onSendMessage_gui), (gpointer)this);
	g_signal_connect(getWidget("chatEntry"), "key-press-event", G_CALLBACK(onEntryKeyPress_gui), (gpointer)this);
	g_signal_connect(getWidget("chatText"), "motion-notify-event", G_CALLBACK(onChatPointerMoved_gui), (gpointer)this);
	g_signal_connect(getWidget("chatText"), "visibility-notify-event", G_CALLBACK(onChatVisibilityChanged_gui), (gpointer)this);
	g_signal_connect(adjustment, "value_changed", G_CALLBACK(onChatScroll_gui), (gpointer)this);
	g_signal_connect(adjustment, "changed", G_CALLBACK(onChatResize_gui), (gpointer)this);
	g_signal_connect(getWidget("copyNickItem"), "activate", G_CALLBACK(onCopyNickItemClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("browseItem"), "activate", G_CALLBACK(onBrowseItemClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("matchItem"), "activate", G_CALLBACK(onMatchItemClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("msgItem"), "activate", G_CALLBACK(onMsgItemClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("grantItem"), "activate", G_CALLBACK(onGrantItemClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyLinkItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openLinkItem"), "activate", G_CALLBACK(onOpenLinkClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyhubItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openhubItem"), "activate", G_CALLBACK(onOpenHubClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyMagnetItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("searchMagnetItem"), "activate", G_CALLBACK(onSearchMagnetClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("magnetPropertiesItem"), "activate", G_CALLBACK(onMagnetPropertiesClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("removeUserItem"), "activate", G_CALLBACK(onRemoveUserItemClicked_gui), (gpointer)this);

	gtk_widget_set_sensitive(getWidget("favoriteUserItem"), FALSE); // Not implemented yet
	gtk_widget_grab_focus(getWidget("chatEntry"));

	// Set the pane position
	gint panePosition = WGETI("nick-pane-position");
	if (panePosition > 10)
	{
		gint width;
		GtkWindow *window = GTK_WINDOW(WulforManager::get()->getMainWindow()->getContainer());
		gtk_window_get_size(window, &width, NULL);
		gtk_paned_set_position(GTK_PANED(getWidget("pane")), width - panePosition);
	}

	history.push_back("");
}

Hub::~Hub()
{
	disconnect_client();

	unordered_map<string, GdkPixbuf *>::iterator it;
	for (it = userIcons.begin(); it != userIcons.end(); ++it)
		g_object_unref(G_OBJECT(it->second));

	// Save the pane position
	gint width;
	GtkWindow *window = GTK_WINDOW(WulforManager::get()->getMainWindow()->getContainer());
	gtk_window_get_size(window, &width, NULL);
	gint panePosition = width - gtk_paned_get_position(GTK_PANED(getWidget("pane")));
	if (panePosition > 10)
		WSET("nick-pane-position", panePosition);

	gtk_widget_destroy(getWidget("passwordDialog"));

	if (handCursor)
	{
		gdk_cursor_unref(handCursor);
		handCursor = NULL;
	}
}

void Hub::show()
{
	// Connect to the hub
	typedef Func2<Hub, string, string> F2;
	F2 *func = new F2(this, &Hub::connectClient_client, address, encoding);
	WulforManager::get()->dispatchClientFunc(func);
}

void Hub::setStatus_gui(string statusBar, string text)
{
	if (!statusBar.empty() && !text.empty())
	{
		if (statusBar == "statusMain")
			text = "[" + Util::getShortTimeString() + "] " + text;

		gtk_statusbar_pop(GTK_STATUSBAR(getWidget(statusBar)), 0);
		gtk_statusbar_push(GTK_STATUSBAR(getWidget(statusBar)), 0, text.c_str());
	}
}

bool Hub::findUser_gui(const string &cid, GtkTreeIter *iter)
{
	unordered_map<std::string, GtkTreeIter>::const_iterator it = userIters.find(cid);

	if (it != userIters.end())
	{
		if (iter)
			*iter = it->second;

		return TRUE;
	}

	return FALSE;
}

bool Hub::findNick_gui(const string &nick, GtkTreeIter *iter)
{
	unordered_map<std::string, std::string>::const_iterator it = userMap.find(nick);

	if (it != userMap.end())
		return findUser_gui(it->second, iter);

	return FALSE;
}

void Hub::updateUser_gui(ParamMap params)
{
	GtkTreeIter iter;
	int64_t shared = Util::toInt64(params["Shared"]);
	const string& cid = params["CID"];

	if (findUser_gui(cid, &iter))
	{
		totalShared += shared - nickView.getValue<int64_t>(&iter, "Shared");
		string nick = nickView.getString(&iter,"Nick");

		if (nick != params["Nick"])
		{
			// User has changed nick, update userMap and remove the old Nick tag
			userMap.erase(nick);
			removeTag_gui(nick);
			userMap[params["Nick"]] = cid;
		}
	
		gtk_list_store_set(nickStore, &iter,
			nickView.col("Nick"), params["Nick"].c_str(),
			nickView.col("Shared"), shared,
			nickView.col("Description"), params["Description"].c_str(),
			nickView.col("Tag"), params["Tag"].c_str(),
 			nickView.col("Connection"), params["Connection"].c_str(),
			nickView.col("eMail"), params["eMail"].c_str(),
 			nickView.col("Icon"), userIcons[params["Icon"]],
			nickView.col("Nick Order"), params["Nick Order"].c_str(),
			nickView.col("CID"), cid.c_str(),
			-1);
	}
	else
	{
		totalShared += shared;
		userMap[params["Nick"]] = cid;

		if (BOOLSETTING(SHOW_JOINS))
		{
			addStatusMessage_gui(params["Nick"] + _(" has joined"), Sound::NONE);
			string line = params["Nick"] + _(" has joined hub ") + client->getHubName();
			WulforManager::get()->getMainWindow()->addPrivateStatusMessage_gui(cid, line);
		}
		else if (BOOLSETTING(FAV_SHOW_JOINS))
		{
			typedef Func1<Hub, string> F1;
			F1 *func = new F1(this, &Hub::checkFavoriteUserJoin_client, cid);
			WulforManager::get()->dispatchClientFunc(func);
		}

		gtk_list_store_insert_with_values(nickStore, &iter, userMap.size(),
			nickView.col("Nick"), params["Nick"].c_str(),
			nickView.col("Shared"), shared,
			nickView.col("Description"), params["Description"].c_str(),
			nickView.col("Tag"), params["Tag"].c_str(),
 			nickView.col("Connection"), params["Connection"].c_str(),
			nickView.col("eMail"), params["eMail"].c_str(),
 			nickView.col("Icon"), userIcons[params["Icon"]],
			nickView.col("Nick Order"), params["Nick Order"].c_str(),
			nickView.col("CID"), cid.c_str(),
			-1);

		userIters[cid] = iter;
	}

	setStatus_gui("statusUsers", Util::toString(userMap.size()) + _(" Users"));
	setStatus_gui("statusShared", Util::formatBytes(totalShared));
}

void Hub::removeUser_gui(string cid)
{
	GtkTreeIter iter;
	string nick;

	if (findUser_gui(cid, &iter))
	{
		nick = nickView.getString(&iter, "Nick");
		totalShared -= nickView.getValue<int64_t>(&iter, "Shared");
		gtk_list_store_remove(nickStore, &iter);
		removeTag_gui(nick);
		userMap.erase(nick);
		userIters.erase(cid);
		setStatus_gui("statusUsers", Util::toString(userMap.size()) + _(" Users"));
		setStatus_gui("statusShared", Util::formatBytes(totalShared));
	}
}

/*
 * Remove nick tag from text view
 */
void Hub::removeTag_gui(const std::string &nick)
{
	GtkTextTagTable *textTagTable = gtk_text_buffer_get_tag_table(chatBuffer);
	GtkTextTag *tag = gtk_text_tag_table_lookup(textTagTable, nick.c_str());
	if (tag)
		gtk_text_tag_table_remove(textTagTable, tag);
}

void Hub::clearNickList_gui()
{
	// Remove all old nick tags from the text view
	unordered_map<string, string>::const_iterator it;
	for (it = userMap.begin(); it != userMap.end(); ++it)
		removeTag_gui(it->first);

	gtk_list_store_clear(nickStore);
	userMap.clear();
	userIters.clear();
	totalShared = 0;
	setStatus_gui("statusUsers", _("0 Users"));
	setStatus_gui("statusShared", "0 B");
}

void Hub::popupNickMenu_gui()
{
	// Build user command menu
	userCommandMenu->cleanMenu_gui();

	GtkTreeIter iter;
	GList *list = gtk_tree_selection_get_selected_rows(nickSelection, NULL);

	for (GList *i = list; i; i = i->next)
	{
		GtkTreePath *path = (GtkTreePath *)i->data;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(nickStore), &iter, path))
 		{
			userCommandMenu->addUser(nickView.getString(&iter, "CID"));
		}
		gtk_tree_path_free(path);
	}
	g_list_free(list);

	userCommandMenu->addHub(client->getHubUrl());
	userCommandMenu->buildMenu_gui();

	gtk_menu_popup(GTK_MENU(getWidget("nickMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	gtk_widget_show_all(getWidget("nickMenu"));
}

void Hub::getPassword_gui()
{
	gint ret;

	ret = gtk_dialog_run(GTK_DIALOG(getWidget("passwordDialog")));
	gtk_widget_hide(getWidget("passwordDialog"));

	if (ret == GTK_RESPONSE_OK)
	{
		string password = gtk_entry_get_text(GTK_ENTRY(getWidget("passwordEntry")));
		typedef Func1<Hub, string> F1;
		F1 *func = new F1(this, &Hub::setPassword_client, password);
		WulforManager::get()->dispatchClientFunc(func);
	}
	else
		client->disconnect(TRUE);
}

void Hub::addStatusMessage_gui(string message, Sound::TypeSound sound)
{
	if (!message.empty())
	{
		if (sound != Sound::NONE)
			Sound::get()->playSound(sound);

		setStatus_gui("statusMain", message);

		if (BOOLSETTING(STATUS_IN_CHAT))
		{
			string line = "*** " + message;
			addMessage_gui(line);
		}
	}
}

void Hub::addMessage_gui(string message)
{
	if (message.empty())
		return;

	GtkTextIter iter;
	string line = "";

	// Add a new line if this isn't the first line in buffer.
	if (gtk_text_buffer_get_char_count(chatBuffer) > 0)
		line = "\n";

	if (BOOLSETTING(TIME_STAMPS))
		line += "[" + Util::getShortTimeString() + "] ";

	line += message;

	gtk_text_buffer_get_end_iter(chatBuffer, &iter);
	gtk_text_buffer_insert(chatBuffer, &iter, line.c_str(), line.size());

	applyTags_gui(line);

	gtk_text_buffer_get_end_iter(chatBuffer, &iter);

	// Limit size of chat text
	if (gtk_text_buffer_get_line_count(chatBuffer) > maxLines)
	{
		GtkTextIter next;
		gtk_text_buffer_get_start_iter(chatBuffer, &iter);
		gtk_text_buffer_get_iter_at_line(chatBuffer, &next, 1);
		gtk_text_buffer_delete(chatBuffer, &iter, &next);
	}
}

void Hub::applyTags_gui(const string &line)
{
	GtkTextIter iter;
	GtkTextIter startIter;
	GtkTextIter endIter;
	bool firstNick = FALSE;
	string::size_type start;
	string::size_type end = 0;

	gtk_text_buffer_get_end_iter(chatBuffer, &iter);

	// Tag nicknames and URIs
	while ((start = line.find_first_not_of(" \n\r\t", end)) != string::npos)
	{
		end = line.find_first_of(" \n\r\t", start);
		if (end == string::npos)
			end = line.size();

		// Special case: catch nicks in the form <nick> at the beginning of the line.
		if (!firstNick && start < end - 1 && line[start] == '<' && line[end - 1] == '>')
		{
			++start;
			--end;
			firstNick = TRUE;
		}

		GCallback callback = NULL;
		bool isNick = FALSE;
		string tagName = line.substr(start, end - start);

		if (findNick_gui(tagName, NULL))
		{
			isNick = TRUE;
			callback = G_CALLBACK(onNickTagEvent_gui);
		}
		else
		{
			if (WulforUtil::isLink(tagName))
				callback = G_CALLBACK(onLinkTagEvent_gui);
			else if (WulforUtil::isHubURL(tagName))
				callback = G_CALLBACK(onHubTagEvent_gui);
			else if (WulforUtil::isMagnet(tagName))
				callback = G_CALLBACK(onMagnetTagEvent_gui);
		}

		if (callback)
		{
			// check for the tag in our buffer
			GtkTextTag *tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(chatBuffer), tagName.c_str());

			if (!tag)
			{
				if (isNick)
					tag = gtk_text_buffer_create_tag(chatBuffer, tagName.c_str(), "style", PANGO_STYLE_ITALIC, NULL);
				else
					tag = gtk_text_buffer_create_tag(chatBuffer, tagName.c_str(), "foreground", "blue", "underline", PANGO_UNDERLINE_SINGLE, NULL);

				g_signal_connect(tag, "event", callback, (gpointer)this);
			}

			startIter = endIter = iter;
			gtk_text_iter_backward_chars(&startIter, g_utf8_strlen(line.c_str() + start, -1));
			gtk_text_iter_backward_chars(&endIter, g_utf8_strlen(line.c_str() + end, -1));
			gtk_text_buffer_apply_tag(chatBuffer, tag, &startIter, &endIter);
		}
	}
}

/*
 * Unfortunately, we can't underline the tag on mouse over since it would
 * underline all the tags with that name.
 */
void Hub::updateCursor_gui(GtkWidget *widget)
{
	gint x, y, buf_x, buf_y;
	GtkTextIter iter;
	GSList *tagList;
	GtkTextTag *newTag = NULL;

	gdk_window_get_pointer(widget->window, &x, &y, NULL);

	// Check for tags under the cursor, and change mouse cursor appropriately
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_WIDGET, x, y, &buf_x, &buf_y);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, buf_x, buf_y);
	tagList = gtk_text_iter_get_tags(&iter);

	if (tagList != NULL) 
	{
		newTag = GTK_TEXT_TAG(tagList->data);
		g_slist_free(tagList);
	}

	if (newTag != selectedTag) 
	{
		// Cursor is in transition.
		if (newTag != NULL) 
		{
			// Cursor is entering a tag.
			selectedTagStr = newTag->name;
			if (selectedTag == NULL)
			{
				// Cursor was in neutral space.
				gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT), handCursor);
			}
		}
		else 
		{
			// Cursor is entering neutral space.
			gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT), NULL);
		}
		selectedTag = newTag;
	}
}

gboolean Hub::onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	gtk_widget_grab_focus(hub->getWidget("chatEntry"));

	return TRUE;
}

gboolean Hub::onNickListButtonPress_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
		hub->oldType = event->type;

	if (event->button == 3)
	{
		GtkTreePath *path;
		if (gtk_tree_view_get_path_at_pos(hub->nickView.get(), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL))
		{
			bool selected = gtk_tree_selection_path_is_selected(hub->nickSelection, path);
			gtk_tree_path_free(path);

			if (selected)
				return TRUE;
		}
	}

	return FALSE;
}

gboolean Hub::onNickListButtonRelease_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		if (event->button == 1 && hub->oldType == GDK_2BUTTON_PRESS)
		{
			hub->onBrowseItemClicked_gui(NULL, data);
		}
		else if (event->button == 2 && event->type == GDK_BUTTON_RELEASE)
		{
			hub->onMsgItemClicked_gui(NULL, data);
		}
		else if (event->button == 3 && event->type == GDK_BUTTON_RELEASE)
		{
			hub->popupNickMenu_gui();
		}
	}

	return FALSE;
}

gboolean Hub::onNickListKeyRelease_gui(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		if (event->keyval == GDK_Menu || (event->keyval == GDK_F10 && event->state & GDK_SHIFT_MASK))
		{
			hub->popupNickMenu_gui();
		}
		else if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
		{
			hub->onBrowseItemClicked_gui(NULL, data);
		}
	}

	return FALSE;
}

gboolean Hub::onEntryKeyPress_gui(GtkWidget *entry, GdkEventKey *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up)
	{
		size_t index = hub->historyIndex - 1;
		if (index >= 0 && index < hub->history.size())
		{
			hub->historyIndex = index;
			gtk_entry_set_text(GTK_ENTRY(entry), hub->history[index].c_str());
		}
		return TRUE;
	}
	else if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
	{
		size_t index = hub->historyIndex + 1;
		if (index >= 0 && index < hub->history.size())
		{
			hub->historyIndex = index;
			gtk_entry_set_text(GTK_ENTRY(entry), hub->history[index].c_str());
		}
		return TRUE;
	}
	else if (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab)
	{
		string current;
		string::size_type start, end;
		string text(gtk_entry_get_text(GTK_ENTRY(entry)));
		int curpos = gtk_editable_get_position(GTK_EDITABLE(entry));

		// Allow tab to focus other widgets if entry is empty
		if (curpos <= 0 && text.empty())
			return FALSE;

		// Erase ": " at the end of the nick.
		if (curpos > 2 && text.substr(curpos - 2, 2) == ": ")
		{
			text.erase(curpos - 2, 2);
			curpos -= 2;
		}

		start = text.rfind(' ', curpos - 1);
		end = text.find(' ', curpos - 1);

		// Text to match starts at the beginning
		if (start == string::npos)
			start = 0;
		else
			++start;

		if (start < end)
		{
			current = text.substr(start, end - start);

			if (hub->completionKey.empty() || Text::toLower(current).find(Text::toLower(hub->completionKey)) == string::npos)
				hub->completionKey = current;

			GtkTreeIter iter;
			bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(hub->nickStore), &iter);
			bool useNext = (current == hub->completionKey);
			string key = Text::toLower(hub->completionKey);
			string complete = hub->completionKey;

			while (valid)
			{
				string nick = hub->nickView.getString(&iter, "Nick");
				string::size_type tagEnd = 0;
				if (useNext && (tagEnd = Text::toLower(nick).find(key)) != string::npos)
				{
					if (tagEnd == 0 || nick.find_first_of("]})", tagEnd - 1) == tagEnd - 1)
					{
						complete = nick;
						if (start <= 0)
							complete.append(": ");
						break;
					}
				}

				if (nick == current)
					useNext = TRUE;

				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(hub->nickStore),&iter);
			}

			text.replace(start, end - start, complete);
			gtk_entry_set_text(GTK_ENTRY(entry), text.c_str());
			gtk_editable_set_position(GTK_EDITABLE(entry), start + complete.length());
		}
		else
			hub->completionKey.clear();

		return TRUE;
	}

	hub->completionKey.clear();
	return FALSE;
}

gboolean Hub::onNickTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		GtkTreeIter nickIter;
		if (hub->findNick_gui(tag->name, &nickIter))
		{
			// Select the user in the nick list view
			GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(hub->nickStore), &nickIter);
			gtk_tree_view_scroll_to_cell(hub->nickView.get(), path, gtk_tree_view_get_column(hub->nickView.get(), hub->nickView.col("Nick")), FALSE, 0.0, 0.0);
			gtk_tree_view_set_cursor(hub->nickView.get(), path, NULL, FALSE);
			gtk_tree_path_free(path);

			if (event->button.button == 3)
				hub->popupNickMenu_gui();
		}

		return TRUE;
	}
	return FALSE;
}

gboolean Hub::onLinkTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button.button)
		{
			case 1:
				onOpenLinkClicked_gui(NULL, data);
				break;
			case 3:
				// Popup uri context menu
				gtk_widget_show_all(hub->getWidget("linkMenu"));
				gtk_menu_popup(GTK_MENU(hub->getWidget("linkMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean Hub::onHubTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button.button)
		{
			case 1:
				onOpenHubClicked_gui(NULL, data);
				break;
			case 3:
				// Popup uri context menu
				gtk_widget_show_all(hub->getWidget("hubMenu"));
				gtk_menu_popup(GTK_MENU(hub->getWidget("hubMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean Hub::onMagnetTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button.button)
		{
			case 1:
				// Search for magnet
				onSearchMagnetClicked_gui(NULL, data);
				break;
			case 3:
				// Popup magnet context menu
				gtk_widget_show_all(hub->getWidget("magnetMenu"));
				gtk_menu_popup(GTK_MENU(hub->getWidget("magnetMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean Hub::onChatPointerMoved_gui(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	hub->updateCursor_gui(widget);

	return FALSE;
}

gboolean Hub::onChatVisibilityChanged_gui(GtkWidget *widget, GdkEventVisibility *event, gpointer data)
{
	Hub *hub = (Hub *)data;

	hub->updateCursor_gui(widget);

	return FALSE;
}

void Hub::onChatScroll_gui(GtkAdjustment *adjustment, gpointer data)
{
	Hub *hub = (Hub *)data;
	gdouble value = gtk_adjustment_get_value(adjustment);
	hub->scrollToBottom = value >= (adjustment->upper - adjustment->page_size);
}

void Hub::onChatResize_gui(GtkAdjustment *adjustment, gpointer data)
{
	Hub *hub = (Hub *)data;
	gdouble value = gtk_adjustment_get_value(adjustment);

	if (hub->scrollToBottom && value < (adjustment->upper - adjustment->page_size))
	{
		GtkTextIter iter;

		gtk_text_buffer_get_end_iter(hub->chatBuffer, &iter);
		gtk_text_buffer_move_mark(hub->chatBuffer, hub->chatMark, &iter);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(hub->getWidget("chatText")), hub->chatMark, 0, FALSE, 0, 0);
	}
}

void Hub::onSendMessage_gui(GtkEntry *entry, gpointer data)
{
	string text = gtk_entry_get_text(entry);
	if (text.empty())
		return;

	gtk_entry_set_text(entry, "");
	Hub *hub = (Hub *)data;
	typedef Func1<Hub, string> F1;
	F1 *func;
	typedef Func2<Hub, string, bool> F2;
	F2 *func2;

	// Store line in chat history
	hub->history.pop_back();
	hub->history.push_back(text);
	hub->history.push_back("");
	hub->historyIndex = hub->history.size() - 1;
	if (hub->history.size() > maxHistory + 1)
		hub->history.erase(hub->history.begin());

	// Process special commands
	if (text[0] == '/')
	{
		string command, param;
		string::size_type separator = text.find_first_of(' ');
		if (separator != string::npos && text.size() > separator + 1)
		{
			command = text.substr(1, separator - 1);
			param = text.substr(separator + 1);
		}
		else
		{
			command = text.substr(1);
		}
		std::transform(command.begin(), command.end(), command.begin(), (int(*)(int))tolower);

		if (command == _("away"))
		{
			if (Util::getAway() && param.empty())
			{
				Util::setAway(FALSE);
				Util::setManualAway(FALSE);
				hub->addStatusMessage_gui(_("Away mode off"), Sound::NONE);
			}
			else
			{
				Util::setAway(TRUE);
				Util::setManualAway(TRUE);
				Util::setAwayMessage(param);
				hub->addStatusMessage_gui(_("Away mode on: ") + Util::getAwayMessage(), Sound::NONE);
			}
		}
		else if (command == _("back"))
		{
			Util::setAway(FALSE);
			hub->addStatusMessage_gui(_("Away mode off"), Sound::NONE);
		}
		else if (command == _("clear"))
		{
			GtkTextIter startIter, endIter;
			gtk_text_buffer_get_start_iter(hub->chatBuffer, &startIter);
			gtk_text_buffer_get_end_iter(hub->chatBuffer, &endIter);
			gtk_text_buffer_delete(hub->chatBuffer, &startIter, &endIter);
		}
		else if (command == _("close"))
		{
			/// @todo: figure out why this sometimes closes and reopens the tab
			WulforManager::get()->getMainWindow()->removeBookEntry_gui(hub);
		}
		else if (command == _("favorite") || command == _("fav"))
		{
			WulforManager::get()->dispatchClientFunc(new Func0<Hub>(hub, &Hub::addAsFavorite_client));
		}
		else if (command == _("getlist"))
		{
			if (hub->userMap.find(param) != hub->userMap.end())
			{
				func2 = new F2(hub, &Hub::getFileList_client, hub->userMap[param], FALSE);
				WulforManager::get()->dispatchClientFunc(func2);
			}
			else
				hub->addStatusMessage_gui(_("User not found"), Sound::NONE);
		}
		else if (command == _("grant"))
		{
			if (hub->userMap.find(param) != hub->userMap.end())
			{
				func = new F1(hub, &Hub::grantSlot_client, hub->userMap[param]);
				WulforManager::get()->dispatchClientFunc(func);
			}
			else
				hub->addStatusMessage_gui(_("User not found"), Sound::NONE);
		}
		else if (command == _("help"))
		{
			hub->addStatusMessage_gui(_("Available commands: /away <message>, /back, /clear, /close, /favorite, "\
				 "/getlist <nick>, /grant <nick>, /help, /join <address>, /me <message>, /pm <nick>, /rebuild, /refresh, /userlist"), Sound::NONE);
		}
		else if (command == _("join") && !param.empty())
		{
			if (BOOLSETTING(JOIN_OPEN_NEW_WINDOW))
			{
				// Assumption: new hub is same encoding as current hub.
				WulforManager::get()->getMainWindow()->showHub_gui(param, hub->encoding);
			}
			else
			{
				typedef Func2<Hub, string, bool> F2;
				F2 *func = new F2(hub, &Hub::redirect_client, param, TRUE);
				WulforManager::get()->dispatchClientFunc(func);
			}
		}
		else if (command == _("me"))
		{
			func2 = new F2(hub, &Hub::sendMessage_client, param, true);
			WulforManager::get()->dispatchClientFunc(func2);
		}
		else if (command == _("pm"))
		{
			if (hub->userMap.find(param) != hub->userMap.end())
				WulforManager::get()->getMainWindow()->addPrivateMessage_gui(hub->userMap[param]);
			else
				hub->addStatusMessage_gui(_("User not found"), Sound::NONE);
		}
		else if (command == _("rebuild"))
		{
			WulforManager::get()->dispatchClientFunc(new Func0<Hub>(hub, &Hub::rebuildHashData_client));
		}
		else if (command == _("refresh"))
		{
			WulforManager::get()->dispatchClientFunc(new Func0<Hub>(hub, &Hub::refreshFileList_client));
		}
		else if (command == _("userlist"))
		{
			if (GTK_WIDGET_VISIBLE(hub->getWidget("scrolledwindow2")))
				gtk_widget_hide(hub->getWidget("scrolledwindow2"));
			else
				gtk_widget_show_all(hub->getWidget("scrolledwindow2"));
		}
		else if (BOOLSETTING(SEND_UNKNOWN_COMMANDS))
		{
			func2 = new F2(hub, &Hub::sendMessage_client, text, false);
			WulforManager::get()->dispatchClientFunc(func2);
		}
		else
		{
			hub->addStatusMessage_gui(_("Unknown command '") + text + _("': type /help for a list of available commands"), Sound::NONE);
		}

	}
	else
	{
		func2 = new F2(hub, &Hub::sendMessage_client, text, false);
		WulforManager::get()->dispatchClientFunc(func2);
	}
}

void Hub::onCopyNickItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string nicks;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				nicks += hub->nickView.getString(&iter, "Nick") + ' ';
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);

		if (!nicks.empty())
		{
			nicks.erase(nicks.length() - 1);
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), nicks.c_str(), nicks.length());
		}
	}
}

void Hub::onBrowseItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		typedef Func2<Hub, string, bool> F2;
		F2 *func;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				cid = hub->nickView.getString(&iter, "CID");
				func = new F2(hub, &Hub::getFileList_client, cid, FALSE);
				WulforManager::get()->dispatchClientFunc(func);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Hub::onMatchItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		typedef Func2<Hub, string, bool> F2;
		F2 *func;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				cid = hub->nickView.getString(&iter, "CID");
				func = new F2(hub, &Hub::getFileList_client, cid, TRUE);
				WulforManager::get()->dispatchClientFunc(func);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Hub::onMsgItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				cid = hub->nickView.getString(&iter, "CID");
				WulforManager::get()->getMainWindow()->addPrivateMessage_gui(cid);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Hub::onGrantItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		typedef Func1<Hub, string> F1;
		F1 *func;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				cid = hub->nickView.getString(&iter, "CID");
				func = new F1(hub, &Hub::grantSlot_client, cid);
				WulforManager::get()->dispatchClientFunc(func);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Hub::onRemoveUserItemClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	if (gtk_tree_selection_count_selected_rows(hub->nickSelection) > 0)
	{
		string cid;
		GtkTreeIter iter;
		GtkTreePath *path;
		typedef Func1<Hub, string> F1;
		F1 *func;
		GList *list = gtk_tree_selection_get_selected_rows(hub->nickSelection, NULL);

		for (GList *i = list; i; i = i->next)
		{
			path = (GtkTreePath *)i->data;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(hub->nickStore), &iter, path))
			{
				cid = hub->nickView.getString(&iter, "CID");
				func = new F1(hub, &Hub::removeUserFromQueue_client, cid);
				WulforManager::get()->dispatchClientFunc(func);
			}
			gtk_tree_path_free(path);
		}
		g_list_free(list);
	}
}

void Hub::onCopyURIClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), hub->selectedTagStr.c_str(), hub->selectedTagStr.length());
}

void Hub::onOpenLinkClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	WulforUtil::openURI(hub->selectedTagStr);
}

void Hub::onOpenHubClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	WulforManager::get()->getMainWindow()->showHub_gui(hub->selectedTagStr);
}

void Hub::onSearchMagnetClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;
	string name;
	int64_t size;
	string tth;

	if (WulforUtil::splitMagnet(hub->selectedTagStr, name, size, tth))
	{
		Search *s = WulforManager::get()->getMainWindow()->addSearch_gui();
		s->putValue_gui(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
	}
}

void Hub::onMagnetPropertiesClicked_gui(GtkMenuItem *item, gpointer data)
{
	Hub *hub = (Hub *)data;

	WulforManager::get()->getMainWindow()->openMagnetDialog_gui(hub->selectedTagStr);
}

void Hub::connectClient_client(string address, string encoding)
{
	dcassert(client == NULL);

	if (address.substr(0, 6) == "adc://" || address.substr(0, 7) == "adcs://")
		encoding = "UTF-8";
	else if (encoding.empty() || encoding == WulforUtil::ENCODING_GLOBAL_HUB_DEFAULT)
		encoding = WGETS("default-charset");

	if (encoding == WulforUtil::ENCODING_SYSTEM_DEFAULT)
		encoding = Text::systemCharset;

	// Only pick "UTF-8" part of "UTF-8 (Unicode)".
	string::size_type i = encoding.find(' ', 0);
	if (i != string::npos)
		encoding = encoding.substr(0, i);

	client = ClientManager::getInstance()->getClient(address);
	client->setEncoding(encoding);
	client->addListener(this);
	client->connect();
}

void Hub::disconnect_client()
{
	if (client)
	{
		client->removeListener(this);
		client->disconnect(TRUE);
		ClientManager::getInstance()->putClient(client);
		client = NULL;
	}
}

void Hub::setPassword_client(string password)
{
	if (client && !password.empty())
	{
		client->setPassword(password);
		client->password(password);
	}
}

void Hub::sendMessage_client(string message, bool thirdPerson)
{
	if (client && !message.empty())
		client->hubMessage(message, thirdPerson);
}

void Hub::getFileList_client(string cid, bool match)
{
	string message;

	if (!cid.empty())
	{
		try
		{
			UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
			if (user)
			{
				if (user == ClientManager::getInstance()->getMe())
				{
					// Don't download file list, open locally instead
					WulforManager::get()->getMainWindow()->openOwnList_client(TRUE);
				}
				else if (match)
				{
					QueueManager::getInstance()->addList(user, QueueItem::FLAG_MATCH_QUEUE);
				}
				else
				{
					QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
				}
			}
			else
			{
				message = _("User not found");
			}
		}
		catch (const Exception &e)
		{
			message = e.getError();
			LogManager::getInstance()->message(message);
		}
	}

	if (!message.empty())
	{
		typedef Func2<Hub, string, Sound::TypeSound> F2;
		F2 *func = new F2(this, &Hub::addStatusMessage_gui, message, Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::grantSlot_client(string cid)
{
	string message = _("User not found");

	if (!cid.empty())
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
		{
			UploadManager::getInstance()->reserveSlot(user);
			message = _("Slot granted to ") + WulforUtil::getNicks(user);
		}
	}

	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *func = new F2(this, &Hub::addStatusMessage_gui, message, Sound::NONE);
	WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::removeUserFromQueue_client(std::string cid)
{
	if (!cid.empty())
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
			QueueManager::getInstance()->removeSource(user, QueueItem::Source::FLAG_REMOVED);
	}
}

void Hub::redirect_client(string address, bool follow)
{
	if (!address.empty())
	{
		if (ClientManager::getInstance()->isConnected(address))
		{
			string error = _("Unable to connect: already connected to the requested hub");
			typedef Func2<Hub, string, Sound::TypeSound> F2;
			F2 *f2 = new F2(this, &Hub::addStatusMessage_gui, error, Sound::NONE);
			WulforManager::get()->dispatchGuiFunc(f2);
			return;
		}

		if (follow)
		{
			// the client is dead, long live the client!
			disconnect_client();

			Func0<Hub> *func = new Func0<Hub>(this, &Hub::clearNickList_gui);
			WulforManager::get()->dispatchGuiFunc(func);

			connectClient_client(address, encoding);
		}
	}
}

void Hub::rebuildHashData_client()
{
	HashManager::getInstance()->rebuild();
}

void Hub::refreshFileList_client()
{
	try
	{
		ShareManager::getInstance()->setDirty();
		ShareManager::getInstance()->refresh(true);
	}
	catch (const ShareException& e)
	{
	}
}

void Hub::addAsFavorite_client()
{
	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *func;

	FavoriteHubEntry *existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());

	if (!existingHub)
	{
		FavoriteHubEntry aEntry;
		aEntry.setServer(client->getHubUrl());
		aEntry.setName(client->getHubName());
		aEntry.setDescription(client->getHubDescription());
		aEntry.setConnect(FALSE);
		aEntry.setNick(client->getMyNick());
		aEntry.setEncoding(encoding);
		FavoriteManager::getInstance()->addFavorite(aEntry);
		func = new F2(this, &Hub::addStatusMessage_gui, _("Favorite hub added"), Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
	else
	{
		func = new F2(this, &Hub::addStatusMessage_gui, _("Favorite hub already exists"), Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::reconnect_client()
{
	Func0<Hub> *func = new Func0<Hub>(this, &Hub::clearNickList_gui);
	WulforManager::get()->dispatchGuiFunc(func);

	if (client)
		client->reconnect();
}

void Hub::checkFavoriteUserJoin_client(string cid)
{
	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));

	if (user && FavoriteManager::getInstance()->isFavoriteUser(user))
	{
		string message = WulforUtil::getNicks(user) + _(" has joined");
		typedef Func2<Hub, std::string, Sound::TypeSound> F2a;
		F2a *func2a = new F2a(this, &Hub::addStatusMessage_gui, message, Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func2a);

		string statusMessage = WulforUtil::getNicks(user) + _(" has joined hub ") + client->getHubName();
		typedef Func2<MainWindow, string, string> F2b;
		F2b *func2b = new F2b(WulforManager::get()->getMainWindow(),
			&MainWindow::addPrivateStatusMessage_gui, cid, statusMessage);
		WulforManager::get()->dispatchGuiFunc(func2b);
	}
}

void Hub::getParams_client(ParamMap &params, Identity &id)
{
	if (id.getUser()->isSet(User::DCPLUSPLUS))
		params["Icon"] = "dc++";
	else
		params["Icon"] = "normal";

	if (id.getUser()->isSet(User::PASSIVE))
		params["Icon"] += "-fw";

	if (id.isOp())
	{
		params["Icon"] += "-op";
		params["Nick Order"] = "o" + id.getNick();
	}
	else
	{
		params["Nick Order"] = "u" + id.getNick();
	}

	params["Nick"] = id.getNick();
	params["Shared"] = Util::toString(id.getBytesShared());
	params["Description"] = id.getDescription();
	params["Tag"] = id.getTag();
	params["Connection"] = id.getConnection();
	params["eMail"] = id.getEmail();
	params["CID"] = id.getUser()->getCID().toBase32();
}

void Hub::on(ClientListener::Connecting, Client *) throw()
{
	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *f2 = new F2(this, &Hub::addStatusMessage_gui, _("Connecting to ") + client->getHubUrl() + "...", Sound::NONE);
	WulforManager::get()->dispatchGuiFunc(f2);
}

void Hub::on(ClientListener::Connected, Client *) throw()
{
	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *func = new F2(this, &Hub::addStatusMessage_gui, _("Connected"), Sound::HUB_CONNECT);
	WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::UserUpdated, Client *, const OnlineUser &user) throw()
{
	Identity id = user.getIdentity();

	if (!id.isHidden())
	{
		ParamMap params;
		getParams_client(params, id);
		Func1<Hub, ParamMap> *func = new Func1<Hub, ParamMap>(this, &Hub::updateUser_gui, params);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::on(ClientListener::UsersUpdated, Client *, const OnlineUserList &list) throw()
{
	Identity id;
	typedef Func1<Hub, ParamMap> F1;
	F1 *func;

	for (OnlineUserList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		id = (*it)->getIdentity();
		if (!id.isHidden())
		{
			ParamMap params;
			getParams_client(params, id);
			func = new F1(this, &Hub::updateUser_gui, params);
			WulforManager::get()->dispatchGuiFunc(func);
		}
	}
}

void Hub::on(ClientListener::UserRemoved, Client *, const OnlineUser &user) throw()
{
	string nick = user.getIdentity().getNick();
	string cid = user.getUser()->getCID().toBase32();
	typedef Func1<Hub, string> F1;
	F1 *func;

	if (BOOLSETTING(SHOW_JOINS) || (BOOLSETTING(FAV_SHOW_JOINS) &&
		FavoriteManager::getInstance()->isFavoriteUser(user.getUser())))
	{
		typedef Func2<Hub, string, Sound::TypeSound> F2a;
		F2a *func2a = new F2a(this, &Hub::addStatusMessage_gui, nick + _(" has quit"), Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func2a);

		string statusMessage = nick + _(" has quit hub ") + client->getHubName();
		typedef Func2<MainWindow, string, string> F2b;
		F2b *func2b = new F2b(WulforManager::get()->getMainWindow(),
			&MainWindow::addPrivateStatusMessage_gui, cid, statusMessage);
		WulforManager::get()->dispatchGuiFunc(func2b);
	}

	func = new F1(this, &Hub::removeUser_gui, cid);
	WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::Redirect, Client *, const string &address) throw()
{
	// redirect_client() crashes unless I put it into the dispatcher (why?)
	typedef Func2<Hub, string, bool> F2;
	F2 *func = new F2(this, &Hub::redirect_client, address, BOOLSETTING(AUTO_FOLLOW));
	WulforManager::get()->dispatchClientFunc(func);
}

void Hub::on(ClientListener::Failed, Client *, const string &reason) throw()
{
	Func0<Hub> *f0 = new Func0<Hub>(this, &Hub::clearNickList_gui);
	WulforManager::get()->dispatchGuiFunc(f0);

	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *f2 = new F2(this, &Hub::addStatusMessage_gui, _("Connect failed: ") + reason, Sound::HUB_DISCONNECT);
	WulforManager::get()->dispatchGuiFunc(f2);
}

void Hub::on(ClientListener::GetPassword, Client *) throw()
{
	if (!client->getPassword().empty())
		client->password(client->getPassword());
	else
	{
		Func0<Hub> *func = new Func0<Hub>(this, &Hub::getPassword_gui);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::on(ClientListener::HubUpdated, Client *) throw()
{
	typedef Func1<Hub, string> F1;
	string hubName = _("Hub: ");

	if (client->getHubName().empty())
		hubName += client->getAddress() + ":" + Util::toString(client->getPort());
	else
		hubName += client->getHubName();

	if (!client->getHubDescription().empty())
		hubName += " - " + client->getHubDescription();

	F1 *func1 = new F1(this, &BookEntry::setLabel_gui, hubName);
	WulforManager::get()->dispatchGuiFunc(func1);
}

void Hub::on(ClientListener::Message, Client *, const OnlineUser &from, const string &message, bool thirdPerson) throw()
{
	if (!message.empty())
	{
		string line;

		if (thirdPerson)
			line = "* " + from.getIdentity().getNick() + " " +  message;
		else
			line = "<" + from.getIdentity().getNick() + "> " + message;

		if (BOOLSETTING(FILTER_MESSAGES))
		{
			if ((message.find("Hub-Security") != string::npos && message.find("was kicked by") != string::npos) ||
				(message.find("is kicking") != string::npos && message.find("because:") != string::npos))
			{
				typedef Func2<Hub, string, Sound::TypeSound> F2;
				F2 *func = new F2(this, &Hub::addStatusMessage_gui, line, Sound::NONE);
				WulforManager::get()->dispatchGuiFunc(func);
				return;
			}
		}

		if (BOOLSETTING(LOG_MAIN_CHAT))
		{
			StringMap params;
			params["message"] = line;
			client->getHubIdentity().getParams(params, "hub", false);
			params["hubURL"] = client->getHubUrl();
			client->getMyIdentity().getParams(params, "my", true);
			LOG(LogManager::CHAT, params);
		}

		typedef Func1<Hub, string> F1;
		F1 *func = new F1(this, &Hub::addMessage_gui, line);
		WulforManager::get()->dispatchGuiFunc(func);

		// Set urgency hint if message contains user's nick
		if (BOOLSETTING(BOLD_HUB) && from.getIdentity().getUser() != client->getMyIdentity().getUser())
		{
			if (message.find(client->getMyIdentity().getNick()) != string::npos)
			{
				typedef Func0<Hub> F0;
				F0 *func = new F0(this, &Hub::setUrgent_gui);
				WulforManager::get()->dispatchGuiFunc(func);
			}
		}
	}
}

void Hub::on(ClientListener::StatusMessage, Client *, const string &message, int /* flag */) throw()
{
	if (!message.empty())
	{
		if (BOOLSETTING(FILTER_MESSAGES))
		{
			if ((message.find("Hub-Security") != string::npos && message.find("was kicked by") != string::npos) ||
				(message.find("is kicking") != string::npos && message.find("because:") != string::npos))
			{
				typedef Func2<Hub, string, Sound::TypeSound> F2;
				F2 *func = new F2(this, &Hub::addStatusMessage_gui, message, Sound::NONE);
				WulforManager::get()->dispatchGuiFunc(func);
				return;
			}
		}

		if (BOOLSETTING(LOG_STATUS_MESSAGES))
		{
			StringMap params;
			client->getHubIdentity().getParams(params, "hub", FALSE);
			params["hubURL"] = client->getHubUrl();
			client->getMyIdentity().getParams(params, "my", TRUE);
			params["message"] = message;
			LOG(LogManager::STATUS, params);
		}

		typedef Func1<Hub, string> F1;
		F1 *func = new F1(this, &Hub::addMessage_gui, message);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::on(ClientListener::PrivateMessage, Client *, const OnlineUser &from,
	const OnlineUser& to, const OnlineUser& replyTo, const string &msg, bool thirdPerson) throw()
{
	string error;
	const OnlineUser& user = (replyTo.getUser() == ClientManager::getInstance()->getMe()) ? to : replyTo;
	string line;

 	if (thirdPerson)
		line = "* " + from.getIdentity().getNick() + " " + msg;
	else
		line  = "<" + from.getIdentity().getNick() + "> " + msg;
	
	if (user.getIdentity().isHub() && BOOLSETTING(IGNORE_HUB_PMS))
	{
		error = _("Ignored private message from hub");
		typedef Func2<Hub, string, Sound::TypeSound> F2;
		F2 *func = new F2(this, &Hub::addStatusMessage_gui, error, Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
	else if (user.getIdentity().isBot() && BOOLSETTING(IGNORE_BOT_PMS))
	{
		error = _("Ignored private message from bot ") + user.getIdentity().getNick();
		typedef Func2<Hub, string, Sound::TypeSound> F2;
		F2 *func = new F2(this, &Hub::addStatusMessage_gui, error, Sound::NONE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
	else
	{
		typedef Func3<MainWindow, string, string, bool> F3;
		F3 *func = new F3(WulforManager::get()->getMainWindow(), &MainWindow::addPrivateMessage_gui, 
			user.getUser()->getCID().toBase32(), line, TRUE);
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void Hub::on(ClientListener::NickTaken, Client *) throw()
{
	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *func = new F2(this, &Hub::addStatusMessage_gui, _("Nick already taken"), Sound::NONE);
	WulforManager::get()->dispatchGuiFunc(func);
}

void Hub::on(ClientListener::SearchFlood, Client *, const string &msg) throw()
{
	typedef Func2<Hub, string, Sound::TypeSound> F2;
	F2 *func = new F2(this, &Hub::addStatusMessage_gui, _("Search spam detected from ") + msg, Sound::NONE);
	WulforManager::get()->dispatchGuiFunc(func);
}

