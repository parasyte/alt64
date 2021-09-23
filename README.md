# Altra64

Alternative Everdrive64 menu


`Altra64` is an open source menu for [Everdrive64](http://krikzz.com/) and is based on a fork of alt64 which was
originally written by saturnu, and released on the
[Everdrive64 forum](http://krikzz.com/forum/index.php?topic=816.0).

## Building
Clone this `Altra64` repo to a directory of your choice.

### Build `Altra64`
If this is the first time building, ensure you create the following folders in the root directory `bin` `obj` and `lib` 
To install the dependencies run: `update-libs.ps1`

To build the ROM

from the projects root directory,
On Windows 10 run 
```
> build
```
on linux
```
$ make
```
If it all worked, you will find `OS64P.V64` in the `Altra64` bin directory.

### Debug Build `Altra64`
To build the debug version of the Rom

from the projects root directory,
On Windows 10 run 
```
> build debug
```
on linux
```
$ make debug
```
If it all worked, you will find `OS64P.V64` in the `Altra64` bin directory.


### Clean `Altra64`
Finally, we can clean the build objects from the project

from the projects root directory,
On Windows 10 run 
```
> build clean
```
on linux
```
$ make clean
```

Enjoy!
