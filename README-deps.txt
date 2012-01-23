
---------------------
 Dependencies README
---------------------

This is the README for obtaining the dependencies for the source tarball
of blitwizard.
Getting the deps is also required on Unix/Linux!

Before you start, extract the blitwizard archive (where this README is in)
to a folder.
All relative paths specified are relative to that extracted folder.

Note on FFmpeg: blitwizard can use FFmpeg, but doesn't depend on it.
If you want to use FFmpeg, please check README-build.txt for detals.

== Dependencies ==

Either get the deps here: http://games.homeofjones.de/blitwizard/deps.zip
(extract into the blitwizard folder root here).

That file contains:
    SDL 1.3 hg cloned/fetched sometime in Jan 2012, PATCHED for rotation support
    libpng 1.5.6
    zlib 1.2.5
    libogg 1.3.0
    libvorbis 1.3.2
    Lua 5.2
	Box2D 2.2.1

WARNING: Those versions might be outdated and may contain bugs! Please
always check if there are newer versions available, and consider
using those instead:   

Alternatively, get them yourself in hand-picked, current versions:

 - drop the contents of a source tarball of a recent SDL 1.3 into
    src/sdl/
   see http://www.libsdl.org/hg.php
   WARNING: Blitwizard uses a patched SDL for rotation/flipping support.
     I expect SDL to adopt this natively, but at this point,
     you will have to patch this in manually.
     Check out the rotation.patch inside src/sdl/ inside deps.zip

 - drop the contents of a source tarball of a recent libogg release into
    src/ogg
   see http://xiph.org/downloads/

 - drop the contents of a source tarball of a recent libvorbis release into
    src/vorbis
   http://xiph.org/downloads/

 - drop the contents of a source tarball of a recent Lua 5.2 release into
    src/lua
   see http://www.lua.org/download.html
   (IMPORTANT: You should apply the patches aswell if you need a secure Lua!)

 - drop the contents of a source tarball of a recent libpng release into
    src/imgloader/png
   see http://libpng.org/pub/png/libpng.html

 - drop the contents of a source tarball of a recent zlib release into
    src/imgloader/zlib
   see http://zlib.net/

 - drop the contents of a source tarball of a recent Box2D release into
    src/box2d
   see http://box2d.org/

A source tarball is simply an archive that contains a folder with all
the source code of the product in it. If you get your tarball as .tar.gz
or .tar.bz2 (instead of .zip) and you are on Microsoft Windows, get
the archiver http://www.7-zip.org/ which can extract those.

Please note you should NOT drop the whole main folder inside
of a tarball into the directories, but just what the main folder
*contains* (so enter the main folder in your archiver, then
mark all the things inside, and extract those).

You should get a verbose error later if you did this
part wrong - so if you get errors about a "MISSING DEPENDENCY"
which you provided, check back on this.
