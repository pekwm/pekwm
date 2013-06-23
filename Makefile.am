ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign

SUBDIRS = src data doc contrib

EXTRA_DIST = LICENSE ChangeLog.aewm++ ChangeLog.until-0.1.6 autogen.sh pekwm.spec.in pekwm.spec

dist-hook:
	@SED@ -e 's:<VERSION>:$(VERSION):g' $(distdir)/pekwm.spec.in > $(distdir)/pekwm.spec

distclean-local:
	rm -f *\~ .\#*
