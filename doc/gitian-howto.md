This guide documents in detail how Gitian can be used on an Ubuntu 16.04 system using KVM to virtualize the build environments for Bitmark and produce verifiable binaries. Gitian can be used with other virtualization methods, and on other operating systems, but they're outside the scope of this guide.

## Gitian Overview

Gitian is a piece of software that makes it possible for many different people to create identical binary builds of a cryptocurrency. Since cyrptocurrency software is particularly at risk for poisoning for monetary gain, having a procedure to create a verifiably clean build is important. With Gitian, many different unaffiliated individuals can build a binary release and check to see that the same result was achieved, and therefore certify that the binary is likely safe, and has not been tampered with. All parties building the binary would have to collude to tamper, and this is less and less likely the more people have posted the result of their Gitian build.

## Instructions

First, install the pre-requisite packages and give yourself permission to KVM virtualization.
```
sudo apt-get install python-vm-builder git python-vm-builder ruby qemu-utils apt-cacher-ng
sudo adduser `id -un` kvm
sudo reboot
```

Next, check out the sourcecode for Gitian and Bitmark.
```
git clone https://github.com/devrandom/gitian-builder.git
git clone https://github.com/project-bitmark/bitmark
```

Inside Bitmark's source we see Gitian descriptors. Each file details a single build action, including source files to load, what type of machine images to use, and exactly what build commands to use. Looking inside them is pretty self explanatory, and can be useful for debugging if this guide becomes out of date.
```
$ ls -l contrib/gitian-descriptors
-rw-rw-r-- 1 icook icook 1749 Nov  5 21:12 boost-linux.yml
-rw-rw-r-- 1 icook icook 3900 Oct 18 23:22 boost-win.yml
-rw-rw-r-- 1 icook icook 3727 Nov  5 21:12 deps-linux.yml
-rw-rw-r-- 1 icook icook 4579 Oct 18 23:22 deps-win.yml
-rw-rw-r-- 1 icook icook 2896 Nov  5 21:12 gitian-linux.yml
-rw-rw-r-- 1 icook icook 2629 Oct 18 23:22 gitian-osx-bitmark.yml
-rw-rw-r-- 1 icook icook 6443 Oct 18 23:22 gitian-osx-depends.yml
-rw-rw-r-- 1 icook icook 6476 Oct 18 23:22 gitian-osx-native.yml
-rw-rw-r-- 1 icook icook 6293 Oct 18 23:22 gitian-osx-qt.yml
-rw-rw-r-- 1 icook icook 3834 Oct 18 23:22 gitian-win.yml
-rw-rw-r-- 1 icook icook 2006 Oct 18 23:22 protobuf-win.yml
-rw-rw-r-- 1 icook icook 8557 Nov  5 21:12 qt-linux.yml
-rw-rw-r-- 1 icook icook 4574 Oct 18 23:22 qt-win.yml
```

Now create the  virtual machine templates that will be used to run our builds. These are the machines required by the Gitian build profiles that we were just discussing.
```
cd gitian-builder
./bin/make-base-vm --arch amd64 --suite precise
./bin/make-base-vm --arch i386 --suite precise
```

We must now obtain all the build dependencies for Bitmark. Execute the following commands to download all requisite dependencies with the exception of OSX libraries.
```
mkdir inputs
cd inputs
wget 'http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.9.20140701.tar.gz' -O miniupnpc-1.9.20140701.tar.gz
wget 'https://www.openssl.org/source/openssl-1.0.1k.tar.gz'
wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
wget 'http://zlib.net/zlib-1.2.8.tar.gz'
wget 'ftp://ftp.simplesystems.org/pub/png/src/history/libpng16/libpng-1.6.8.tar.gz'
wget 'https://fukuchi.org/works/qrencode/qrencode-3.4.3.tar.bz2'
wget 'https://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.bz2'
wget 'https://svn.boost.org/trac/boost/raw-attachment/ticket/7262/boost-mingw.patch' -O boost-mingw-gas-cross-compile-2013-03-03.patch
wget 'https://download.qt-project.org/official_releases/qt/5.2/5.2.1/single/qt-everywhere-opensource-src-5.2.1.tar.gz'
wget 'https://download.qt-project.org/archive/qt/4.8/4.8.6/qt-everywhere-opensource-src-4.8.6.tar.gz'
wget 'https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.bz2'
wget 'https://github.com/mingwandroid/toolchain4/archive/10cc648683617cca8bcbeae507888099b41b530c.tar.gz'
wget 'http://www.opensource.apple.com/tarballs/cctools/cctools-809.tar.gz'
wget 'http://www.opensource.apple.com/tarballs/dyld/dyld-195.5.tar.gz'
wget 'http://www.opensource.apple.com/tarballs/ld64/ld64-127.2.tar.gz'
wget 'http://pkgs.fedoraproject.org/repo/pkgs/cdrkit/cdrkit-1.1.11.tar.gz/efe08e2f3ca478486037b053acd512e9/cdrkit-1.1.11.tar.gz'
wget 'https://github.com/theuni/libdmg-hfsplus/archive/libdmg-hfsplus-v0.1.tar.gz'
wget 'http://llvm.org/releases/3.2/clang+llvm-3.2-x86-linux-ubuntu-12.04.tar.gz' -O clang-llvm-3.2-x86-linux-ubuntu-12.04.tar.gz
wget 'https://raw.githubusercontent.com/theuni/osx-cross-depends/master/patches/cdrtools/genisoimage.diff' -O cdrkit-deterministic.patch
```

OSX Libraries are a bit of a pain, so you can skip this is you're not building OSX for some reason. On an OSX machine,  visit [Apple Developer Downloads](
https://developer.apple.com/download/more/?path=xcode%204.6.3), sign in with a Apple Developer account, and search for "xcode 4.6.3". You now need to download the `.dmg` image. Mount that image, and then extract a small chunk of the image to an archive. Transfer that archive to the inputs folder on your original Ubuntu machine.

```
tar -C /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ -czf MacOSX10.7.sdk.tar.gz MacOSX10.7.sdk
```

Now we must build intermediate builds. These are collections of built dependencies that will be used to build the final binaries.

**Linux Dependencies**
```
./bin/gbuild ../bitmark/contrib/gitian-descriptors/boost-linux.yml
mv build/out/boost-*.zip inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/deps-linux.yml
mv build/out/bitmark-deps-*.zip inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/qt-linux.yml
mv build/out/qt-*.tar.gz inputs/
```

**Windows Dependencies**
```
./bin/gbuild ../bitmark/contrib/gitian-descriptors/boost-win.yml
mv build/out/boost-*.zip inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/deps-win.yml
mv build/out/bitmark-deps-*.zip inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/qt-win.yml
mv build/out/qt-*.zip inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/protobuf-win.yml
mv build/out/protobuf-*.zip inputs/
```

**OSX Dependencies**
```
./bin/gbuild ../bitmark/contrib/gitian-descriptors/gitian-osx-native.yml
mv build/out/osx-*.tar.gz inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/gitian-osx-depends.yml
mv build/out/osx-*.tar.gz inputs/
./bin/gbuild ../bitmark/contrib/gitian-descriptors/gitian-osx-qt.yml
mv build/out/osx-*.tar.gz inputs/
```

Before the final builds we set some variables that will be used a lot in the final build commands. Also we make a folder to hold our Gitian signatures.
```
export SIGNER=(your gitian key, ie bluematt, sipa, etc)
export VERSION=(new version, e.g. 0.8.0)
mkdir ../gitian.sigs
```
**Build Linux**
```
./bin/gbuild --commit bitmark=v${VERSION} ../bitmark/contrib/gitian-descriptors/gitian-linux.yml
./bin/gsign --signer $SIGNER --release ${VERSION} --destination ../gitian.sigs/ ../bitmark/contrib/gitian-descriptors/gitian-linux.yml
zip -rj bitmark-${VERSION}-linux-gitian.zip build/out/*
mv bitmark-${VERSION}-linux-gitian.zip ..
```
**Build Windows**
```
./bin/gbuild --commit bitmark=v${VERSION} ../bitmark/contrib/gitian-descriptors/gitian-win.yml
./bin/gsign --signer $SIGNER --release ${VERSION}-win --destination ../gitian.sigs/ ../bitmark/contrib/gitian-descriptors/gitian-win.yml
zip -rj bitmark-${VERSION}-win-gitian.zip build/out/*
mv bitmark-${VERSION}-win-gitian.zip ..
```
**Build OSX**
```
./bin/gbuild --commit bitmark=v${VERSION} ../bitmark/contrib/gitian-descriptors/gitian-osx-bitmark.yml
./bin/gsign --signer $SIGNER --release ${VERSION}-osx --destination ../gitian.sigs/ ../bitmark/contrib/gitian-descriptors/gitian-osx-bitmark.yml
mv build/out/Bitmark-Qt.dmg ..
```
 We now have built binaries and our associated signatures.
