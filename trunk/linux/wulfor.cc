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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>

#include "settingsmanager.hh"
#include "wulformanager.hh"
#include "WulforUtil.hh"
#include <iostream>
#include <signal.h>

#define GUI_PACKAGE "freedcpp"
#define GUI_LOCALE_DIR _DATADIR PATH_SEPARATOR_STR "locale"

void callBack(void* x, const std::string& a)
{
	std::cout << "Loading: " << a << std::endl;
}

int main(int argc, char *argv[])
{
	// Initialize i18n support
	bindtextdomain(GUI_PACKAGE, GUI_LOCALE_DIR);
	textdomain(GUI_PACKAGE);
	bind_textdomain_codeset(GUI_PACKAGE, "UTF-8");

	// Check if profile is locked
	if (WulforUtil::profileIsLocked())
	{
		gtk_init(&argc, &argv);
		std::string message = _("Only one instance of FreeDC++ is allowed per profile");

		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message.c_str());
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		return -1;
	}

	// Start the DC++ client core
	dcpp::startup(callBack, NULL);

	dcpp::TimerManager::getInstance()->start();

	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc, &argv);
	glade_init();
	g_set_application_name("FreeDC++");

	signal(SIGPIPE, SIG_IGN);

	WulforSettingsManager::newInstance();
	WulforManager::start();
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	WulforManager::stop();
	WulforSettingsManager::deleteInstance();

	std::cout << "Shutting down..." << std::endl;
	dcpp::shutdown();

	return 0;
}

