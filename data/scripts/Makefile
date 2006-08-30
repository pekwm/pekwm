# Makefile for pekwm
# Copyright (C) 2003 Claes Nästen <pekdon at pekdon dot net>

prefix = /usr/local
SCRIPTDIR = ${prefix}/share/pekwm/scripts
DATA_MODE = 755
INSTALL = /usr/bin/install -c
SCRIPTS = pekwm_themeset.pl pekwm_ws_menu.pl

install:
	mkdir -p $(DESTDIR)$(SCRIPTDIR)
	for i in $(SCRIPTS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(SCRIPTDIR); \
	done
