# FreeDC++ &ndash; file sharing program using Direct Connect protocols

Website: http://freedcpp.narod.ru (outdated)
Sources: https://github.com/eiskaltdcpp/freedcpp

## License

GNU GPL v2.0 or later. See [License.txt](https://github.com/eiskaltdcpp/freedcpp/blob/master/License.txt) for details.

## Description

FreeDC++ &ndash; DC++ client based on [LinuxDC++](https://launchpad.net/linuxdcpp) source code.<br/>
See changes in [Changelog.txt](https://github.com/eiskaltdcpp/freedcpp/blob/master/Changelog.txt) and [commits history](https://github.com/eiskaltdcpp/freedcpp/commits/master).

FreeDC++ code base was used in [BMDC++](https://github.com/bmdcpp/bmdc) and [EiskaltDC++ GTK](https://github.com/eiskaltdcpp/eiskaltdcpp/tree/v2.4.2/eiskaltdcpp-gtk) projects.

**Attention!** Active project development was stopped long time ago and its usage is not recommended!  FreeDC++ users should consider switching to one of these modern clients: [EiskaltDC++](https://github.com/eiskaltdcpp/eiskaltdcpp#readme), [BMDC++](https://bmdcpp.github.io/), [AirDC++](https://airdcpp-web.github.io/), [Ncdc](https://dev.yorhel.nl/ncdc).

## Dependencies

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
* libcanberra
* libnotify
* libboost
* libminiupnpc

## Compiling and installing

```
$ cd /path/to/freedcpp
$ scons PREFIX=/path/to/install
$ sudo scons install

```

## Running

```
$ freedcpp
```
or
```
$ /path/to/install/bin/freedcpp
```

## Uninstalling

To uninstall FreeDC++, simply delete the directory you installed it in.

To uninstall using scons, run:
```
$ scons -c
```

or if you used scons to install into the file system then use the same scons command that you used to install and add the option -c:
```
$ sudo scons -c install
```

