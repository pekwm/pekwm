commandsdir = $(bindir)
commands_SCRIPTS = pekwm_theme

scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = pekwm_audio_ctrl.sh \
		  pekwm_lock.sh \
		  pekwm_panel_battery.sh \
		  pekwm_themeset.sh \
		  pekwm_toggle_kbd_layout.sh \
		  pekwm_ws_menu.sh


CLEANFILES = $(commands_SCRIPTS) $(scripts_SCRIPTS)

# not using < $< > $@ or %.sh: %.sh.in $@ due to Solaris and NetBSD
# make compatability
pekwm_theme: pekwm_theme.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		-e 's|\@PEKWM_THEMES\@|$(PEKWM_THEMES)|g' \
		-e 's:\@CMAKE_INSTALL_PREFIX\@:$(prefix):g' \
		< pekwm_theme.in > pekwm_theme

pekwm_audio_ctrl.sh: pekwm_audio_ctrl.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_audio_ctrl.sh.in > pekwm_audio_ctrl.sh

pekwm_lock.sh: pekwm_lock.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_lock.sh.in > pekwm_lock.sh

pekwm_panel_battery.sh: pekwm_panel_battery.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_panel_battery.sh.in > pekwm_panel_battery.sh

pekwm_themeset.sh: pekwm_themeset.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_themeset.sh.in > pekwm_themeset.sh

pekwm_toggle_kbd_layout.sh: pekwm_toggle_kbd_layout.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_toggle_kbd_layout.sh.in > pekwm_toggle_kbd_layout.sh

pekwm_ws_menu.sh: pekwm_ws_menu.sh.in
	$(SED) -e 's:\@AWK\@:$(AWK):g' -e 's:\@GREP\@:$(GREP):g' \
		-e 's:\@SED\@:$(SED):g' -e 's:\@SH\@:$(PEKWM_SH):g' \
		< pekwm_ws_menu.sh.in > pekwm_ws_menu.sh

EXTRA_DIST = CMakeLists.txt \
	     pekwm_lock.sh.in \
	     pekwm_panel_battery.sh.in \
	     pekwm_theme.in \
	     pekwm_themeset.sh.in \
	     pekwm_toggle_kbd_layout.sh.in \
	     pekwm_ws_menu.sh.in
