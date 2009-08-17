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

#include "privatemessage.hh"

#include <dcpp/ClientManager.h>
#include <dcpp/FavoriteManager.h>
#include "settingsmanager.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"
#include "search.hh"
#include "sound.hh"

using namespace std;
using namespace dcpp;

PrivateMessage::PrivateMessage(const string &cid):
	BookEntry(Entry::PRIVATE_MESSAGE, _("PM: ") + WulforUtil::getNicks(cid), "privatemessage.glade", cid),
	cid(cid),
	historyIndex(0),
	sentAwayMessage(FALSE),
	scrollToBottom(TRUE)
{
	// Intialize the chat window
	if (SETTING(USE_OEM_MONOFONT))
	{
		PangoFontDescription *fontDesc = pango_font_description_new();
		pango_font_description_set_family(fontDesc, "Mono");
		gtk_widget_modify_font(getWidget("text"), fontDesc);
		pango_font_description_free(fontDesc);
	}
	GtkTextIter iter;
	buffer = gtk_text_buffer_new(NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(getWidget("text")), buffer);
	gtk_text_buffer_get_end_iter(buffer, &iter);
	mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
	handCursor = gdk_cursor_new(GDK_HAND2);

	/* FIXME: BOOLSETTING(PRIVATE_MESSGE_BEEP_OPEN) has been changed to
	 * FIXME: STRINGSETTING(SOUND_PM_WINDOW)
	 
	if (BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN))
		gdk_beep();
	*/

	GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(getWidget("scroll")));

	// Connect the signals to their callback functions.
	g_signal_connect(getContainer(), "focus-in-event", G_CALLBACK(onFocusIn_gui), (gpointer)this);
	g_signal_connect(getWidget("entry"), "activate", G_CALLBACK(onSendMessage_gui), (gpointer)this);
	g_signal_connect(getWidget("entry"), "key-press-event", G_CALLBACK(onKeyPress_gui), (gpointer)this);
	g_signal_connect(getWidget("text"), "motion-notify-event", G_CALLBACK(onChatPointerMoved_gui), (gpointer)this);
	g_signal_connect(getWidget("text"), "visibility-notify-event", G_CALLBACK(onChatVisibilityChanged_gui), (gpointer)this);
	g_signal_connect(adjustment, "value_changed", G_CALLBACK(onChatScroll_gui), (gpointer)this);
	g_signal_connect(adjustment, "changed", G_CALLBACK(onChatResize_gui), (gpointer)this);
	g_signal_connect(getWidget("copyLinkItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openLinkItem"), "activate", G_CALLBACK(onOpenLinkClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyhubItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("openhubItem"), "activate", G_CALLBACK(onOpenHubClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("copyMagnetItem"), "activate", G_CALLBACK(onCopyURIClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("searchMagnetItem"), "activate", G_CALLBACK(onSearchMagnetClicked_gui), (gpointer)this);
	g_signal_connect(getWidget("magnetPropertiesItem"), "activate", G_CALLBACK(onMagnetPropertiesClicked_gui), (gpointer)this);

	gtk_widget_grab_focus(getWidget("entry"));
	history.push_back("");
	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	isBot = user ? user->isSet(User::BOT) : FALSE;

	setLabel_gui(_("PM: ") + WulforUtil::getNicks(cid) + " [" + WulforUtil::getHubNames(cid) + "]");
}

PrivateMessage::~PrivateMessage()
{
	if (handCursor)
	{
		gdk_cursor_unref(handCursor);
		handCursor = NULL;
	}
}

void PrivateMessage::show()
{
}

void PrivateMessage::addMessage_gui(string message)
{
	addLine_gui(message);

	if (BOOLSETTING(LOG_PRIVATE_CHAT))
	{
		StringMap params;
		params["message"] = message;
		params["hubNI"] = WulforUtil::getHubNames(cid);
		params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(CID(cid)));
		params["userCID"] = cid;
		params["userNI"] = ClientManager::getInstance()->getNicks(CID(cid))[0];
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
		LOG(LogManager::PM, params);
	}

	if (BOOLSETTING(BOLD_PM))
		setUrgent_gui();

	/*if (BOOLSETTING(PRIVATE_MESSAGE_BEEP) && !isActive_gui())
		gdk_beep();
	*/

	// Send an away message, but only the first time after setting away mode.
	if (!Util::getAway())
	{
		sentAwayMessage = FALSE;
	}
	else if (!sentAwayMessage && !(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && isBot))
	{
		sentAwayMessage = TRUE;
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(this, &PrivateMessage::sendMessage_client, Util::getAwayMessage());
		WulforManager::get()->dispatchClientFunc(func);
	}

	/* FIXME: BOOLSETTING(PRIVATE_MESSAGE_BEEP) has been changed to
	 * FIXME: STRINGSETTING(SOUND_PM) 
	if (BOOLSETTING(PRIVATE_MESSAGE_BEEP))
	{
		MainWindow *mw = WulforManager::get()->getMainWindow();
		GdkWindowState state = gdk_window_get_state(mw->getContainer()->window);
	 	if ((state & GDK_WINDOW_STATE_ICONIFIED) || mw->currentPage_gui() != getContainer())
			gdk_beep();
	}
	*/

	if (WGETB("sound-pm"))
	{
		MainWindow *mw = WulforManager::get()->getMainWindow();
		GdkWindowState state = gdk_window_get_state(mw->getContainer()->window);

		if ((state & GDK_WINDOW_STATE_ICONIFIED) || mw->currentPage_gui() != getContainer())
			Sound::get()->playSound(Sound::PRIVATE_MESSAGE);
		else if (WGETB("sound-pm-open")) Sound::get()->playSound(Sound::PRIVATE_MESSAGE);
	}
}

void PrivateMessage::addStatusMessage_gui(string message)
{
	addLine_gui("*** " + message);
}

void PrivateMessage::addLine_gui(const string &message)
{
	GtkTextIter iter;
	string line = "";
	string::size_type start, end = 0;
	GtkTextIter i_start, i_end;

	// Add a new line if this isn't the first line in buffer.
	if (gtk_text_buffer_get_char_count(buffer) > 0)
		line = "\n";

	if (BOOLSETTING(TIME_STAMPS))
		line += "[" + Util::getShortTimeString() + "] ";

	line += message;

	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, line.c_str(), line.size());

	// check for URIs in line
	gtk_text_buffer_get_end_iter(buffer, &iter);

	while ((start = line.find_first_not_of(" \n\r\t", end)) != string::npos)
	{
		end = line.find_first_of(" \n\r\t", start);
		if (end == string::npos)
			end = line.size();

		string uri = line.substr(start, end - start);
		GCallback callback = NULL;

		if (WulforUtil::isLink(uri))
			callback = G_CALLBACK(onLinkTagEvent_gui);
		else if (WulforUtil::isHubURL(uri))
			callback = G_CALLBACK(onHubTagEvent_gui);
		else if (WulforUtil::isMagnet(uri))
			callback = G_CALLBACK(onMagnetTagEvent_gui);

		if (callback)
		{
			// check for the URI in our buffer
			GtkTextTag *tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buffer), uri.c_str());

			if (!tag)
			{
				tag = gtk_text_buffer_create_tag(buffer, uri.c_str(), "foreground", "blue", "underline", PANGO_UNDERLINE_SINGLE, NULL);
				g_signal_connect(tag, "event", callback, (gpointer)this);
			}

			i_start = i_end = iter;
			gtk_text_iter_backward_chars(&i_start, g_utf8_strlen(line.c_str() + start, -1));
			gtk_text_iter_backward_chars(&i_end, g_utf8_strlen(line.c_str() + end, -1));
			gtk_text_buffer_apply_tag(buffer, tag, &i_start, &i_end);
		}
	}

	if (gtk_text_buffer_get_line_count(buffer) > maxLines)
	{
		GtkTextIter next;
		gtk_text_buffer_get_start_iter(buffer, &iter);
		gtk_text_buffer_get_iter_at_line(buffer, &next, 1);
		gtk_text_buffer_delete(buffer, &iter, &next);
	}
}

void PrivateMessage::updateCursor(GtkWidget *widget)
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
			selectedURI = newTag->name;
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


gboolean PrivateMessage::onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	gtk_widget_grab_focus(pm->getWidget("entry"));

	return TRUE;
}

void PrivateMessage::onSendMessage_gui(GtkEntry *entry, gpointer data)
{
	string text = gtk_entry_get_text(entry);
	if (text.empty())
		return;

	PrivateMessage *pm = (PrivateMessage *)data;
	gtk_entry_set_text(entry, "");

	// Store line in chat history
	pm->history.pop_back();
	pm->history.push_back(text);
	pm->history.push_back("");
	pm->historyIndex = pm->history.size() - 1;
	if (pm->history.size() > maxHistory + 1)
		pm->history.erase(pm->history.begin());

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
				pm->addStatusMessage_gui(_("Away mode off"));
				pm->sentAwayMessage = FALSE;
			}
			else
			{
				Util::setAway(TRUE);
				Util::setManualAway(TRUE);
				Util::setAwayMessage(param);
				pm->addStatusMessage_gui(_("Away mode on: ") + Util::getAwayMessage());
			}
		}
		else if (command == _("back"))
		{
			Util::setAway(FALSE);
			pm->addStatusMessage_gui(_("Away mode off"));
		}
		else if (command == _("clear"))
		{
			GtkTextIter startIter, endIter;
			gtk_text_buffer_get_start_iter(pm->buffer, &startIter);
			gtk_text_buffer_get_end_iter(pm->buffer, &endIter);
			gtk_text_buffer_delete(pm->buffer, &startIter, &endIter);
		}
		else if (command == _("close"))
		{
			WulforManager::get()->getMainWindow()->removeBookEntry_gui(pm);
		}
		else if (command == _("favorite") || text == _("fav"))
		{
			typedef Func0<PrivateMessage> F0;
			F0 *func = new F0(pm, &PrivateMessage::addFavoriteUser_client);
			WulforManager::get()->dispatchClientFunc(func);
		}
		else if (command == _("getlist"))
		{
			typedef Func0<PrivateMessage> F0;
			F0 *func = new F0(pm, &PrivateMessage::getFileList_client);
			WulforManager::get()->dispatchClientFunc(func);
		}
		else if (command == _("grant"))
		{
			typedef Func0<PrivateMessage> F0;
			F0 *func = new F0(pm, &PrivateMessage::grantSlot_client);
			WulforManager::get()->dispatchClientFunc(func);
		}
		else if (command == _("help"))
		{
			pm->addStatusMessage_gui(_("Available commands: /away <message>, /back, /clear, /close, /favorite, /getlist, /grant, /help"));
		}
		else
		{
			pm->addStatusMessage_gui(_("Unknown command ") + text + _(": type /help for a list of available commands"));
		}
	}
	else
	{
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(pm, &PrivateMessage::sendMessage_client, text);
		WulforManager::get()->dispatchClientFunc(func);
	}
}

gboolean PrivateMessage::onKeyPress_gui(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;
	string text;
	size_t index;

	if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up)
	{
		index = pm->historyIndex - 1;
		if (index >= 0 && index < pm->history.size())
		{
			text = pm->history[index];
			pm->historyIndex = index;
			gtk_entry_set_text(GTK_ENTRY(widget), text.c_str());
		}
		return TRUE;
	}
	else if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
	{
		index = pm->historyIndex + 1;
		if (index >= 0 && index < pm->history.size())
		{
			text = pm->history[index];
			pm->historyIndex = index;
			gtk_entry_set_text(GTK_ENTRY(widget), text.c_str());
		}
		return TRUE;
	}

	return FALSE;
}

gboolean PrivateMessage::onLinkTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		pm->selectedURI = tag->name;

		switch (event->button.button)
		{
			case 1:
				onOpenLinkClicked_gui(NULL, data);
				break;
			case 3:
				// Pop-up link context menu
				gtk_widget_show_all(pm->getWidget("linkMenu"));
				gtk_menu_popup(GTK_MENU(pm->getWidget("linkMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean PrivateMessage::onHubTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		pm->selectedURI = tag->name;

		switch (event->button.button)
		{
			case 1:
				onOpenHubClicked_gui(NULL, data);
				break;
			case 3:
				// Popup hub context menu
				gtk_widget_show_all(pm->getWidget("hubMenu"));
				gtk_menu_popup(GTK_MENU(pm->getWidget("hubMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean PrivateMessage::onMagnetTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	if (event->type == GDK_BUTTON_PRESS)
	{
		pm->selectedURI = tag->name;

		switch (event->button.button)
		{
			case 1:
				// Search for magnet
				onSearchMagnetClicked_gui(NULL, data);
				break;
			case 3:
				// Popup magnet context menu
				gtk_widget_show_all(pm->getWidget("magnetMenu"));
				gtk_menu_popup(GTK_MENU(pm->getWidget("magnetMenu")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
				break;
		}
		return TRUE;
	}
	return FALSE;
}

gboolean PrivateMessage::onChatPointerMoved_gui(GtkWidget* widget, GdkEventMotion* event, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	pm->updateCursor(widget);

	return FALSE;
}

gboolean PrivateMessage::onChatVisibilityChanged_gui(GtkWidget* widget, GdkEventVisibility* event, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	pm->updateCursor(widget);

	return FALSE;
}

void PrivateMessage::onChatScroll_gui(GtkAdjustment *adjustment, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;
	gdouble value = gtk_adjustment_get_value(adjustment);
	pm->scrollToBottom = value >= (adjustment->upper - adjustment->page_size);
}

void PrivateMessage::onChatResize_gui(GtkAdjustment *adjustment, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;
	gdouble value = gtk_adjustment_get_value(adjustment);

	if (pm->scrollToBottom && value < (adjustment->upper - adjustment->page_size))
	{
		GtkTextIter iter;

		gtk_text_buffer_get_end_iter(pm->buffer, &iter);
		gtk_text_buffer_move_mark(pm->buffer, pm->mark, &iter);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(pm->getWidget("text")), pm->mark, 0, FALSE, 0, 0);
	}
}

void PrivateMessage::onCopyURIClicked_gui(GtkMenuItem *item, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), pm->selectedURI.c_str(), pm->selectedURI.length());
}

void PrivateMessage::onOpenLinkClicked_gui(GtkMenuItem *item, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	WulforUtil::openURI(pm->selectedURI);
}

void PrivateMessage::onOpenHubClicked_gui(GtkMenuItem *item, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	WulforManager::get()->getMainWindow()->showHub_gui(pm->selectedURI);
}

void PrivateMessage::onSearchMagnetClicked_gui(GtkMenuItem *item, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;
	string name;
	int64_t size;
	string tth;

	if (WulforUtil::splitMagnet(pm->selectedURI, name, size, tth))
	{
		Search *s = WulforManager::get()->getMainWindow()->addSearch_gui();
		s->putValue_gui(tth, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
	}
}

void PrivateMessage::onMagnetPropertiesClicked_gui(GtkMenuItem *item, gpointer data)
{
	PrivateMessage *pm = (PrivateMessage *)data;

	WulforManager::get()->getMainWindow()->openMagnetDialog_gui(pm->selectedURI);
}

void PrivateMessage::sendMessage_client(std::string message)
{
	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	if (user && user->isOnline())
	{
		// FIXME: WTF does the 3rd param (bool thirdPerson) do? A: Used for /me stuff
		ClientManager::getInstance()->privateMessage(user, message, false);
	}
	else
	{
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(this, &PrivateMessage::addStatusMessage_gui, _("User went offline"));
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void PrivateMessage::addFavoriteUser_client()
{
	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	if (user)
	{
		FavoriteManager::getInstance()->addFavoriteUser(user);
	}
	else
	{
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(this, &PrivateMessage::addStatusMessage_gui, _("Added user to favorites list"));
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void PrivateMessage::getFileList_client()
{
	try
	{
		UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
		if (user)
			QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW);
	}
	catch (const Exception& e)
	{
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(this, &PrivateMessage::addStatusMessage_gui, e.getError());
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

void PrivateMessage::grantSlot_client()
{
	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	if (user)
	{
		UploadManager::getInstance()->reserveSlot(user);
	}
	else
	{
		typedef Func1<PrivateMessage, string> F1;
		F1 *func = new F1(this, &PrivateMessage::addStatusMessage_gui, _("Slot granted"));
		WulforManager::get()->dispatchGuiFunc(func);
	}
}

