AUTOMAKE_OPTIONS = foreign

SUBDIRS = themes scripts

CLEANFILES = config menu
EXTRA_DIST = autoproperties config.in keys menu.in mouse start vars
cfgdatadir = $(sysconfdir)/pekwm
cfgdata_DATA = autoproperties config keys menu mouse start vars

all-local: config menu

config:
	@SED@ -e 's:<PREFIX>:$(pkgdatadir):g' -e "s:<THEME>:@THEME@:g" $@.in > $@
menu:
	@SED@ -e 's:<PREFIX>:$(pkgdatadir):g' -e "s:<THEME>:@THEME@:g" $@.in > $@

distclean-local:
	rm -f *\~ .\#*
