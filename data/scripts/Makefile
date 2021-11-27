.POSIX:

MK = ../../mk

include $(MK)/config.mk

SCRIPTS = pekwm_themeset.sh pekwm_ws_menu.sh pekwm_theme

all: $(SCRIPTS)

install:
	mkdir -p $(DESTDIR)$(PREFIX)/share/pekwm/scripts
	cp pekwm_themeset.sh pekwm_ws_menu.sh \
	   $(DESTDIR)$(PREFIX)/share/pekwm/scripts
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp pekwm_theme \
	   $(DESTDIR)$(PREFIX)/bin

pekwm_themeset.sh: pekwm_themeset.sh.in
	$(SED) -e 's@\@SH\@@$(SH)@g' \
	    -e 's@\@SED\@@$(SED)@g' \
	    pekwm_themeset.sh.in > pekwm_themeset.sh
	chmod +x pekwm_themeset.sh

pekwm_ws_menu.sh: pekwm_ws_menu.sh.in
	$(SED) -e 's@\@SH\@@$(SH)@g' \
	    pekwm_ws_menu.sh.in > pekwm_ws_menu.sh
	chmod +x pekwm_ws_menu.sh

pekwm_theme: pekwm_theme.in
	$(SED) -e 's@\@SH\@@$(SH)@g' \
	    -e 's@\@AWK\@@$(AWK)@g' \
	    -e 's@\@CMAKE_INSTALL_PREFIX\@/@$(PREFIX)@g' \
	    -e 's@\@PEKWM_THEMES\@@$(PEKWM_THEMES)@g' \
	    pekwm_theme.in > pekwm_theme
	chmod +x pekwm_theme

clean:
	rm -f *~ $(SCRIPTS) || true
