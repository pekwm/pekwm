MK = mk

include $(MK)/config.mk

all:
	( cd src; $(MAKE) all )
	( cd data; $(MAKE) all )
	( cd doc; $(MAKE) all )

install:
	( cd src; $(MAKE) install )
	( cd data; $(MAKE) install )
	( cd doc; $(MAKE) install )

dist:
	rm -f pekwm-$(VERSION).tar.gz
	mkdir -p pekwm-$(VERSION)
	mkdir -p pekwm-$(VERSION)/data
	mkdir -p pekwm-$(VERSION)/mk
	mkdir -p pekwm-$(VERSION)/src
	cp configure Makefile CMakeLists.txt pekwm-$(VERSION)/
	cp -R data/* pekwm-$(VERSION)/data/
	cp -R doc/* pekwm-$(VERSION)/data/
	cp -R mk/* pekwm-$(VERSION)/mk/
	cp -R src/Makefile pekwm-$(VERSION)/src/
	cp -R src/*.cc pekwm-$(VERSION)/src/
	cp -R src/*.hh pekwm-$(VERSION)/src/
	find pekwm-$(VERSION) -name '*~' | xargs rm 2>/dev/null || true
	tar cf pekwm-$(VERSION).tar pekwm-$(VERSION)
	gzip pekwm-$(VERSION).tar
	rm -rf pekwm-$(VERSION)

clean:
	(cd src; $(MAKE) clean)
	(cd data; $(MAKE) clean)
	rm mk/config.h mk/config.mk
