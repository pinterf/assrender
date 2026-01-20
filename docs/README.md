# AssRender

AssRender is an AviSynth plugin that renders ASS/SSA and SRT (without the HTML-like markup) subtitles. It uses libass to render the subtitles, which makes it the fastest and most correct ASS renderer for AviSynth.

This also means that it is much more picky about script syntax than VSFilter and friends, so keep that in mind before blaming the filter. Yes, people have reported a lot of errors that were actually the script author’s fault.

## Usage

assrender(clip, string file, [string vfr, int hinting, float scale, float line_spacing, int frame_width, int frame_height, bool set_default_storage_size, float dar, float sar, int top, int bottom, int left, int right, string charset, int debuglevel, string fontdir, string srt_font, string colorspace])

`string file`

Your subtitle file. May be ASS, SSA or SRT.

`string vfr`

Specify timecodes v1 or v2 file when working with VFRaC.

`int hinting`

Font hinting mode. Choose between none (0, default), light (1), normal (2) and Freetype native (3) autohinting.

`float scale`

Font scale. Defaults to 1.0.

`float line_spacing`

Line spacing in pixels. Defaults to 1.0 and won’t be scaled with frame size.

`int frame_width`, `int frame_height`

 Actual displayed size, provide more information than `dar` & `sar`. Of course you need to set both parameters.

`float dar`, `float sar`

Aspect ratio, less priority than `frame_width` & `frame_height`. You need to set both parameters, too.

`set_default_storage_size`

Whether to render ASS according to storage size, default is True. 

`int top`, `int bottom`, `int left`, `int right`

Margins. They will be added to the frame size and may be negative.

`string charset`

Character set to use, in GNU iconv or enca format. Defaults to UTF-8.
Example enca format: `enca:pl:cp1250`
(guess the encoding for Polish, fall back on `cp1250`)

`int debuglevel`

How much crap assrender is supposed to spam to stderr.

`string fontdir`

Additional font directory.
Useful if you are lazy but want to keep your system fonts clean.
Default value: C:/Windows/Fonts or /usr/share/fonts (Windows and linux respectively)

`string srt_font`

Font to use for SRT subtitles.
Defaults to whatever Fontconfig chooses for “sans-serif”.

`string colorspace`

The color space of your (YUV) video. Possible values:
 - Rec2020, BT.2020
 - Rec709, BT.709
 - Rec601, BT.601
 - PC.709
 - PC.601
 - TV.fcc, PC.fcc
 - TV.240m, PC.240m
 - none, guess
 
Default is to use the ASS script's "YCbCr Matrix" or "Video Colorspace" property.

Recognized .ASS properties: "TV.601" "TV.709", "PC.601" "PC.709" "TV.240m" "PC.240m" "TV.fcc" "PC.fcc" and "none". 

"none" and "guess" determine the colorspace based on the video's `_Matrix` frame property if available. Otherwise, if the resolution is width > 1024 or height > 576, BT.709 is used; otherwise BT.601.

## Build instructions
See: [BUILD.md](BUILD.md)

## Change log
See: [CHANGELOG.md](CHANGELOG.md)

## Links
* Doom9 forum: https://forum.doom9.org/showthread.php?t=148926
* Avisynth wiki: http://avisynth.nl/index.php/AssRender
* libass: https://github.com/libass/libass
* Aegisub: https://github.com/TypesettingTools/Aegisub
