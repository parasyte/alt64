# Altra64

Alternative Everdrive64 menu

`Altra64` is an open source menu for [Everdrive64](http://krikzz.com/) and ed64+ and is based on a fork of alt64 which was
originally written by saturnu, and released on the
[Everdrive64 forum](http://krikzz.com/forum/index.php?topic=816.0).

## Reason for this fork

The original version overwrote 1 save file per game on system reset, but if you (or your kids...) accidentally turn it off, or hit reset twice, you just lost your entire game progress forever.  I have modified the saving/loading to *never* overwrite a save file, and instead save `gamename.0000.SRM`, then `gamename.0001.SRM` next and so on, up to `gamename.9999.SRM`, so that absolute worst case if you mess up you only lose 1 gaming session's save. If need be you can put the SD card into a computer and delete the latest faulty save.  Upon starting a game it'll always load the highest numbered save.

## Building

If you want to build the menu, you need an n64 toolchain. This is terrible to build, I ended up creating a Dockerfile in the docker folder, instructions included in it.

Or if you trust me, you can use the one I built and pushed to docker hub, [moparisthebest/altra64-dev](https://hub.docker.com/r/moparisthebest/altra64-dev)


### Build `Altra64`

To build the Rom

from the projects root directory, with docker installed
```
$ docker run --rm -v "$(pwd):/build" moparisthebest/altra64-dev make
```
If it all worked, you will find `OS64.v64` in the `bin` directory.


### Clean `Altra64`

Finally, we can clean the build objects from the project

from the projects root directory
```
$ docker run --rm -v "$(pwd):/build" moparisthebest/altra64-dev make clean
```

Enjoy!
