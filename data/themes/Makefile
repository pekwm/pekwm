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

clean:
	rm -f themes/default/*~ themes/default-plain/*~ || true
