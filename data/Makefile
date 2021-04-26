.POSIX:

MK = ../mk

include $(MK)/config.mk

all:
	( cd scripts; $(MAKE) all )

install:
	mkdir -p $(DESTDIR)$(PREFIX)/etc/pekwm
	cp autoproperties \
	   autoproperties_typerules \
	   config \
	   config_system \
	   keys \
	   menu \
	   mouse \
	   mouse_system \
	   mouse_click \
	   mouse_sloppy \
	   panel \
	   start \
	   vars \
	   $(DESTDIR)$(PREFIX)/etc/pekwm
	( cd themes; $(MAKE) install )
	( cd scripts; $(MAKE) install )

clean:
	( cd themes; $(MAKE) clean )
	( cd scripts; $(MAKE) clean )
	rm -f *~ || true
