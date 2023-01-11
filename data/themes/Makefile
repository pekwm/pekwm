.POSIX:

MK = ../../mk

include $(MK)/config.mk

install:
	mkdir -p $(DESTDIR)$(PREFIX)/share/pekwm/themes
	mkdir -p $(DESTDIR)$(PREFIX)/share/pekwm/themes/default
	cp -R default/theme default/*.png default/backgrounds \
	   $(DESTDIR)$(PREFIX)/share/pekwm/themes/default
	mkdir -p $(DESTDIR)$(PREFIX)/share/pekwm/themes/default-plain
	cp default-plain/theme \
	   $(DESTDIR)$(PREFIX)/share/pekwm/themes/default-plain
	mkdir -p $(DESTDIR)$(PREFIX)/share/pekwm/themes/winter
	cp -R winter/theme winter/*.png winter/backgrounds \
	   $(DESTDIR)$(PREFIX)/share/pekwm/themes/winter

clean:
	rm -f themes/default/*~ themes/default-plain/*~ \
		themes/winter/*~ || true
