AUTOMAKE_OPTIONS = foreign

SUBDIRS = themes scripts

CLEANFILES = config menu
EXTRA_DIST = autoproperties config.in keys menu.in mouse start vars
cfgdatadir = $(sysconfdir)/pekwm
cfgdata_DATA = autoproperties config keys menu mouse start vars

all-local: config menu

config: config.in
	@regex_cmd@ -e 's:<PREFIX>:$(pkgdatadir):g' config.in > config

menu: menu.in
	@regex_cmd@ -e 's:<PREFIX>:$(pkgdatadir):g' menu.in > menu

distclean-local:
	rm -f *\~ .\#*
