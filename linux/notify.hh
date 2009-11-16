/*
 * Copyright © 2009 Leliksan Floyd <leliksan@Quadrafon2>
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

/* patch changelog:
 * [05.11.09] модификация исходного кода, убрал лишний код.
 * [05.11.09] масштабирование изображения иконки.
 * [06.11.09] исправлена ошибка, "забивание" экрана уведомлениями, когда период сообщений меньше периода уведомления.
 * [07.11.09] установка уровня уведомления в зависимости от типа сообщения (critical-ошибки, normal-все остальные).
 * [08.11.09] исправлена ошибка, после выхода не закрывалось уведомление.
 * [08.11.09] исправлена ошибка, не обновлялась иконка.
 *
 * copyright author patch: troll, freedcpp, http://code.google.com/p/freedcpp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef NOTIFY_HH
#define NOTIFY_HH

#include <libnotify/notify.h>

class Notify
{
	public:
		enum TypeNotify
		{
			DOWNLOAD_FINISHED,
			PRIVATE_MESSAGE,
			HUB_CONNECT,
			HUB_DISCONNECT,
			NONE
		};

		static Notify* get();
		static void start();
		static void stop();

		Notify() {init();}
		~Notify() {finalize();}

		void showNotify(const std::string &head, const std::string &body, TypeNotify notify);
		void showNotify(const std::string &title, const std::string &head, const std::string &body,
			const std::string &icon, NotifyUrgency urgency);

	private:
		static Notify *notify;
		enum {x16, x22, x24, x32, x36, x48, x64, DEFAULT};

		void init();
		void finalize();
		void setCurrIconSize(const int size);

		int icon_width;
		int icon_height;
		int currIconSize;
		NotifyNotification *notification;
};

#else
class Notify;
#endif
