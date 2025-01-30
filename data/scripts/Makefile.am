commandsdir = $(bindir)
commands_SCRIPTS = pekwm_theme

scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = pekwm_lock.sh \
	pekwm_panel_battery.sh \
	pekwm_themeset.sh \
	pekwm_toggle_kbd_layout.sh \
	pekwm_ws_menu.sh


CLEANFILES = $(commands_SCRIPTS) $(scripts_SCRIPTS)

pekwm_theme: pekwm_theme.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		-e 's|\@PEKWM_THEMES\@|$(PEKWM_THEMES)|g' \
		-e 's:\@CMAKE_INSTALL_PREFIX\@:$(prefix):g' \
		< $< > $@

%.sh: %.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' < $< > $@

EXTRA_DIST = pekwm_lock.sh.in \
	pekwm_panel_battery.sh.in \
	pekwm_theme.in \
	pekwm_themeset.sh.in \
	pekwm_toggle_kbd_layout.sh.in \
	pekwm_ws_menu.sh.in
