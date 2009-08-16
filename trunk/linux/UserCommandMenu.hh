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

#ifndef USER_COMMAND_MENU_HH
#define USER_COMMAND_MENU_HH

#include <gtk/gtk.h>
#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include "entry.hh"

using namespace dcpp;

class UserCommandMenu : public Entry
{
	public:
		UserCommandMenu(GtkWidget *userCommandMenu, int ctx);
		virtual ~UserCommandMenu() {}

		GtkWidget *getContainer() { return userCommandMenu; }
		void addHub(const std::string &hub);
		void addHub(const StringList &hubs2);
		void addUser(const std::string &cid);
		void cleanMenu_gui();
		void buildMenu_gui();

	private:
		// GUI functions
		void createSubMenu_gui(GtkWidget *&menu, std::string &command);

		// GUI callbacks
		static void onUserCommandClick_gui(GtkMenuItem *item, gpointer data);

		// Client functions
		void sendUserCommand_client(std::string cid, string commandName, string hub, StringMap params);

		GtkWidget *userCommandMenu;
		int ctx;
		StringList hubs;
		StringList users;
};

#else
class UserCommandMenu;
#endif

