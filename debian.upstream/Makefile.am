DEBIANFILES = \
	changelog.in			\
	compat				\
	control.in			\
	copyright			\
	libva-vdpau-driver.install.in	\
	rules				\
	$(NULL)

DEBIANGENFILES = \
	changelog			\
	control				\
	libva-vdpau-driver.install	\
	$(NULL)

EXTRA_DIST = $(DEBIANFILES)

dist_noinst_DATA = $(DEBIANGENFILES)

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in $(DEBIANGENFILES)

$(DEBIANGENFILES): %: %.in Makefile
	-sed \
	  -e 's|\@PACKAGE_VERSION\@|$(PACKAGE_VERSION)|' \
	  -e 's|\@LIBVA_PACKAGE_VERSION\@|$(LIBVA_PACKAGE_VERSION)|' \
	  -e 's|\@LIBVA_DRIVERS_PATH\@|$(LIBVA_DRIVERS_PATH)|' \
	  -e 's|\@DATE\@|'"`LC_ALL=C date +'%a, %d %b %Y %X %z'`"'|' \
	  $< > $@
