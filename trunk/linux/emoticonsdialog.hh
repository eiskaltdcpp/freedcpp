/*
 * Copyright © 2009 freedcpp, http://code.google.com/p/freedcpp
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

#ifndef EMOTICONS_DIALOG_HH
#define EMOTICONS_DIALOG_HH

class EmoticonsDialog
{
	public:
		EmoticonsDialog(GtkWidget *chat, GtkWidget *button, GtkWidget *menu);
		~EmoticonsDialog();

		// GUI functions
		void showEmotDialog_gui();
		void showPacksMenu_gui();

	private:
		GtkWidget *Chat;       // chat entry
		GtkWidget *Button;     // chat emoticons button
		GtkWidget *Menu;       // packs menu
		GtkWidget *dialog;     // emoticons dialog
		GtkTooltips *tooltips; // tooltips for all icon widgets

		void build();
		void position();
		void graber();

		//GUI callback functions
		static void onChat(GtkWidget *widget /*button*/, gpointer data /*this*/);
		static void onCheckItemMenu(GtkMenuItem *checkItem, gpointer data);
		static gboolean event(GtkWidget *widget /*dialog*/, GdkEvent *event, gpointer data /*this*/);
};

#else
class EmoticonsDialog;
#endif
