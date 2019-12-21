# vdpau-va-driver-vp9
Experimental VP9 codec support for vdpau-va-driver (NVIDIA VDPAU-VAAPI wrapper) and chromium-vaapi.

Forked off Ubuntu vdpau-va-driver (which includes some patches over the original freedesktop code):
https://launchpad.net/~saiarcot895/+archive/ubuntu/chromium-beta/+sourcepub/10036592/+listing-archive-extra

C associative map taken from (and modified to use integer keys) [sorry, the modifications are really makeshift]:
https://github.com/rxi/map

# Explanation

This adds experimental NVIDIA hardware video acceleration support to vdpau-va-driver for videos encoded using VP9 Profile 0 8-bit color depth. This seems to include most of the latest and greatest 4k and 8k videos I've used on YouTube that are hardest on the CPU, also available at lower resolutions in the same codec. As of writing, VP9 Profile 1-3 are not supported in the NVIDIA VDPAU library itself, so support for those cannot be added here either. VP8 also is not supported.

To check whether your GPU can decode VP9, please check the NVDEC support matrix. I believe that this should match the VDPAU capabilities. The only exception is that VDPAU supports only Profile 0, so don't expect support for Profile 1-3 even if your GPU supports them in NVDEC. Also, VDPAU does not support VP8, H.265, or AV1 and may have other limitations.

https://developer.nvidia.com/video-encode-decode-gpu-support-matrix

This patch has only been tested on Ubuntu 19.10 (Linux), so I'm really not sure if it works on anything else but "may". In my experience the patchsets vary per distro and some just didn't work at all on my Ubuntu, so keep this in mind when trying the patch. However, my modifications themselves should not be distro-specific.

Note that 8k currently may not work properly, but 4k and below should work fine. This is based on my testing with a GTX 1060. 8k seems to allocate too many surfaces and run out of resources. I'm currently investigating if there's any way to remedy that, but will likely require modifications to chromium-vaapi.

# Original author of vdpau-va-driver

https://github.com/freedesktop/vdpau-driver

  libva-vdpau-driver
  A VDPAU-based backend for VA-API

  Copyright (C) 2009-2011 Splitted-Desktop Systems

# Requirements

I recommend following this guide. https://www.linuxuprising.com/2018/08/how-to-enable-hardware-accelerated.html
That's how I started. It includes information on everything you need, except this driver. Following the guide completely up to step 3 and then installing this driver will be enough if you're not sure what to do. If you know what you're doing, then you should skip installing vdpau-va-driver from that guide. Also, don't install h264ify if you're planning on using VP9.

1. GPU with VP9 NVDEC decode support: https://developer.nvidia.com/video-encode-decode-gpu-support-matrix
2. NVIDIA Linux video driver (only tested with 440.44 so far; not sure what the minimum version is for VP9 support)

   vdpauinfo will not report VP9 support, unless you have a patched version of it (unsure if this is available yet). Use nvidia-settings instead. It should show "VP9" under X Screen/VDPAU Information/Base Information
   
   ![NVIDIA VDPAU Settings VP9](doc/img/nvidia-settings-vp9.png "NVIDIA VDPAU Settings VP9")

3. Chromium with VAAPI patch (NOT Google Chrome)
4. Latest headers for VDPAU with VP9 patch (for compiling this patch): https://gitlab.freedesktop.org/ManojBonda/libvdpau
   This requires meson to compile.
   
        $ git clone https://gitlab.freedesktop.org/ManojBonda/libvdpau.git
        $ cd libvdpau
        $ meson -Dprefix=/usr build
        $ ninja -C build
        $ sudo ninja -C build install
   
   Messages (output should probably be /usr/include/vdpau and /usr/lib/x86_64-linux-gnu if on 64-bit):
   ```
   Installing subdir /tmp/libvdpau/include/vdpau to /usr/include/vdpau
   Installing /tmp/libvdpau/include/vdpau/vdpau_x11.h to /usr/include/vdpau
   Installing /tmp/libvdpau/include/vdpau/vdpau.h to /usr/include/vdpau
   Installing src/libvdpau.so.1.0.0 to /usr/lib/x86_64-linux-gnu
   Installing trace/libvdpau_trace.so.1.0.0 to /usr/lib/x86_64-linux-gnu/vdpau
   Installing /tmp/libvdpau/src/vdpau_wrapper.cfg to /etc
   Installing /tmp/libvdpau/build/meson-private/vdpau.pc to /usr/lib/x86_64-linux-gnu/pkgconfig
   ```

# Unimplemented Features

1. Loop filter
2. Segmentation (segmentFeatureMode) and some segmentation features
3. modeRefLfEnabled, mbRefLfDelta, mbModeLfDelta

Most of these are not implemented well or at all due to either: 1) VA-API not providing "raw" enough information from the VP9 headers, or 2) Me not knowing how to use the information that VA-API gives in order to achieve them. Any help for those with far more experience in video codecs is greatly appreciated; my experience with video codecs, VDPAU, and VA-API is all extremely limited.

# Limitations and Disclaimers

Don't expect a lot. This may crash, hang, just not work at all, or result in video artifacts. This is extremely experimental, and roughly half of the VP9 codec probably isn't even implemented at all. That's denoted by 'XXX' comments in the code. (It seems that the other codecs implemented in the wrapper may also be missing some features.)

That being said, based on my testing I haven't found significant video artifacts or all-out crashes yet. It appears that most videos may not use the features that aren't implemented or maybe they just don't make enough of a difference visually, or I just haven't noticed yet; I'm really not sure, but I'm just providing this in good faith because it "seems to work". I would appreciate bug reports, contributions, etc. though and will try to look at them when I have time.

# Future Work

1. Getting this patch to work well enough
2. Getting 8k video HW accel to work, because it seems to work on Windows
3. Most likely out of the scope of VDPAU, but for hardware acceleration on Linux/web browsers in general: VP9 Profiles 1-3, AV1, HEVC H.265, more native support for VDPAU/Cuvid/NVDEC/NVENC in Chromium, Firefox, etc, etc...
4. Stretch goal: achieving HW accel feature parity with Windows on Chromium or Firefox on Linux distributions officially

# Compiling

Please check the Requirements section, or else nothing below will work. Also, don't forget to use the latest VDPAU headers with VP9 support. There may be other dependencies depending on your distro.

    $ git clone https://github.com/xtknight/vdpau-va-driver-vp9.git
    $ cd vdpau-va-driver-vp9
    $ ./autogen.sh --prefix=/usr
    $ make
    $ sudo make install

Messages (output should probably be /usr/lib/x86_64-linux-gnu/dri if on 64-bit):
```
----------------------------------------------------------------------
Libraries have been installed in:
   /usr/lib/x86_64-linux-gnu/dri

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the '-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the 'LD_RUN_PATH' environment variable
     during linking
   - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to '/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
----------------------------------------------------------------------
make  install-data-hook
make[3]: Entering directory '/home/andy/dev/vdpau-va-driver-vp9/src'
cd /usr/lib/x86_64-linux-gnu/dri ;			\
for drv in nvidia s3g ; do				\
            rm -f ${drv}_drv_video.so;                         \
    ln -s vdpau_drv_video.so ${drv}_drv_video.so;	\
done
```

# Using

Launch chromium-browser from chromium-vaapi package without using extensions like h264ify and try some 4k videos. It works with lower resolutions too but many CPUs are already fast enough to decode 1080p so it would be hard to notice. It may also work with 8k depending on your card and/or setup.

Right click the YouTube video and click 'Stats for Nerds' to ensure codec starts with vp09.00. (VP9 profile 0). Here's an example 4k@60fps video in VP9 (as of writing): https://www.youtube.com/watch?v=aqz-KE-bpKQ

Depending on how fast your CPU is, it may actually hard to *prove* that the GPU is being used. Although those with slower CPUs will probably notice a lot easier. There's still going to be some CPU usage, so don't be surprised. Exactly why, I'm not sure, but may be related to issues in the frame handling that can be optimized. And I need to do more testing to see how much of a problem that really is. Based on nvidia-smi, my GTX 1060 seems to be around 25-40% usage for 4k@60fps if the video is playing and only around 15% if the video is paused. For me CPU usage is reduced from around ~450% (multi-core) to around ~50-100% if I disable HW accel in Chromium through the flags. It's a big improvement, at least, the difference between potentially being able to play a video smoothly without dropped frames and not being able to do so.

# Debugging

Executing these commands in the shell (terminal) and then running chromium-browser from the same shell will activate them.

## vdpau-va-driver
Verbose debug messages
```
export VDPAU_VIDEO_DEBUG=1
```

More detailed trace messages that output data structures
```
export VDPAU_VIDEO_TRACE=1
```

## NVIDIA VDPAU
Trace all function calls made to VDPAU library and dump most parameters

https://download.nvidia.com/XFree86/Linux-x86_64/440.44/README/vdpausupport.html

VDPAU_TRACE: Enables tracing. Set to 1 to trace function calls. Set to 2 to trace all arguments passed to the function.
VDPAU_TRACE_FILE: Filename to write traces to. By default, traces are sent to stderr. This variable may either contain a plain filename, or a reference to an existing open file-descriptor in the format "&N" where N is the file descriptor number.

```
export VDPAU_TRACE=2
export VDPAU_NVIDIA_DEBUG=3
export VDPAU_TRACE_FILE=chromium-vdpau.log
```

## Source Code

The bulk of the initial support code is here: https://github.com/xtknight/vdpau-va-driver-vp9/commit/894fe2e1b0dedbf02dca72d0a2c632a41adc701b

Most heavy-lifting occurs in src/vdpau_decode.c

## Tips

Using VDPAU_VIDEO_DEBUG/VDPAU_VIDEO_TRACE and the dump_* functions in the source code is a good way to debug things.

I also compared VDPAU calls to ffmpeg output for testing (although keep in mind ffmpeg may also call get_bits yuv commands to get video surface data for encoding). This is just a simple command to convert the stream to a TS file, but in the process it uses VDPAU for decoding.

(Note: requires ffmpeg with VDPAU VP9 support! I had to compile the latest git revision manually)

```
rm -f /tmp/out1.ts
ffmpeg -hwaccel vdpau -i ~/Downloads/Profile_0_8bit/buf/crowd_run_1080X512_fr30_bd8_8buf_l3.webm -strict -1 /tmp/out1.ts
```

## Test Streams

1. VP9 test video pack: \
   a. https://www.webmproject.org/vp9/levels/#test-bitstreams \
   b. https://storage.googleapis.com/downloads.webmproject.org/vp9/decoder-test-streams/Profile_0_8bit.zip
   
2. VP9 8k test video: https://commons.wikimedia.org/wiki/File:First_8K_Video_from_Space_-_Ultra_HD_VP9.webm

## References

Most of this project was developed by cross-referencing these bits of source code and resources, along with lots of guessing.

1. http://git.videolan.org/?p=ffmpeg.git;a=blob;f=libavcodec/vp9.c;h=0eb92f8c08764c425c08c57393787b5c5d1d3808;hb=HEAD
2. https://github.com/FFmpeg/FFmpeg/blob/1054752c563cbe978f16010ed57dfa23a41ee490/libavcodec/vdpau_vp9.c
3. https://intel.github.io/libva/va__dec__vp9_8h_source.html
4. https://vdpau.pages.freedesktop.org/libvdpau/vdpau_8h_source.html
5. https://storage.googleapis.com/downloads.webmproject.org/docs/vp9/vp9-bitstream-specification-v0.6-20160331-draft.pdf
6. https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_uncompressed_header_parser.cc
7. https://chromium.googlesource.com/chromium/src/+/master/media/filters/vp9_compressed_header_parser.cc
8. https://download.nvidia.com/XFree86/Linux-x86_64/440.44/README/vdpausupport.html
9. https://github.com/Intel-Media-SDK/MediaSDK/blob/master/_studio/shared/umc/codec/vp9_dec/src/umc_vp9_bitstream.cpp
10. https://vdpau.pages.freedesktop.org/libvdpau/struct_vdp_picture_info_v_p9.html

# Screenshots

![Screenshot 1](doc/img/vp9_sample_1.png "Screenshot 1")
