AUTOMAKE_OPTIONS = foreign

SUBDIRS = themes scripts

CLEANFILES = config menu
EXTRA_DIST = autoproperties config.in keys menu.in mouse start vars
cfgdatadir = $(sysconfdir)/pekwm
cfgdata_DATA = autoproperties config keys menu mouse start vars

all-local: config menu

%: $(srcdir)/%.in
	@SED@ -e 's:<PREFIX>:$(pkgdatadir):g' -e "s:<THEME>:@THEME@:g" $< > $@

distclean-local:
	rm -f *\~ .\#*
