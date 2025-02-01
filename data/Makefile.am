SUBDIRS = themes scripts

config_files = autoproperties \
	autoproperties_clientrules \
	autoproperties_typerules \
	config \
	config_system \
	keys \
	keys_inputdialog \
	keys_menu \
	keys_moveresize \
	menu \
	menu_windowmenu \
	mouse \
	mouse_system \
	mouse_click \
	mouse_sloppy \
	panel \
	start \
	vars

EXTRA_DIST = CMakeLists.txt $(config_files)

cfgdatadir = $(sysconfdir)/pekwm
cfgdata_DATA = $(config_files)
