Introduction
------------
LinuxDC++ is a Linux port of the Direct Connect client DC++. Though it is
primarily aimed at Linux, it has been shown to work on other Unix-based
operating systems as well. It is written in C++ and makes use of GTK+ for the
user-interface. LinuxDC++ is free and open source software licensed under the
GPL.

Direct connect is a peer-to-peer file-sharing protocol. The most popular Windows
client implementing this protocol currently is DC++. Direct Connect clients
connect to a central hub where they can view a list of clients or users
connected to them. Users can search for files and download them from other
clients, as well as chat with other users.

This readme contains only the basic information. For more detailed instructions,
read the manual in the wiki:

http://openfacts.berlios.de/index-en.phtml?title=Ldcpp_Manual


Dependencies:
-------------
scons >= 0.96
pkg-config
g++ >= 3.4
gtk+-2.0 >= 2.10
gthread-2.0 >= 2.4
libglade-2.0 >= 2.4
pthread
zlib
libbz2
libssl


Compiling:
----------
$ cd /path/to/linuxdcpp
$ scons PREFIX=/path/to/install

Scons will output if something's missing in a clear and human-readable way.
The PREFIX parameter is the path that the program will be installed to. PREFIX
defaults to /usr/local, so it need only be specified if a different
directory is desired. Note the program doesn't have to be installed in order to
be ran. See the second part of the Running section for more details.

For compile options, type "scons --help".


Installing:
-----------
$ scons install

This command may need to be ran as root (sudo, su, etc.) depending upon
the PREFIX path that was supplied when compiling.


Running:
--------
If LinuxDC++ was installed using scons install, simply run the executable:

$ linuxdcpp

To run the program from the source directory instead of installing, first
navigate to that directory then run "./linuxdcpp". This is an important
point: scripts calling linuxdcpp in the source directory have to first cd to
that path in order to work (since it will look for pixmaps using the path
"./pixmaps" and this will fail if it's not in the correct directory).
Linuxdcpp no longer dynamically finds the location of the binary using
binreloc (since it wasn't portable).


Using:
------
Open the preferences and change the necessary options to your liking. If you're
behind a firewall or router, make sure to either set your connection type to
passive/firewall or configure the device to allow port forwarding. Read
http://dcpp.net/faq/ for more information on this topic.

To minimize any character encoding issues, specify the default hub encoding on
the personal tab in preferences. This should be set to the character encoding
most commonly used in one's favorite hubs. Additionally, a per-hub character
encoding option can be found on the favorite hubs tab.

License:
--------
The GPL (couldn't be any other, could it?)
See License.txt for details.


Contact:
--------
Website: https://launchpad.net/linuxdcpp
IRC: #linuxdc++@freenode.org

