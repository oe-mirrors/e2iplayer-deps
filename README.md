# lsdir
Allows to list directory

# cmdwrap
Command line args splitter

# e2isubparser
Python module for text subtitles parsing

# hlsdl
==============

This program converts .m3u8 playlists (using fragmented MPEG-2 Transport Streams) to a .ts video. It supports decryption of both AES-128 and SAMPLE-AES encryption.

Requirements
------------

This program requires libcurl and libcrypto libraries.

Build
-----
Linux:
`make && make install && make clean`

Windows:
https://github.com/samsamsam-iptvplayer/hlsdl/blob/master/msvc/BUID_WINDOWS.txt


Usage and Options
-------
`./hlsdl url [options]`

---------------------------
```
-b ... Automatically choose the best quality.

-v ... Verbose more information.

-o ... Choose name of output file ("-" alias for stdout).

-u ... Set custom HTTP User-Agent header.

-h ... Set custom HTTP header.

-p ... Set proxy uri.

-k ... Allow to replace part of AES key uri - old.

-n ... Allow to replace part of AES key uri - new.

-f ... Force overwriting the output file.

-q ... Print less to the console.

-d ... Print the openssl decryption command.

-t ... Print the links to the .ts files.

-s ... Set live start offset in seconds.

-e ... Set refresh delay in seconds.

-r ... Set max retries at open.

-w ... Set max download segment retries.

-a ... Set additional url to the audio media playlist.
```

ToDo
-----
* support for Fragmented MPEG-4 playlist
* support for EXT-X-MAP in the MPEG-2 Transport Streams playlist

# F4MDump
Info:
Simple F4M/HDS (Adobe HTTP Dynamic Streaming) dumper. Intended to be used by IPTVPlayer plugin: https://gitorious.org/iptv-pl-dla-openpli, http://iptvplayer.vline.pl.

For downloading data with URL syntax F4MDump do not use any library it use wget, called as child process using vfor/execvp.

Usage:
List available bitrates from manifest file: F4MDump "/path/to/wget with additional params if needed" "http://url.to/Manifest.f4m"

Downloading: F4MDump "/path/to/wget with additional params if needed" "http://url.to/Manifest.f4m" "/path/to/out/file" [bitrate]

