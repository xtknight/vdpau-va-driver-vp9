# vdpau_video package version number
m4_define([vdpau_video_major_version], [0])
m4_define([vdpau_video_minor_version], [7])
m4_define([vdpau_video_micro_version], [4])
m4_define([vdpau_video_pre_version],   [0])
m4_define([vdpau_video_version],
          [vdpau_video_major_version.vdpau_video_minor_version.vdpau_video_micro_version])
m4_if(vdpau_video_pre_version, [0], [], [
m4_append([vdpau_video_version], vdpau_video_pre_version, [.pre])
])

# VA-API minimum version number
m4_define([va_api_x11_version], [0.31.0])
m4_define([va_api_glx_version], [0.32.0])

# libva package version number
m4_define([libva_package_x11_version], [1.0.3])
m4_define([libva_package_glx_version], [1.0.9])

AC_PREREQ([2.57])
AC_INIT([vdpau_video], [vdpau_video_version], [gwenole.beauchesne@intel.com],
        [libva-vdpau-driver])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE

AM_CONFIG_HEADER([src/config.h])

VDPAU_VIDEO_MAJOR_VERSION=vdpau_video_major_version
VDPAU_VIDEO_MINOR_VERSION=vdpau_video_minor_version
VDPAU_VIDEO_MICRO_VERSION=vdpau_video_micro_version
AC_DEFINE([VDPAU_VIDEO_MAJOR_VERSION], [vdpau_video_major_version], [Major version of the driver])
AC_DEFINE([VDPAU_VIDEO_MINOR_VERSION], [vdpau_video_minor_version], [Minor version of the driver])
AC_DEFINE([VDPAU_VIDEO_MICRO_VERSION], [vdpau_video_micro_version], [Micro version of the driver])
AC_DEFINE([VDPAU_VIDEO_PRE_VERSION], [vdpau_video_pre_version], [Preversion of the driver])

VDPAU_VIDEO_LT_LDFLAGS="-avoid-version"
AC_SUBST(VDPAU_VIDEO_LT_LDFLAGS)

AC_DEFUN([AC_DEFINE_01], [
    if test "$2" = "yes" -o "$2" = "guessing yes"; then
        AC_DEFINE($1, 1, $3)
    else
        AC_DEFINE($1, 0, $3)
    fi
])

AC_ARG_ENABLE(glx,
    AC_HELP_STRING([--enable-glx],
                   [enable OpenGL/X11 @<:@default=yes@:>@]),
    [], [enable_glx="yes"])

AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],
                   [enable debugging information]),
    [], [enable_debug="yes"])

AC_DEFINE_01(USE_DEBUG, "$enable_debug",
    [Enable debugging information])

AC_ARG_ENABLE(tracer,
    AC_HELP_STRING([--enable-tracer],
                   [enable VDPAU tracer]),
    [], [enable_tracer="yes"])

AC_DEFINE_01(USE_TRACER, "$enable_tracer",
    [Enable VDPAU tracer])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AC_C_BIGENDIAN
AC_HEADER_STDC
AC_SYS_LARGEFILE

dnl Use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [
    AM_DEFAULT_VERBOSITY=1
    AC_SUBST(AM_DEFAULT_VERBOSITY)
])

dnl We want pthreads. Try libpthread first, then libc_r (FreeBSD), then PTL.
HAVE_PTHREADS=yes
AC_CHECK_LIB(pthread, pthread_create, , [
    AC_CHECK_LIB(c_r, pthread_create, , [
        AC_CHECK_LIB(PTL, pthread_create, , [
            HAVE_PTHREADS=no
        ])
    ])
])
if test "x$HAVE_PTHREADS" = "xyes"; then
    AC_DEFINE(HAVE_PTHREADS, 1, [Defined if pthreads are available])
fi

dnl Checks for libraries.
AC_CHECK_LIB(m, tan)
AC_CHECK_LIB(rt, timer_create)

dnl Checks for library functions.
AC_CHECK_FUNCS(clock_gettime)

dnl Check for __attribute__((visibility()))
AC_CACHE_CHECK([whether __attribute__((visibility())) is supported],
  vdpau_cv_visibility_attribute,
  [cat > conftest.c <<EOF
int foo __attribute__ ((visibility ("hidden"))) = 1;
int bar __attribute__ ((visibility ("protected"))) = 1;
EOF
  vdpau_cv_visibility_attribute=no
  if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then
    if grep '\.hidden.*foo' conftest.s >/dev/null; then
      if grep '\.protected.*bar' conftest.s >/dev/null; then
	vdpau_cv_visibility_attribute=yes
      fi
    fi
  fi
  rm -f conftest.[cs]
])
if test $vdpau_cv_visibility_attribute = yes; then
  AC_DEFINE([HAVE_VISIBILITY_ATTRIBUTE], 1, [Define to use the __attribute__((visibility())) extension])
fi

dnl Check for OpenGL
USE_GLX=1
if test "$enable_glx" != "yes"; then
    USE_GLX=0
fi
GL_DEPS_CFLAGS=""
GL_DEPS_LIBS=""
AC_CHECK_HEADERS([GL/gl.h GL/glext.h GL/glx.h], [], [USE_GLX=0], [
#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#endif
])
AC_CHECK_LIB(GL, glXCreateContext, [GL_DEPS_LIBS="-lGL -ldl"], [USE_GLX=0])
AC_SUBST(GL_DEPS_CFLAGS)
AC_SUBST(GL_DEPS_LIBS)

dnl Check for VA-API
PKG_CHECK_MODULES(LIBVA_DEPS,	  [libva])
PKG_CHECK_MODULES(LIBVA_X11_DEPS, [libva-x11 >= va_api_x11_version],
    [LIBVA_PACKAGE_VERSION=libva_package_x11_version])
PKG_CHECK_MODULES(LIBVA_GLX_DEPS, [libva-glx >= va_api_glx_version],
    [LIBVA_PACKAGE_VERSION=libva_package_glx_version], [USE_GLX=0])

VA_VERSION=`$PKG_CONFIG --modversion libva`
VA_MAJOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f1`
VA_MINOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f2`
VA_MICRO_VERSION=`echo "$VA_VERSION" | cut -d'.' -f3`
VA_VERSION_STR="$VA_VERSION"

va_full_version_int=`expr ${VA_MAJOR_VERSION:-0} "*" 1000000 + \
                          ${VA_MINOR_VERSION:-0} "*" 10000   + \
                          ${VA_MICRO_VERSION:-0} "*" 100     + \
                          0`
VA_DRIVER_INIT_FUNC="__vaDriverInit_${VA_MAJOR_VERSION}_${VA_MINOR_VERSION}"
AC_DEFINE_UNQUOTED([VA_DRIVER_INIT_FUNC], [$VA_DRIVER_INIT_FUNC],
    [Define driver entry-point])

dnl Check for VA-API drivers path
AC_MSG_CHECKING([for VA drivers path])
LIBVA_DRIVERS_PATH=`$PKG_CONFIG libva --variable driverdir`
if test -z "$LIBVA_DRIVERS_PATH"; then
    LIBVA_DRIVERS_PATH="/usr/lib/xorg/modules/drivers"
fi
AC_MSG_RESULT([$LIBVA_DRIVERS_PATH])
AC_SUBST(LIBVA_DRIVERS_PATH)

dnl Check for VDPAU
AC_CACHE_CHECK([for VDPAU],
    ac_cv_have_vdpau, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -lvdpau"
    AC_TRY_LINK(
    [#include <vdpau/vdpau_x11.h>],
    [vdp_device_create_x11(0,0,0,0)],
    [ac_cv_have_vdpau="yes" HAVE_VDPAU=1],
    [ac_cv_have_vdpau="no"  HAVE_VDPAU=0])
])
if test "$ac_cv_have_vdpau" = "no"; then
    AC_MSG_ERROR([you need VDPAU to build this package])
fi

AC_CACHE_CHECK([for VDPAU/MPEG-4 support],
    ac_cv_have_vdpau_mpeg4, [
    AC_TRY_LINK(
    [#include <vdpau/vdpau.h>],
    [VdpPictureInfoMPEG4Part2 pic_info],
    [ac_cv_have_vdpau_mpeg4="yes" HAVE_VDPAU_MPEG4=1],
    [ac_cv_have_vdpau_mpeg4="no"  HAVE_VDPAU_MPEG4=0])
])
AC_DEFINE_UNQUOTED(HAVE_VDPAU_MPEG4, [$HAVE_VDPAU_MPEG4], [VDPAU/MPEG-4 support])

dnl Check for VDPAU version (poor man's check, no pkgconfig joy)
VDPAU_VERSION=`cat << EOF | $CC -E - | grep ^vdpau_version | cut -d' ' -f2
#include <vdpau/vdpau.h>
vdpau_version VDPAU_VERSION
EOF
`

AC_DEFINE_UNQUOTED(USE_GLX, $USE_GLX, [Defined to 1 if GLX is enabled])
AM_CONDITIONAL(USE_GLX, test $USE_GLX -eq 1)

VDPAU_VIDEO_CFLAGS="$LIBVA_DEPS_CFLAGS"
VDPAU_VIDEO_LIBS=""
if test $USE_GLX -eq 1; then
    VDPAU_VIDEO_CFLAGS="$VDPAU_VIDEO_CFLAGS $GL_DEPS_CFLAGS"
    VDPAU_VIDEO_LIBS="$VDPAU_VIDEO_LIBS $GL_DEPS_LIBS"
fi
AC_SUBST(VDPAU_VIDEO_CFLAGS)
AC_SUBST(VDPAU_VIDEO_LIBS)

AC_SUBST(LIBVA_PACKAGE_VERSION)

AC_OUTPUT([
    Makefile
    debian.upstream/Makefile
    src/Makefile
])

dnl Print summary
echo
echo $PACKAGE configuration summary:
echo
echo VA-API version ................... : $VA_VERSION_STR
echo VA-API drivers path .............. : $LIBVA_DRIVERS_PATH
echo VDPAU version .................... : $VDPAU_VERSION
echo VDPAU/MPEG-4 support ............. : $(test $HAVE_VDPAU_MPEG4  -eq 1 && echo yes || echo no)
echo GLX support ...................... : $(test $USE_GLX  -eq 1 && echo yes || echo no)
echo
