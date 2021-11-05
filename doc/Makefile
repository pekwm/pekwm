.POSIX:

MK = ../mk

include $(MK)/config.mk

all:

install:
	mkdir -p $(DESTDIR)$(PREFIX)/$(MANDIR)/man1
	cp *.1 $(DESTDIR)$(PREFIX)/$(MANDIR)/man1

clean:
	rm -f *~ || true
