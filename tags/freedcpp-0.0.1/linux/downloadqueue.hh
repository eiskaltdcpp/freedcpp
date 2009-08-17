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

#ifndef WULFOR_DOWNLOAD_QUEUE_HH
#define WULFOR_DOWNLOAD_QUEUE_HH

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include <dcpp/QueueManager.h>

#include "bookentry.hh"
#include "treeview.hh"

using namespace dcpp;

class DownloadQueue:
	public BookEntry,
	public QueueManagerListener
{
	public:
		DownloadQueue();
		virtual ~DownloadQueue();
		virtual void show();

	private:
		// GUI functions
		void buildDynamicMenu_gui();
		void setStatus_gui(std::string text, std::string statusItem);
		void updateStatus_gui();
		void addFiles_gui(vector<StringMap> files, bool firstUpdate);
		void addFile_gui(StringMap params, bool updateDirs);
		void addDir_gui(const std::string &path, GtkTreeIter *parent);
		void updateFile_gui(StringMap params);
		void removeFile_gui(std::string path, int64_t size);
		void removeDir_gui(const std::string &path, GtkTreeIter *parent);
		void updateFileView_gui();
		void sendMessage_gui(std::string cid);

		// GUI callbacks
		static gboolean onDirButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onDirButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onDirKeyReleased_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static gboolean onFileButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onFileButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static gboolean onFileKeyReleased_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static void onDirPriorityClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDirMoveClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDirRemoveClicked_gui(GtkMenuItem *menuitem, gpointer data);
		static void onFileSearchAlternatesClicked_gui(GtkMenuItem *item, gpointer data);
		static void onCopyMagnetClicked_gui(GtkMenuItem* item, gpointer data);
		static void onFileMoveClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFilePriorityClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileGetListClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileSendPMClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileReAddSourceClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileRemoveSourceClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileRemoveUserFromQueueClicked_gui(GtkMenuItem *item, gpointer data);
		static void onFileRemoveClicked_gui(GtkMenuItem *menuitem, gpointer data);

		// Client functions
		void buildList_client();
		void move_client(std::string source, std::string target);
		void moveDir_client(std::string source, std::string target);
		void setPriority_client(std::string target, QueueItem::Priority p);
		void setPriorityDir_client(std::string path, QueueItem::Priority p);
		void addList_client(std::string target, std::string nick);
		void sendMessage_client(std::string target, std::string nick);
		void reAddSource_client(std::string target, std::string nick);
		void removeSource_client(std::string target, std::string nick);
		void removeSources_client(std::string target, std::string nick);
		void remove_client(std::string target);
		void removeDir_client(std::string path);
		void updateFileView_client(std::string path);
		void getQueueParams_client(QueueItem *item, StringMap &params);

		// Client callbacks
		virtual void on(QueueManagerListener::Added, QueueItem *item) throw();
		virtual void on(QueueManagerListener::Moved, QueueItem *item, const std::string &oldTarget) throw();
		virtual void on(QueueManagerListener::Removed, QueueItem *item) throw();
		virtual void on(QueueManagerListener::SourcesUpdated, QueueItem *item) throw();
		virtual void on(QueueManagerListener::StatusUpdated, QueueItem *item) throw();

		// Private variables
		TreeView dirView;
		TreeView fileView;
		GtkTreeStore *dirStore;
		GtkListStore *fileStore;
		GtkTreeSelection *dirSelection;
		GtkTreeSelection *fileSelection;
		GdkEventType dirPrevious;
		std::string currentDir;
		unordered_map<std::string, std::map<std::string, std::string> > sources;
		unordered_map<std::string, std::map<std::string, std::string> > badSources;
		int currentItems;
		int totalItems;
		int64_t currentSize;
		int64_t totalSize;

		typedef map<std::string, std::string>::const_iterator SourceIter;
};

#else
class DownloadQueue;
#endif
