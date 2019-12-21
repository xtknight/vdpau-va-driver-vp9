DRIVERS  = nvidia
DRIVERS += s3g	# native VA-API driver is too old, so use VDPAU based one

INCLUDES = \
	$(VDPAU_VIDEO_CFLAGS)

LDADD = \
	$(VDPAU_VIDEO_LT_LDFLAGS) -module \
	-no-undefined -module -Wl,--no-undefined

if USE_GLX
source_glx_h = vdpau_video_glx.h utils_glx.h
source_glx_c = vdpau_video_glx.c utils_glx.c
endif

source_x11_h = vdpau_video_x11.h utils_x11.h
source_x11_c = vdpau_video_x11.c utils_x11.c

source_h = \
	debug.h			\
	object_heap.h		\
	sysdeps.h		\
	uasyncqueue.h		\
	ulist.h			\
	uqueue.h		\
	utils.h			\
	vaapi_compat.h		\
	vdpau_buffer.h		\
	vdpau_decode.h		\
	vdpau_driver.h		\
	vdpau_driver_template.h	\
	vdpau_dump.h		\
	vdpau_gate.h		\
	vdpau_image.h		\
	vdpau_mixer.h		\
	vdpau_subpic.h		\
	vdpau_video.h		\
	$(source_glx_h)		\
	$(source_x11_h)

source_c = \
	debug.c			\
	object_heap.c		\
	put_bits.h		\
	uasyncqueue.c		\
	ulist.c			\
	uqueue.c		\
	utils.c			\
	vdpau_buffer.c		\
	vdpau_decode.c		\
	vdpau_driver.c		\
	vdpau_dump.c		\
	vdpau_gate.c		\
	vdpau_image.c		\
	vdpau_mixer.c		\
	vdpau_subpic.c		\
	vdpau_video.c		\
	$(source_glx_c)		\
	$(source_x11_c)

vdpau_drv_video_la_LTLIBRARIES	= vdpau_drv_video.la
vdpau_drv_video_ladir		= @LIBVA_DRIVERS_PATH@
vdpau_drv_video_la_SOURCES	= $(source_c)
vdpau_drv_video_la_LIBADD	= $(VDPAU_VIDEO_LIBS) -lX11
vdpau_drv_video_la_LDFLAGS	= $(LDADD)

noinst_HEADERS = $(source_h)

EXTRA_DIST = \
	$(source_glx_c) \
	$(source_glx_h)	\
	$(source_x11_c)	\
	$(source_x11_h)

install-data-hook:
	cd $(DESTDIR)$(LIBVA_DRIVERS_PATH) ;			\
	for drv in $(DRIVERS); do				\
	    ln -s vdpau_drv_video.so $${drv}_drv_video.so;	\
	done

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in
