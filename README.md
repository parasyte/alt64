# alt64

Alternative Everdrive64 menu

    Kuroneko!

           :\     /;               _
          ;  \___/  ;             ; ;
         ,:-"'   `"-:.            / ;
    _   /,---.   ,---.\   _     _; /
    _:>((  |  ) (  |  ))<:_ ,-""_,"
        \`````   `````/""""",-""
         '-.._ v _..-'      )
           / ___   ____,..  \
          / /   | |   | ( \. \
    ctr  / /    | |    | |  \ \
         `"     `"     `"    `"

    nyannyannyannyannyannyannyannyannyannyannyannyannyannyannyannyannyan


`alt64` is an open source menu for [Everdrive64](http://krikzz.com/). It was
originally written by saturnu, and released on the
[Everdrive64 forum](http://krikzz.com/forum/index.php?topic=816.0).

## Building

If you want to build the menu, you need an n64 toolchain. We'll use the
toolchain recommended by libdragon, with some updated versions.

### Dependencies

* [libdragon](https://github.com/parasyte/libdragon)
* [libmikmod-n64](https://github.com/parasyte/libmikmod-n64)
* [libmad-n64](https://github.com/parasyte/libmad-n64)

### Build the Toolchain

*You may skip this step if it's already installed.*

Clone the `libdragon` repo and create a directory for the build.

```bash
$ git clone https://github.com/parasyte/libdragon.git
$ mkdir libdragon/build_gcc
$ cp libdragon/tools/build libdragon/build_gcc
$ cd libdragon/build_gcc
```

Modify the `build` script to set your installation path. Here is the default:

```bash
# EDIT THIS LINE TO CHANGE YOUR INSTALL PATH!
export INSTALL_PATH=/usr/local/mips64-elf
```

Build it! This can take an hour or more.

```bash
$ ./build
```

### Configure your Environment

Add the following environment variable to your `~/.bash_profile` or `~/.bashrc`
Be sure to use the same path that you configured in the build script!

```bash
export N64_INST=/usr/local/mips64-elf
```

### Build `libdragon`

Make sure you are in the `libdragon` top-level directory, and make sure `libpng`
is installed:

```bash
$ make && make install
$ make tools && make tools-install
```

### Build `libmikmod`

Clone `libmikmod-n64` and build:

```bash
$ git clone https://github.com/parasyte/libmikmod-n64.git
$ cd libmikmod-n64
$ make
$ make install
```

### Build `libmad-n64`

Clone `libmad-n64` and build, be sure to set the path according to your
toolchain installation path:

```bash
$ git clone https://github.com/parasyte/libmad-n64.git
$ cd libmad-n64
$ export PATH=$PATH:/usr/local/mips64-elf/bin
$ CFLAGS="-march=vr4300 -mtune=vr4300 -mno-extern-sdata" \
  LDFLAGS="-L/usr/local/mips64-elf/lib" LIBS="-lc -lnosys" \
  ./configure --host=mips64-elf --disable-shared \
  --prefix=/usr/local/mips64-elf --enable-speed --enable-fpm=mips
$ make
$ make install
```

### Build `alt64`

Finally, we can clone `alt64` and build it!

```bash
$ git clone https://github.com/parasyte/alt64.git
$ make
```

If it all worked, you will find `OS64.v64` in the `alt64` top-level directory.
