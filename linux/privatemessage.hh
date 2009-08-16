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

#ifndef WULFOR_PRIVATE_MESSAGE_HH
#define WULFOR_PRIVATE_MESSAGE_HH

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include "bookentry.hh"

class PrivateMessage:
	public BookEntry
{
	public:
		PrivateMessage(const std::string &cid);
		virtual ~PrivateMessage();
		virtual void show();

		// GUI functions
		void addMessage_gui(std::string message);
		void addStatusMessage_gui(std::string message);

	private:
		// GUI functions
		void addLine_gui(const std::string &line);
		void updateCursor(GtkWidget *widget);

		// GUI callbacks
		static gboolean onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data);
		static gboolean onKeyPress_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static gboolean onLinkTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onHubTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onMagnetTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onChatPointerMoved_gui(GtkWidget *widget, GdkEventMotion *event, gpointer data);
		static gboolean onChatVisibilityChanged_gui(GtkWidget* widget, GdkEventVisibility* event, gpointer data);
		static void onChatScroll_gui(GtkAdjustment *adjustment, gpointer data);
		static void onChatResize_gui(GtkAdjustment *adjustment, gpointer data);
		static void onSendMessage_gui(GtkEntry *entry, gpointer data);
		static void onCopyURIClicked_gui(GtkMenuItem *item, gpointer data);
		static void onOpenLinkClicked_gui(GtkMenuItem *item, gpointer data);
		static void onOpenHubClicked_gui(GtkMenuItem *item, gpointer data);
		static void onSearchMagnetClicked_gui(GtkMenuItem *item, gpointer data);
		static void onMagnetPropertiesClicked_gui(GtkMenuItem *item, gpointer data);

		// Client functions
		void sendMessage_client(std::string message);
		void addFavoriteUser_client();
		void getFileList_client();
		void grantSlot_client();

		GtkTextBuffer *buffer;
		GtkTextMark *mark;
		std::string cid;
		bool isBot;
		std::vector<std::string> history;
		int historyIndex;
		bool sentAwayMessage;
		static const int maxLines = 500; ///@todo: make these preferences
		static const int maxHistory = 20;
		GdkCursor* handCursor;
		bool aboveURI;
		std::string selectedURI;
		bool scrollToBottom;
};

#else
class PrivateMessage;
#endif
