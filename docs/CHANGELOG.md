## Change log

### 0.36.0-dev.1 (20251201)
* Switch submodule build system to meson (only x64 available)
* Update avisynth(plus) headers to v12
* Unicode-safe file reading
* Add frame size parameters
* Add set_default_storage_size boolean
* Update libass to 0.17.4

### 0.35 (20210304)
* Windows MSVC: Update to libass v0.15
  (git submodule update --init --recursive --remote)
  For changes since v0.14 see https://github.com/libass/libass/blob/master/Changelog
* don't guess base on video resolution (realfinder)
  if .ass file has no Matrix info then it should be treated as it "Rec601" to maintain compatibility
* Parameter 'colorspace' default value is no longer "guess"
* Add more color options: PC.709, PC.601, TV.fcc, PC.fcc, TV.240m, PC.240m, none.
  "none" and "guess" implies "guess-by-resolution".
* Fix: possible crash on initializing phase (buffer overread, linux crashed, Windows was just lucky)

### 0.34 (20210301)
* Fix the fix: revert matrix change made in 0.33	
* Fix: Check matrix from .ASS file "YCbCr Matrix:" section besides "Video Colorspace:"
  Recognized values are "tv.601" and "tv.709"

### 0.33 (20210228)
* Fix: wrong Bt.709 matrix (it wasn't :) )

### 0.32 (20210227)
* Fix: treat I420 variant as YV12 instead of unsupported color space

### 0.31 (20210218)
* Fix colors for planar RGB
* code: hidden ifdef FOR_AVISYNTH_26_ONLY for Avisynth 2.6-only build

### 0.30 (20210217)
* From now assrender does not works with classic Aviysnth: high-bitdepth helper function calls
* 10-16 bit support (including RGB48 and RGB64)
* YV411, Planar RGB support

### 0.29 (20210216 - pinterf)
* project moved to https://github.com/pinterf/assrender from https://github.com/vadosnaprimer/assrender
* Move to Visual Studio 2019 - v142 platform toolset
* Add .def module definition file for Avisynth 2.6 std compatibility (function name mangling)
* Update Avisynth C headers
* Check Linux and gcc-MinGW CMake build
* Add build instructions and change log to README

### no version (20190114 - vadosnaprimer)
* https://github.com/vadosnaprimer/assrender/
* add batch that lets not to change deps sdk and vs version copied from SMP libass
* update SMP submodules

### no version (20161018 - Blitzker)
* https://github.com/Blitzker/assrender
* Visual Studio 2015 support

### 0.28 (20120226 - pingplug)
* small changes and update version to 0.28

### 0.27 (20150202 - pingplug)
* https://github.com/pingplug/assrender
* add a simple .rc file :-)
* cache the last img to rend faster if img not changed 
* add YUY2 and YV16 support

### 0.25 (20120420 - lachs0r)
* moved to github
* code restructured
* added support for the BT.709 color space and the 'Video Colorspace' property that has been introduced with recent versions of Aegisub.
* binary:
  - updated everything, switched to MinGW-w64 (same toolchain as mplayer2 now)

### 0.24.1 (20110922 - lachs0r)
* binary changes only
* binary:
  - updated libass to current git HEAD
  - switched Harfbuzz to libass’ compatibility branch  
  - compiled Harfbuzz without Uniscribe backend
* fixes lots of crashes and misbehavior

### 0.24 (20110729)
* fixing the performance regression

### 0.23 (20110728)
* disabled font hinting by default
* binary:
 - updated libass to current git HEAD and included Harfbuzz:
 - added support for bidirectional text, Arabic shaping etc.
 - added proper support for @fonts (vertical writing)
 - slight performance regression (glyph cache not hooked up with Harfbuzz yet)
 - updated FreeType to current git HEAD:
 - fixed outline stroker for some broken fonts

### 0.22 (20110618 - lachs0r) 
* fixed that annoying hang on vector clips

### 0.21 (20110608 - lachs0r) 
* fixed YV12 subsampling so it no longer looks horrible, which should be rather good news for most users.
* temporarily removed YV16 support
* renamed parameter verbosity → debuglevel
* code cleanups
* binary:
  - reverted to GCC 4.5.2 (4.6 miscompiles MinGW)

### 0.20 (20110601)
* fixed RGB32 support (it’s actually usable with BlankClip(pixel_type="RGB32") now).
* fixed the masksub stuff
* properly output debug messages to stderr instead of stdout
* reformatted source and corrected/removed some comments
* modified CMakeLists.txt to strip the binary by default
* binary:
  - now built with GCC 4.6 instead of 4.5.2
  - included enca again
  - patched fontconfig:
    - prettier debug output
    - use the correct location for its cache

### 0.19 (2011-02-01 - lachs0r)
This is a bugfix/cleanup release.
* fixed possible buffer overflows in timecodesv1 and SRT parsing
* fixed random crashes on unloading
* probably fixed compilation with MSVC (patch by TheFluff)
* very slightly improved performance with GCC
* various code cleanups

### 0.16 (2011-01-25 - lachs0r)
* improved YV12 support (should be somewhat usable now)
* added support for RGB24, YV24, YV16 and Y8 (YUY2 coming soon)
* added SRT subtitle format support, additional parameter: srt_font (font to use for srt subs)
* exposed some libass parameters:
  - line_spacing (line spacing)
  - dar, sar (aspect ratio)
  - top, bottom, left, right (margins)
  - fontdir (additional font directory)
* masksub equivalent if used on a blankclip
  (still buggy - read source for details)
* no more global variables

### 0.16 (2011-01-17 - lachs0r)
* added rudimentary YV12 support (chroma subsampling still needs work)
* binary: Previously, I linked against a very old avisynth_c.lib 
  now you shouldn’t get any error messages about "avisynth_c.dll"
* tidied up the RGB32 blitter a bit

### 0.16 (2011-01-16)
* implemented VFRaC support via timecodes files (v1 and v2 supported)

### 0.15 (2011-01-16 - lachs0r)
* reimplemented as AviSynth C plugin - this fixed several crashes and
  got rid of the major pain in the ass that is MSVC
* binary: built with patched Fontconfig (no longer needs fonts.conf)

### 0.11 TheFluff
* Source code (under MIT license, binaries are under GPL for obvious reasons): assrender_0.11-src.7z
