Introduction:
-------------
FreeDC++ -DC++ клиент основан на исходном коде LinuxDC++
Изменения смотрите в Changelog.txt, Changelog-svn.txt

Dependencies:
-------------
scons >= 1.2
pkg-config
g++ >= 4.5
gtk+-2.0 >= 2.10
gthread-2.0 >= 2.4
libglade-2.0 >= 2.4
pthread
zlib
libbz2
libssl
libgnome
libnotify >= 0.4.1
libboost >= 1.42
libminiupnpc

Compiling:
----------
$ cd /path/to/freedcpp
$ scons PREFIX=/path/to/install

Installing:
-----------
# scons install

Running:
--------
$ freedcpp

Uninstalling:
-------------
To uninstall FreeDC++, simply delete the directory you installed it in. To uninstall using scons, run:

$ scons -c

or if you used scons to install into the file system then use the same scons command that you used to install and add the option -c:

# scons -c install

License:
--------
GNU GPL Version 2
See License.txt for details.

Contact:
--------
Website: http://code.google.com/p/freedcpp http://freedcpp.narod.ru
