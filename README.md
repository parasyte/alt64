# Altra64

Alternative Everdrive64 menu


`Altra64` is an open source menu for [Everdrive64](http://krikzz.com/) and is based on a fork of alt64 which was
originally written by saturnu, and released on the
[Everdrive64 forum](http://krikzz.com/forum/index.php?topic=816.0).

## Building

If you want to build the menu, you need an n64 toolchain. When using windows 10 or Ubuntu, installation is totally automated through a script.

### Dependencies (installed automatically)
* Windows 10 (Aniversary Update or above) / Ubuntu 16.04 (or above)
* [libdragon](https://github.com/DragonMinded/libdragon)
* [libmikmod (with n64 driver)](https://github.com/networkfusion/libmikmod)
* [libmad-n64](https://github.com/networkfusion/libmad-n64)
* [libyaml](http://pyyaml.org/wiki/LibYAML)

### Build the Toolchain

*You may skip this step if it's already installed.*

Clone this `Altra64` repo to a directory of your choice.

On Windows 10:
* Ensure that ["Windows Subsystem For Linux (Ubuntu)"](https://msdn.microsoft.com/en-gb/commandline/wsl/install_guide) is installed.
* browse to the tools directory and double click on ```setup-wsfl.cmd```.

On Ubuntu, browse to the tools directory and run the command ```$ chmod +x ./setup-linux.sh;source ./setup-linux.sh```


### Build `Altra64`

To build the Rom

from the projects root directory,
On Windows 10 run 
```
> build
```
on linux
```
$ make
```
If it all worked, you will find `OS64.v64` in the `Altra64` bin directory.

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
If it all worked, you will find `OS64.v64` in the `Altra64` bin directory.


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
