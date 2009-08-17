/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "HashManager.h"
#include "SimpleXML.h"
#include "LogManager.h"
#include "File.h"
#include "ZUtils.h"
#include "SFVReader.h"

#ifndef _WIN32
#include <sys/mman.h> // mmap, munmap, madvise
#endif

namespace dcpp {

#define HASH_FILE_VERSION_STRING "2"
static const uint32_t HASH_FILE_VERSION = 2;
const int64_t HashManager::MIN_BLOCK_SIZE = 64 * 1024;

bool HashManager::checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) {
	Lock l(cs);
	if (!store.checkTTH(aFileName, aSize, aTimeStamp)) {
		hasher.hashFile(aFileName, aSize);
		return false;
	}
	return true;
}

TTHValue HashManager::getTTH(const string& aFileName, int64_t aSize) throw(HashException) {
	Lock l(cs);
	const TTHValue* tth = store.getTTH(aFileName);
	if (tth == NULL) {
		hasher.hashFile(aFileName, aSize);
		throw HashException(Util::emptyString);
	}
	return *tth;
}

bool HashManager::getTree(const TTHValue& root, TigerTree& tt) {
	Lock l(cs);
	return store.getTree(root, tt);
}

size_t HashManager::getBlockSize(const TTHValue& root) {
	Lock l(cs);
	return store.getBlockSize(root);
}

void HashManager::hashDone(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, int64_t speed) {
	try {
		Lock l(cs);
		store.addFile(aFileName, aTimeStamp, tth, true);
	} catch (const Exception& e) {
		LogManager::getInstance()->message(str(F_("Hashing failed: %1%") % e.getError()));
		return;
	}

	fire(HashManagerListener::TTHDone(), aFileName, tth.getRoot());

	string fn = aFileName;
	if (count(fn.begin(), fn.end(), PATH_SEPARATOR) >= 2) {
		string::size_type i = fn.rfind(PATH_SEPARATOR);
		i = fn.rfind(PATH_SEPARATOR, i - 1);
		fn.erase(0, i);
		fn.insert(0, "...");
	}
	if (speed > 0) {
		LogManager::getInstance()->message(str(F_("Finished hashing: %1% (%2%/s)") % fn % Util::formatBytes(speed)));
	} else {
		LogManager::getInstance()->message(str(F_("Finished hashing: %1%") % fn));
	}
}

void HashManager::HashStore::addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed) {
	addTree(tth);

	string fname = Text::toLower(Util::getFileName(aFileName));
	string fpath = Text::toLower(Util::getFilePath(aFileName));

	FileInfoList& fileList = fileIndex[fpath];

	FileInfoIter j = find(fileList.begin(), fileList.end(), fname);
	if (j != fileList.end()) {
		fileList.erase(j);
	}

	fileList.push_back(FileInfo(fname, tth.getRoot(), aTimeStamp, aUsed));
	dirty = true;
}

void HashManager::HashStore::addTree(const TigerTree& tt) throw() {
	if (treeIndex.find(tt.getRoot()) == treeIndex.end()) {
		try {
			File f(getDataFile(), File::READ | File::WRITE, File::OPEN);
			int64_t index = saveTree(f, tt);
			treeIndex.insert(make_pair(tt.getRoot(), TreeInfo(tt.getFileSize(), index, tt.getBlockSize())));
			dirty = true;
		} catch (const FileException& e) {
			LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
		}
	}
}

int64_t HashManager::HashStore::saveTree(File& f, const TigerTree& tt) throw(FileException) {
	if (tt.getLeaves().size() == 1)
		return SMALL_TREE;

	f.setPos(0);
	int64_t pos = 0;
	size_t n = sizeof(pos);
	if (f.read(&pos, n) != sizeof(pos))
		throw HashException(_("Unable to read hash data file"));

	// Check if we should grow the file, we grow by a meg at a time...
	int64_t datsz = f.getSize();
	if ((pos + (int64_t) (tt.getLeaves().size() * TTHValue::BYTES)) >= datsz) {
		f.setPos(datsz + 1024 * 1024);
		f.setEOF();
	}
	f.setPos(pos);dcassert(tt.getLeaves().size()> 1);
	f.write(tt.getLeaves()[0].data, (tt.getLeaves().size() * TTHValue::BYTES));
	int64_t p2 = f.getPos();
	f.setPos(0);
	f.write(&p2, sizeof(p2));
	return pos;
}

bool HashManager::HashStore::loadTree(File& f, const TreeInfo& ti, const TTHValue& root, TigerTree& tt) {
	if (ti.getIndex() == SMALL_TREE) {
		tt = TigerTree(ti.getSize(), ti.getBlockSize(), root);
		return true;
	}
	try {
		f.setPos(ti.getIndex());
		size_t datalen = TigerTree::calcBlocks(ti.getSize(), ti.getBlockSize()) * TTHValue::BYTES;
		boost::scoped_array<uint8_t> buf(new uint8_t[datalen]);
		f.read(&buf[0], datalen);
		tt = TigerTree(ti.getSize(), ti.getBlockSize(), &buf[0]);
		if (!(tt.getRoot() == root))
			return false;
	} catch (const Exception&) {
		return false;
	}

	return true;
}

bool HashManager::HashStore::getTree(const TTHValue& root, TigerTree& tt) {
	TreeIter i = treeIndex.find(root);
	if (i == treeIndex.end())
		return false;
	try {
		File f(getDataFile(), File::READ, File::OPEN);
		return loadTree(f, i->second, root, tt);
	} catch (const Exception&) {
		return false;
	}
}

size_t HashManager::HashStore::getBlockSize(const TTHValue& root) const {
	TreeMap::const_iterator i = treeIndex.find(root);
	return i == treeIndex.end() ? 0 : i->second.getBlockSize();
}

bool HashManager::HashStore::checkTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) {
	string fname = Text::toLower(Util::getFileName(aFileName));
	string fpath = Text::toLower(Util::getFilePath(aFileName));
	DirIter i = fileIndex.find(fpath);
	if (i != fileIndex.end()) {
		FileInfoIter j = find(i->second.begin(), i->second.end(), fname);
		if (j != i->second.end()) {
			FileInfo& fi = *j;
			TreeIter ti = treeIndex.find(fi.getRoot());
			if (ti == treeIndex.end() || ti->second.getSize() != aSize || fi.getTimeStamp() != aTimeStamp) {
				i->second.erase(j);
				dirty = true;
				return false;
			}
			return true;
		}
	}
	return false;
}

const TTHValue* HashManager::HashStore::getTTH(const string& aFileName) {
	string fname = Text::toLower(Util::getFileName(aFileName));
	string fpath = Text::toLower(Util::getFilePath(aFileName));

	DirIter i = fileIndex.find(fpath);
	if (i != fileIndex.end()) {
		FileInfoIter j = find(i->second.begin(), i->second.end(), fname);
		if (j != i->second.end()) {
			j->setUsed(true);
			return &(j->getRoot());
		}
	}
	return NULL;
}

void HashManager::HashStore::rebuild() {
	try {
		DirMap newFileIndex;
		TreeMap newTreeIndex;

		for (DirIter i = fileIndex.begin(); i != fileIndex.end(); ++i) {
			for (FileInfoIter j = i->second.begin(); j != i->second.end(); ++j) {
				if (!j->getUsed())
					continue;

				TreeIter k = treeIndex.find(j->getRoot());
				if (k != treeIndex.end()) {
					newTreeIndex[j->getRoot()] = k->second;
				}
			}
		}

		string tmpName = getDataFile() + ".tmp";
		string origName = getDataFile();

		createDataFile(tmpName);

		{
			File in(origName, File::READ, File::OPEN);
			File out(tmpName, File::READ | File::WRITE, File::OPEN);

			for (TreeIter i = newTreeIndex.begin(); i != newTreeIndex.end();) {
				TigerTree tree;
				if (loadTree(in, i->second, i->first, tree)) {
					i->second.setIndex(saveTree(out, tree));
					++i;
				} else {
					newTreeIndex.erase(i++);
				}
			}
		}

		for (DirIter i = fileIndex.begin(); i != fileIndex.end(); ++i) {
			DirIter fi = newFileIndex.insert(make_pair(i->first, FileInfoList())).first;

			for (FileInfoIter j = i->second.begin(); j != i->second.end(); ++j) {
				if (newTreeIndex.find(j->getRoot()) != newTreeIndex.end()) {
					fi->second.push_back(*j);
				}
			}

			if (fi->second.empty())
				newFileIndex.erase(fi);
		}

		File::deleteFile(origName);
		File::renameFile(tmpName, origName);
		treeIndex = newTreeIndex;
		fileIndex = newFileIndex;
		dirty = true;
		save();
	} catch (const Exception& e) {
		LogManager::getInstance()->message(str(F_("Hashing failed: %1%") % e.getError()));
	}
}

void HashManager::HashStore::save() {
	if (dirty) {
		try {
			File ff(getIndexFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			BufferedOutputStream<false> f(&ff);

			string tmp;
			string b32tmp;

			f.write(SimpleXML::utf8Header);
			f.write(LIT("<HashStore Version=\"" HASH_FILE_VERSION_STRING "\">\r\n"));

			f.write(LIT("\t<Trees>\r\n"));

			for (TreeIter i = treeIndex.begin(); i != treeIndex.end(); ++i) {
				const TreeInfo& ti = i->second;
				f.write(LIT("\t\t<Hash Type=\"TTH\" Index=\""));
				f.write(Util::toString(ti.getIndex()));
				f.write(LIT("\" BlockSize=\""));
				f.write(Util::toString(ti.getBlockSize()));
				f.write(LIT("\" Size=\""));
				f.write(Util::toString(ti.getSize()));
				f.write(LIT("\" Root=\""));
				b32tmp.clear();
				f.write(i->first.toBase32(b32tmp));
				f.write(LIT("\"/>\r\n"));
			}

			f.write(LIT("\t</Trees>\r\n\t<Files>\r\n"));

			for (DirIter i = fileIndex.begin(); i != fileIndex.end(); ++i) {
				const string& dir = i->first;
				for (FileInfoIter j = i->second.begin(); j != i->second.end(); ++j) {
					const FileInfo& fi = *j;
					f.write(LIT("\t\t<File Name=\""));
					f.write(SimpleXML::escape(dir + fi.getFileName(), tmp, true));
					f.write(LIT("\" TimeStamp=\""));
					f.write(Util::toString(fi.getTimeStamp()));
					f.write(LIT("\" Root=\""));
					b32tmp.clear();
					f.write(fi.getRoot().toBase32(b32tmp));
					f.write(LIT("\"/>\r\n"));
				}
			}
			f.write(LIT("\t</Files>\r\n</HashStore>"));
			f.flush();
			ff.close();
			File::deleteFile( getIndexFile());
			File::renameFile(getIndexFile() + ".tmp", getIndexFile());

			dirty = false;
		} catch (const FileException& e) {
			LogManager::getInstance()->message(str(F_("Error saving hash data: %1%") % e.getError()));
		}
	}
}

class HashLoader: public SimpleXMLReader::CallBack {
public:
	HashLoader(HashManager::HashStore& s) :
		store(s), size(0), timeStamp(0), version(HASH_FILE_VERSION), inTrees(false), inFiles(false), inHashStore(false) {
	}
	virtual void startTag(const string& name, StringPairList& attribs, bool simple);
	virtual void endTag(const string& name, const string& data);

private:
	HashManager::HashStore& store;

	string file;
	int64_t size;
	uint32_t timeStamp;
	int version;

	bool inTrees;
	bool inFiles;
	bool inHashStore;
};

void HashManager::HashStore::load() {
	try {
		HashLoader l(*this);
		SimpleXMLReader(&l).fromXML(File(getIndexFile(), File::READ, File::OPEN).read());
	} catch (const Exception&) {
		// ...
	}
}

static const string sHashStore = "HashStore";
static const string sversion = "version"; // Oops, v1 was like this
static const string sVersion = "Version";
static const string sTrees = "Trees";
static const string sFiles = "Files";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sHash = "Hash";
static const string sType = "Type";
static const string sTTH = "TTH";
static const string sIndex = "Index";
static const string sLeafSize = "LeafSize"; // Residue from v1 as well
static const string sBlockSize = "BlockSize";
static const string sTimeStamp = "TimeStamp";
static const string sRoot = "Root";

void HashLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	if (!inHashStore && name == sHashStore) {
		version = Util::toInt(getAttrib(attribs, sVersion, 0));
		if (version == 0) {
			version = Util::toInt(getAttrib(attribs, sversion, 0));
		}
		inHashStore = !simple;
	} else if (inHashStore && version == 2) {
		if (inTrees && name == sHash) {
			const string& type = getAttrib(attribs, sType, 0);
			int64_t index = Util::toInt64(getAttrib(attribs, sIndex, 1));
			int64_t blockSize = Util::toInt64(getAttrib(attribs, sBlockSize, 2));
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 3));
			const string& root = getAttrib(attribs, sRoot, 4);
			if (!root.empty() && type == sTTH && (index >= 8 || index == HashManager::SMALL_TREE) && blockSize >= 1024) {
				store.treeIndex[TTHValue(root)] = HashManager::HashStore::TreeInfo(size, index, blockSize);
			}
		} else if (inFiles && name == sFile) {
			file = getAttrib(attribs, sName, 0);
			timeStamp = Util::toUInt32(getAttrib(attribs, sTimeStamp, 1));
			const string& root = getAttrib(attribs, sRoot, 2);

			if (!file.empty() && size >= 0 && timeStamp > 0 && !root.empty()) {
				string fname = Text::toLower(Util::getFileName(file));
				string fpath = Text::toLower(Util::getFilePath(file));

				store.fileIndex[fpath].push_back(HashManager::HashStore::FileInfo(fname, TTHValue(root), timeStamp,
				    false));
			}
		} else if (name == sTrees) {
			inTrees = !simple;
		} else if (name == sFiles) {
			inFiles = !simple;
		}
	}
}

void HashLoader::endTag(const string& name, const string&) {
	if (name == sFile) {
		file.clear();
	}
}

HashManager::HashStore::HashStore() :
	dirty(false) {
	if (File::getSize(getDataFile()) <= static_cast<int64_t> (sizeof(int64_t))) {
		try {
			createDataFile( getDataFile());
		} catch (const FileException&) {
			// ?
		}
	}
}

/**
 * Creates the data files for storing hash values.
 * The data file is very simple in its format. The first 8 bytes
 * are filled with an int64_t (little endian) of the next write position
 * in the file counting from the start (so that file can be grown in chunks).
 * We start with a 1 mb file, and then grow it as needed to avoid fragmentation.
 * To find data inside the file, use the corresponding index file.
 * Since file is never deleted, space will eventually be wasted, so a rebuild
 * should occasionally be done.
 */
void HashManager::HashStore::createDataFile(const string& name) {
	try {
		File dat(name, File::WRITE, File::CREATE | File::TRUNCATE);
		dat.setPos(1024 * 1024);
		dat.setEOF();
		dat.setPos(0);
		int64_t start = sizeof(start);
		dat.write(&start, sizeof(start));

	} catch (const FileException& e) {
		LogManager::getInstance()->message(str(F_("Error creating hash data file: %1%") % e.getError()));
	}
}

void HashManager::Hasher::hashFile(const string& fileName, int64_t size) {
	Lock l(cs);
	if (w.insert(make_pair(fileName, size)).second) {
		s.signal();
	}
}

void HashManager::Hasher::stopHashing(const string& baseDir) {
	Lock l(cs);
	for (WorkIter i = w.begin(); i != w.end();) {
		if (Util::strnicmp(baseDir, i->first, baseDir.length()) == 0) {
			w.erase(i++);
		} else {
			++i;
		}
	}
}

void HashManager::Hasher::getStats(string& curFile, int64_t& bytesLeft, size_t& filesLeft) {
	Lock l(cs);
	curFile = currentFile;
	filesLeft = w.size();
	if (running)
		filesLeft++;
	bytesLeft = 0;
	for (WorkMap::const_iterator i = w.begin(); i != w.end(); ++i) {
		bytesLeft += i->second;
	}
	bytesLeft += currentSize;
}

#ifdef _WIN32
#define BUF_SIZE (256*1024)

bool HashManager::Hasher::fastHash(const string& fname, uint8_t* buf, TigerTree& tth, int64_t size, CRC32Filter* xcrc32) {
	HANDLE h = INVALID_HANDLE_VALUE;
	DWORD x, y;
	if (!GetDiskFreeSpace(Text::toT(Util::getFilePath(fname)).c_str(), &y, &x, &y, &y)) {
		return false;
	} else {
		if ((BUF_SIZE % x) != 0) {
			return false;
		} else {
			h = ::CreateFile(Text::toT(fname).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			    FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
			if (h == INVALID_HANDLE_VALUE)
				return false;
		}
	}
	DWORD hn = 0;
	DWORD rn = 0;
	uint8_t* hbuf = buf + BUF_SIZE;
	uint8_t* rbuf = buf;

	OVERLAPPED over = { 0 };
	BOOL res = TRUE;
	over.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	bool ok = false;

	uint32_t lastRead = GET_TICK();
	if (!::ReadFile(h, hbuf, BUF_SIZE, &hn, &over)) {
		if (GetLastError() == ERROR_HANDLE_EOF) {
			hn = 0;
		} else if (GetLastError() == ERROR_IO_PENDING) {
			if (!GetOverlappedResult(h, &over, &hn, TRUE)) {
				if (GetLastError() == ERROR_HANDLE_EOF) {
					hn = 0;
				} else {
					goto cleanup;
				}
			}
		} else {
			goto cleanup;
		}
	}

	over.Offset = hn;
	size -= hn;
	for (;;) {
		if (size > 0) {
			// Start a new overlapped read
			ResetEvent(over.hEvent);
			if (SETTING(MAX_HASH_SPEED) > 0) {
				uint32_t now = GET_TICK();
				uint32_t minTime = hn * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
				if (lastRead + minTime > now) {
					uint32_t diff = now - lastRead;
					Thread::sleep(minTime - diff);
				}
				lastRead = lastRead + minTime;
			} else {
				lastRead = GET_TICK();
			}
			res = ReadFile(h, rbuf, BUF_SIZE, &rn, &over);
		} else {
			rn = 0;
		}

		tth.update(hbuf, hn);
		if (xcrc32)
			(*xcrc32)(hbuf, hn);

		{
			Lock l(cs);
			currentSize = max(currentSize - hn, _LL(0));
		}

		if (size == 0) {
			ok = true;
			break;
		}

		if (!res) {
			// deal with the error code
			switch (GetLastError()) {
			case ERROR_IO_PENDING:
				if (!GetOverlappedResult(h, &over, &rn, TRUE)) {
					dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
					goto cleanup;
				}
				break;
				default:
				dcdebug("Error 0x%x: %s\n", GetLastError(), Util::translateError(GetLastError()).c_str());
				goto cleanup;
			}
		}

		*((uint64_t*)&over.Offset) += rn;
		size -= rn;

		swap(rbuf, hbuf);
		swap(rn, hn);
	}

	cleanup:
	::CloseHandle(over.hEvent);
	::CloseHandle(h);
	return ok;
}

#else // !_WIN32
static const int64_t BUF_SIZE = 0x1000000 - (0x1000000 % getpagesize());

bool HashManager::Hasher::fastHash(const string& filename, uint8_t* , TigerTree& tth, int64_t size, CRC32Filter* xcrc32) {
	int fd = open(Text::fromUtf8(filename).c_str(), O_RDONLY);
	if(fd == -1)
	return false;

	int64_t size_left = size;
	int64_t pos = 0;
	int64_t size_read = 0;
	void *buf = 0;

	uint32_t lastRead = GET_TICK();
	while(pos <= size) {
		if(size_left> 0) {
			size_read = std::min(size_left, BUF_SIZE);
			buf = mmap(0, size_read, PROT_READ, MAP_SHARED, fd, pos);
			if(buf == MAP_FAILED) {
				close(fd);
				return false;
			}

			madvise(buf, size_read, MADV_SEQUENTIAL | MADV_WILLNEED);

			if(SETTING(MAX_HASH_SPEED)> 0) {
				uint32_t now = GET_TICK();
				uint32_t minTime = size_read * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
				if(lastRead + minTime> now) {
					uint32_t diff = now - lastRead;
					Thread::sleep(minTime - diff);
				}
				lastRead = lastRead + minTime;
			} else {
				lastRead = GET_TICK();
			}
		} else {
			size_read = 0;
		}

		tth.update(buf, size_read);
		if(xcrc32)
		(*xcrc32)(buf, size_read);
		{
			Lock l(cs);
			currentSize = max(static_cast<uint64_t>(currentSize - size_read), static_cast<uint64_t>(0));
		}

		if(size_left <= 0) {
			break;
		}

		munmap(buf, size_read);
		pos += size_read;
		size_left -= size_read;
	}
	close(fd);
	return true;
}

#endif // !_WIN32
int HashManager::Hasher::run() {
	setThreadPriority(Thread::IDLE);

	uint8_t* buf = NULL;
	bool virtualBuf = true;

	string fname;
	bool last = false;
	for(;;) {
		s.wait();
		if(stop)
		break;
		if(rebuild) {
			HashManager::getInstance()->doRebuild();
			rebuild = false;
			LogManager::getInstance()->message(_("Hash database rebuilt"));
			continue;
		}
		{
			Lock l(cs);
			if(!w.empty()) {
				currentFile = fname = w.begin()->first;
				currentSize = w.begin()->second;
				w.erase(w.begin());
				last = w.empty();
			} else {
				last = true;
				fname.clear();
			}
		}
		running = true;

		if(!fname.empty()) {
			int64_t size = File::getSize(fname);
			int64_t sizeLeft = size;
#ifdef _WIN32
			if(buf == NULL) {
				virtualBuf = true;
				buf = (uint8_t*)VirtualAlloc(NULL, 2*BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);
			}
#endif
			if(buf == NULL) {
				virtualBuf = false;
				buf = new uint8_t[BUF_SIZE];
			}
			try {
				File f(fname, File::READ, File::OPEN);
				int64_t bs = max(TigerTree::calcBlockSize(f.getSize(), 10), MIN_BLOCK_SIZE);
				uint32_t start = GET_TICK();
				uint32_t timestamp = f.getLastModified();
				TigerTree slowTTH(bs);
				TigerTree* tth = &slowTTH;

				CRC32Filter crc32;
				SFVReader sfv(fname);
				CRC32Filter* xcrc32 = 0;
				if(sfv.hasCRC())
				xcrc32 = &crc32;

				size_t n = 0;
				TigerTree fastTTH(bs);
				tth = &fastTTH;
#ifdef _WIN32
				if(!virtualBuf || !BOOLSETTING(FAST_HASH) || !fastHash(fname, buf, fastTTH, size, xcrc32)) {
#else
					if(!BOOLSETTING(FAST_HASH) || !fastHash(fname, 0, fastTTH, size, xcrc32)) {
#endif
						tth = &slowTTH;
						crc32 = CRC32Filter();
						uint32_t lastRead = GET_TICK();

						do {
							size_t bufSize = BUF_SIZE;
							if(SETTING(MAX_HASH_SPEED)> 0) {
								uint32_t now = GET_TICK();
								uint32_t minTime = n * 1000LL / (SETTING(MAX_HASH_SPEED) * 1024LL * 1024LL);
								if(lastRead + minTime> now) {
									Thread::sleep(minTime - (now - lastRead));
								}
								lastRead = lastRead + minTime;
							} else {
								lastRead = GET_TICK();
							}
							n = f.read(buf, bufSize);
							tth->update(buf, n);
							if(xcrc32) (*xcrc32)(buf, n);

							{
								Lock l(cs);
								currentSize = max(static_cast<uint64_t>(currentSize - n), static_cast<uint64_t>(0));
							}
							sizeLeft -= n;
						}while (n> 0 && !stop);
					} else {
						sizeLeft = 0;
					}

					f.close();
					tth->finalize();
					uint32_t end = GET_TICK();
					int64_t speed = 0;
					if(end> start) {
						speed = size * _LL(1000) / (end - start);
					}
					if(xcrc32 && xcrc32->getValue() != sfv.getCRC()) {
						LogManager::getInstance()->message(str(F_("%1% not shared; calculated CRC32 does not match the one found in SFV file.") % fname));
					} else {
						HashManager::getInstance()->hashDone(fname, timestamp, *tth, speed);
					}
				} catch(const FileException& e) {
					LogManager::getInstance()->message(str(F_("Error hashing %1%: %2%") % fname % e.getError()));
				}
			}
			{
				Lock l(cs);
				currentFile.clear();
				currentSize = 0;
			}
			running = false;
			if(buf != NULL && (last || stop)) {
				if(virtualBuf) {
#ifdef _WIN32
					VirtualFree(buf, 0, MEM_RELEASE);
#endif
				} else {
					delete [] buf;
				}
				buf = NULL;
			}
		}
		return 0;
	}

} // namespace dcpp
