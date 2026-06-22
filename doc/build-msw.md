WINDOWS BUILD NOTES
===================


Compilers Supported
-------------------
TODO: What works?
Note: releases are cross-compiled using mingw running on Linux.


Dependencies
------------
Libraries you need to download separately and build:

	name            default path               download
	--------------------------------------------------------------------------------------------------------------------
	OpenSSL         \openssl-1.0.1c-mgw        http://www.openssl.org/source/
	Berkeley DB     \db-4.8.30.NC-mgw          http://www.oracle.com/technology/software/products/berkeley-db/index.html
	Boost           \boost-1.50.0-mgw          http://www.boost.org/users/download/
	miniupnpc       \miniupnpc-1.6-mgw         http://miniupnp.tuxfamily.org/files/

Their licenses:

	OpenSSL        Old BSD license with the problematic advertising requirement
	Berkeley DB    New BSD license with additional requirement that linked software must be free open source
	Boost          MIT-like license
	miniupnpc      New (3-clause) BSD license

Versions used in this release:

	OpenSSL      1.0.1c
	Berkeley DB  4.8.30.NC
	Boost        1.50.0
	miniupnpc    1.6


OpenSSL
-------
MSYS shell:

un-tar sources with MSYS 'tar xfz' to avoid issue with symlinks (OpenSSL ticket 2377)
change 'MAKE' env. variable from 'C:\MinGW32\bin\mingw32-make.exe' to '/c/MinGW32/bin/mingw32-make.exe'

	cd /c/openssl-1.0.1c-mgw
	./config
	make

Berkeley DB
-----------
MSYS shell:

	cd /c/db-4.8.30.NC-mgw/build_unix
	sh ../dist/configure --enable-mingw --enable-cxx
	make

Boost
-----
MSYS shell:

	downloaded boost jam 3.1.18
	cd \boost-1.50.0-mgw
	bjam toolset=gcc --build-type=complete stage

MiniUPnPc
---------
UPnP support is optional, make with `USE_UPNP=` to disable it.

MSYS shell:

	cd /c/miniupnpc-1.6-mgw
	make -f Makefile.mingw
	mkdir miniupnpc
	cp *.h miniupnpc/

Bitmark
-------
MSYS shell:

	cd \bitmark
	sh autogen.sh
	sh configure
	mingw32-make
	strip bitmarkd.exe

Version 0.9.7.5 Building Instructions
-------------------------------------
Cross-compiling on Ubuntu with mxe:

	git clone https://github.com/mxe/mxe
	cd mxe
	git checkout be62fd6d3703f2136c812500fb828506b8bc488b
	export PATH="$PWD/usr/bin:$PATH"
	export PREFIX="$PWD/usr/x86_64-w64-mingw32.static"
	make MXE_TARGETS='x86_64-w64-mingw32.static' cc qtbase openssl libqrencode miniupnpc libsodium qttools

	wget https://download.oracle.com/berkeley-db/db-4.8.30.tar.gz
	(sha256sum should be e0491a07cdb21fb9aa82773bbbedaeb7639cbd0e7f96147ab46141e0045db72a)
	tar -xzf ~/db-4.8.30.tar.gz
	cd db-4.8.30/
	cd build_unix/
	../dist/configure --enable-mingw --enable-cxx --prefix=$PREFIX --host=x86_64-w64-mingw32.static --disable-pthread_api --disable-replication --disable-shared --enable-static
	make
	make install

	git clone https://github.com/google/protobuf
	cd protobuf
	git checkout v3.9.0
	./autogen.sh
	./configure --prefix=/usr/local --enable-shared=no
	make
	sudo make install

	git clone https://github.com/akrmn2021/bitmark
	cd bitmark
	git checkout v0.9.7.5 (or latest version)
	./autogen.sh
	./configure --prefix=/usr/local --host=x86_64-w64-mingw32.static --with-gui=qt5 --with-boost-system=boost_system-mt --with-boost-filesystem=boost_filesystem-mt --with-boost-program-options=boost_program_options-mt --with-boost-thread=boost_thread_win32-mt --with-boost-chrono=boost_chrono-mt --with-boost-unit-test-framework=boost_unit_test_framework-mt --with-qt-incdir=$PREFIX/qt5/include --with-qt-libdir=$PREFIX/qt5/lib --with-qt-plugindir=$PREFIX/qt5/plugins --with-qt-bindir=$PREFIX/qt5/bin
	make
	sudo make install

	(replace x86_64 with i686 if building as a 32 bit Windows application)
