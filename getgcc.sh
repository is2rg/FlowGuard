#!/bin/bash

# Set a root folder (needs absolute path)
export BASEDIR=$(pwd)
GCCVERSION="5.4.0"


# Get GCC
wget http://ftp.gnu.org/gnu/gcc/gcc-$GCCVERSION/gcc-$GCCVERSION.tar.bz2
tar xfj gcc-$GCCVERSION.tar.bz2

# Get the required software to build it
cd gcc-$GCCVERSION/
./contrib/download_prerequisites

# Create build directory
cd ..
mkdir gcc-build-$GCCVERSION
cd gcc-build-$GCCVERSION
export INSTALLDIR=$BASEDIR/gcc-install-$GCCVERSION
mkdir -p $INSTALLDIR

# Configure gcc
../gcc-$GCCVERSION/configure --prefix=$INSTALLDIR --enable-languages=c,c++

# Build & install
make -j$(nproc) > /dev/null 2>&1
make install

# Check the installation
export GCCDIR=$INSTALLDIR/bin
$GCCDIR/g++ --version
