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

#include "bookentry.hh"
#include "wulformanager.hh"

using namespace std;

GSList* BookEntry::group = NULL;

BookEntry::BookEntry(const EntryType type, const string &text, const string &glade, const string &id):
	Entry(type, glade, id),
	bold(FALSE),
	urgent(FALSE)
{
	labelBox = gtk_hbox_new(FALSE, 5);

	eventBox = gtk_event_box_new();
	gtk_event_box_set_above_child(GTK_EVENT_BOX(eventBox), TRUE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventBox), FALSE);

	// Make the eventbox fill to all left-over space.
	gtk_box_pack_start(GTK_BOX(labelBox), GTK_WIDGET(eventBox), TRUE, TRUE, 0);

	label = GTK_LABEL(gtk_label_new(text.c_str()));
	gtk_container_add(GTK_CONTAINER(eventBox), GTK_WIDGET(label));

	// Align text to the left (x = 0) and in the vertical center (0.5)
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	closeButton = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(closeButton), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(closeButton), FALSE);

	// Shrink the padding around the close button
	GtkRcStyle *rcstyle = gtk_rc_style_new();
	rcstyle->xthickness = rcstyle->ythickness = 0;
	gtk_widget_modify_style(closeButton, rcstyle);
	gtk_rc_style_unref(rcstyle);

	// Add the stock icon to the close button
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(closeButton), image);
	gtk_box_pack_start(GTK_BOX(labelBox), closeButton, FALSE, FALSE, 0);

	tips = gtk_tooltips_new();
	gtk_tooltips_enable(tips);
	gtk_tooltips_set_tip(tips, closeButton, _("Close tab"), NULL);

	gtk_widget_show_all(labelBox);

	tabMenuItem = gtk_radio_menu_item_new_with_label(group, text.c_str());
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(tabMenuItem));

	setLabel_gui(text);

	// Associates entry to the widget for later retrieval in MainWindow::switchPage_gui()
	g_object_set_data(G_OBJECT(getContainer()), "entry", (gpointer)this);
}

GtkWidget* BookEntry::getContainer()
{
	return getWidget("mainBox");
}

void BookEntry::setLabel_gui(string text)
{
	// Update the tab menu item label
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(tabMenuItem));
	if (child && GTK_IS_LABEL(child))
		gtk_label_set_text(GTK_LABEL(child), text.c_str());

	// Update the notebook tab label
	gtk_tooltips_set_tip(tips, eventBox, text.c_str(), text.c_str());
	glong len = g_utf8_strlen(text.c_str(), -1);

	// Truncate the label text
	if (len > labelSize)
	{
		gchar truncatedText[text.size()];
		const string clipText = _("...");
		len = labelSize - g_utf8_strlen(clipText.c_str(), -1);
		g_utf8_strncpy(truncatedText, text.c_str(), len);
		truncatedLabelText = truncatedText + clipText;
	}
	else
	{
		truncatedLabelText = text;
	}

	labelText = text;
	updateLabel_gui();

	// Update the main window title if the current tab is selected.
	if (isActive_gui())
		WulforManager::get()->getMainWindow()->setTitle(getLabelText());
}

void BookEntry::setBold_gui()
{
	if (!bold && !isActive_gui())
	{
		bold = TRUE;
		updateLabel_gui();
	}
}

void BookEntry::setUrgent_gui()
{
	if (!isActive_gui())
	{
		MainWindow *mw = WulforManager::get()->getMainWindow();

		if (!urgent)
		{
			bold = TRUE;
			urgent = TRUE;
			updateLabel_gui();
		}

		if (!mw->isActive_gui())
			mw->setUrgent_gui();
	}
}

void BookEntry::setActive_gui()
{
	if (bold || urgent)
	{
		bold = FALSE;
		urgent = FALSE;
		updateLabel_gui();
	}
}

bool BookEntry::isActive_gui()
{
	MainWindow *mw = WulforManager::get()->getMainWindow();

	return mw->isActive_gui() && mw->currentPage_gui() == getContainer();
}

void BookEntry::updateLabel_gui()
{
	const char *format = "%s";

	if (urgent)
		format = "<i><b>%s</b></i>";
	else if (bold)
		format = "<b>%s</b>";

	char *markup = g_markup_printf_escaped(format, truncatedLabelText.c_str());
	gtk_label_set_markup(label, markup);
	g_free(markup);
}

const string& BookEntry::getLabelText()
{
	return labelText;
}

