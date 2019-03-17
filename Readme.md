# FreeDC++ &ndash; file sharing using DC and ADC protocols

Introduction
------------
FreeDC++ -- DC++ client based on LinuxDC++ source code.<br/>
See changes in [Changelog.txt](https://github.com/eiskaltdcpp/freedcpp/blob/master/Changelog.txt), [Changelog-svn.txt](https://github.com/eiskaltdcpp/freedcpp/blob/master/Changelog-svn.txt) and [commits history](https://github.com/eiskaltdcpp/freedcpp/commits/master).

License
-------
GNU GPL v2.0 or later. See [License.txt](https://github.com/eiskaltdcpp/freedcpp/blob/master/License.txt) for details.

Contact
-------
Website:
* http://freedcpp.narod.ru
* https://github.com/eiskaltdcpp/freedcpp

Dependencies
------------
* scons >= 0.96
* pkg-config
* g++ >= 3.4
* gtk+-2.0 >= 2.10
* gthread-2.0 >= 2.4
* libglade-2.0 >= 2.4
* pthread
* zlib
* libbz2
* libssl
* libgnome
* libnotify >= 0.4.1
* libboost
* libminiupnpc

Compiling and installing
------------------------
```
$ cd /path/to/freedcpp
$ scons PREFIX=/path/to/install
$ sudo scons install

```

Running
-------
```
$ freedcpp
```
or
```
$ /path/to/install/bin/freedcpp
```

Uninstalling
------------
To uninstall FreeDC++, simply delete the directory you installed it in.

To uninstall using scons, run:
```
$ scons -c
```

or if you used scons to install into the file system then use the same scons command that you used to install and add the option -c:
```
$ sudo scons -c install
```

