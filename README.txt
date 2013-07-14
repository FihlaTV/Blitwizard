
- Blitwizard README -

Blitwizard is a 2d engine that runs Lua scripts which implement
video games or multimedia applications.

LICENSE:

  The license of blitwizard is zlib (open e.g. src/main.c and read at the top).
  The examples are public domain including graphics and the blitwizard logo,
  which was designed by uoou - thanks :-)

  Blitwizard uses various libraries. Check their licenses in README-libs.txt
  
HOW TO USE:

  Run blitwizard to see a few example applications:

  * Run on WINDOWS:
    Double-click "Run-blitwizard.bat"

  * Run on LINUX/BSD/MAC:
    When using a local compiled copy (e.g. the linux build from the website):
    Open a terminal, change directory to the extracted blitwizard main directory:
        cd /path/to/blitwizard/files/
    Then type this to run: cd bin/ && chmod +x ./blitwizard && ./blitwizard
    When using a system-wide installation (possible with an own source build
    and "sudo make install" after completing the "make" step):
    Open a terminal and simply type: blitwizard

  If you don't have a binary release, you need to build blitwizard first.
  Please check out README-build.txt for this.
   (if there is no such file, you have a binary release)

LEARN BLITWIZARD:

You can check out the example source code by opening the .lua files
of the examples which are located in examples/ in separate sub
folders. Feel free to modify those .lua files, then rerun the sample
browser and see how the examples changed!

Check out docs/index.html for acomplete documentation of the
provided functionality of blitwizard for advanced usage.
(That file is only present for blitwizard releases, not for dev code)

To start your own game project, copy all contents of the folder where
this README.txt is inside into a new empty folder named after your game
(e.g. a folder named "mygame"), then add a new game.lua into your new
game folder (you can write it with any text editor, e.g. Notepad) and
simply double-click "Run-Blitwizard.bat" inside your game folder to
run it.
(On Windows, for other systems simply run the appropriate blitwizard
binary from the bin folder).

To ship your game project with just the required files, check out
Ship-your-game.txt.

MORE AUDIO FORMATS THROUGH FFMPEG:

Blitwizard supports FFmpeg, however it does not ship with it.
FFmpeg supports a wide range of multimedia formats (e.g. mp3).

However, many other programs install FFmpeg and blitwizard does
automatically attempt to load and locate it if that is the case.

A possibly incomplete list of places checked for FFmpeg:
 - Valve's Steam usually comes with FFmpeg which
   blitwizard attempts to detect and load (on Windows)
 - Blender also comes with FFmpeg which blitwizard should
   find (on Windows)
 - On Linux, /usr/lib, /usr/lib64/ and other places are scanned
   for a system-wide FFmpeg installation

When FFmpeg is found and loaded, the additional formats will
be automatically available to your blitwizard program.
Otherwise, only playback for .ogg and .flac will be available.

