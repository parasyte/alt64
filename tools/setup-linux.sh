#!/bin/bash
# Download and install latest updates for the system [sudo req.]
apt-get update
apt-get -y upgrade

# Install essential packages [sudo req.]
apt-get -y install build-essential git texinfo libc6 libgmp-dev libmpfr-dev libmpc-dev libpng-dev zlib1g-dev libtool autoconf

# change to the users root directory
cd ~/

# add a system variable and make it perminent
# echo 'N64_INST=/usr/local/libdragon' >> /etc/environment
# echo 'export N64_INST=/usr/local/libdragon' >> ~/.bashrc
export N64_INST=/usr/local/libdragon
# source ~/.bashrc

# Pull the latest libdragon source code and make a build directory
git clone https://github.com/dragonminded/libdragon.git

# fix issues with the build scripts
sed -i -- 's|${N64_INST:-/usr/local}|/usr/local/libdragon|g' libdragon/tools/build
sed -i -- 's|--with-newlib|--with-newlib --with-system-zlib|g' libdragon/tools/build

sed -i -- 's| -lpng|\nLDLIBS = -lpng|g' libdragon/tools/mksprite/Makefile
sed -i -- 's| -Werror| -w|g' libdragon/tools/mksprite/Makefile

# make a build folder for libdragon
mkdir libdragon/build_gcc
cp libdragon/tools/build libdragon/build_gcc

# run the build script (this will take a while! and if not sudo, will ask for password mid flow!)
cd libdragon/build_gcc
./build

cd ..
# run the install script [sudo req]
make
make install
make tools
make tools-install

cd ..
# install libmikmod
git clone https://github.com/parasyte/libmikmod-n64.git
cd libmikmod-n64
mkdir build
make
make install

cd ..
# install libmad-64
git clone https://github.com/parasyte/libmad-n64.git
cd libmad-n64
#$(N64_INST) converterd to $N64_INST below otherwise it will not run on WSFL
export PATH=$PATH:$N64_INST/bin
CFLAGS="-march=vr4300 -mtune=vr4300 -mno-extern-sdata" \
LDFLAGS="-L$N64_INST/lib" LIBS="-lc -lnosys" \
./configure --host=mips64-elf --disable-shared \
--prefix=$N64_INST --enable-speed --enable-fpm=mips
make
make install

cd ..
# install libyaml
git clone https://github.com/yaml/libyaml
cd libyaml
./bootstrap
#$(N64_INST) converterd to $N64_INST below otherwise it will not run on WSFL
export PATH=$PATH:$N64_INST/bin
CFLAGS="-std=gnu99 -march=vr4300 -mtune=vr4300" \
LDFLAGS="-L$N64_INST/lib -Tn64ld.x" \
LIBS="-ldragon -lc -ldragonsys -lnosys" \
./configure --host=mips64-elf --prefix=$N64_INST
make
make install

cd ..
# Install menu code (perhaps this script should be called from it instead?
# git clone https://github.com/networkfusion/alt64.git
# cd alt64
# make

# Perform cleanup
apt-get -y autoremove
apt-get autoclean

