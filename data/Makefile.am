AUTOMAKE_OPTIONS = foreign

SUBDIRS = themes scripts

EXTRA_DIST = autoproperties autoproperties_typerules \
	config config_system \
	keys menu \
	mouse mouse_system mouse_click mouse_sloppy \
	start vars
cfgdatadir = $(sysconfdir)/pekwm
cfgdata_DATA = autoproperties autoproperties_typerules \
	config config_system \
	keys menu \
	mouse mouse_system mouse_click mouse_sloppy \
	start vars

distclean-local:
	rm -f *\~ .\#*
