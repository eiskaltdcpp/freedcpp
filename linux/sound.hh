/*
 * Copyright © 2009 freedcpp
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

#ifndef SOUND_HH
#define SOUND_HH

class Sound
{
	public:
		static Sound *pSound;

		enum TypeSound
		{
			FIRST = 0,
			/*-*/
			DOWNLOAD_BEGINS = FIRST,
			DOWNLOAD_FINISHED,
			UPLOAD_FINISHED,
			PRIVATE_MESSAGE,
			HUB_CONNECT,
			HUB_DISCONNECT,
			/*-*/
			LAST,

			NONE
		};

		static Sound* get();
		static void start();
		static void stop();

		Sound() { sound_init(); }
		~Sound() { sound_finalize(); }

		void playSound(TypeSound sound);
		void playSound(const std::string &target);

	private:
		void sound_init();
		void sound_finalize();
};

#else
class Sound;
#endif
