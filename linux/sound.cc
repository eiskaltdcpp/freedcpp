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

#include <libgnome/gnome-sound.h>
#include "settingsmanager.hh"

#include "sound.hh"

using namespace std;

Sound *Sound::pSound = NULL;

void Sound::start()
{
	dcassert(!pSound);
	pSound = new Sound();
}

void Sound::stop()
{
	dcassert(pSound);
	delete pSound;
	pSound = NULL;
}

Sound* Sound::get()
{
	dcassert(pSound);
	return pSound;
}

void Sound::sound_init()
{
	gnome_sound_init(NULL);
	dcdebug("Sound::sound_init: Esound connection %d...\n", gnome_sound_connection_get());
}

void Sound::playSound(TypeSound sound)
{
	switch (sound)
	{
		case DOWNLOAD_BEGINS:
			playSound(WGETS("sound-download-begins")); break;
		case DOWNLOAD_FINISHED:
			playSound(WGETS("sound-download-finished")); break;
		case UPLOAD_FINISHED:
			playSound(WGETS("sound-upload-finished")); break;
		case PRIVATE_MESSAGE:
			playSound(WGETS("sound-private-message")); break;
		case HUB_CONNECT:
			playSound(WGETS("sound-hub-connect")); break;
		case HUB_DISCONNECT:
			playSound(WGETS("sound-hub-disconnect")); break;

		default: break;
	}
}

void Sound::playSound(const string &target)
{
	gnome_sound_play(target.c_str());
}

void Sound::sound_finalize()
{
	gnome_sound_shutdown();
}
