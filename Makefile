# prefix for non-VDR stuff
PREFIX ?= /usr/local
# VDR's plugin conf directory
VDRPLUGINCONFDIR ?= /video/plugins

VERSION := $(shell grep VERSION src/libwebvi/webvi/version.py | cut -d \' -f 2)

TMPDIR = /tmp
ARCHIVE = webvideo-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

# The following two commented lines cause the main VDR Makefile to
# consider this as a plugin Makefile:
#PKGCFG
#$(LIBDIR)/$^.$(APIVERSION)

all: libwebvi vdr-plugin

vdr-plugin: libwebvi
	$(MAKE) -C src/vdr-plugin

libwebvi: build-python
	$(MAKE) -C src/libwebvi all libwebvi.a

build-python: webvi.conf
	python setup.py build

webvi.conf webvi.plugin.conf: %.conf: examples/%.conf
	sed 's_templatepath = /usr/local/share/webvi/templates_templatepath = $(PREFIX)/share/webvi/templates_g' < $< > $@

install-vdr-plugin:
	$(MAKE) -C src/vdr-plugin install

install-libwebvi: libwebvi
	$(MAKE) -C src/libwebvi install

install-python: uninstall-deprecated-templates
	python setup.py install --skip-build --prefix $(PREFIX) $${DESTDIR:+--root $(DESTDIR)}

install-conf: webvi.conf webvi.plugin.conf
	mkdir -p $(DESTDIR)/etc
	cp -f webvi.conf $(DESTDIR)/etc
	mkdir -p $(DESTDIR)$(VDRPLUGINCONFDIR)/webvideo
	cp -f webvi.plugin.conf $(DESTDIR)$(VDRPLUGINCONFDIR)/webvideo

install-webvi: install-libwebvi install-python

install: install-vdr-plugin install-webvi install-conf

# Template directories were renamed in 0.4.0. Remove old templates.
uninstall-deprecated-templates:
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/youtube
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/svtplay
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/moontv
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/metacafe
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/vimeo
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/katsomo
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/subtv
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/ruutufi
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/google
	rm -rf $(DESTDIR)$(PREFIX)/share/webvi/templates/yleareena

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

check:
	make -C src/unittest check

test: check

clean:
	$(MAKE) -C src/vdr-plugin clean
	$(MAKE) -C src/libwebvi clean
	$(MAKE) -C src/unittest clean
	rm -rf src/vdr-plugin/locale webvi.conf
	python setup.py clean -a
	find . -name "*~" -exec rm {} \;
	find . -name "*.pyc" -exec rm {} \;

.PHONY: vdr-plugin libwebvi build-python install install-vdr-plugin install-webvi dist clean check test
