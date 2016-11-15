AUTOMAKE_OPTIONS = foreign

scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = pekwm_ws_menu.sh pekwm_themeset.sh pekwm_screenshot.sh

CLEANFILES = pekwm_ws_menu.sh pekwm_themeset.sh
EXTRA_DIST = pekwm_ws_menu.sh.in pekwm_themeset.sh.in pekwm_screenshot.sh CMakeLists.txt

all-local: $(scripts_SCRIPTS)

%.sh:	%.sh.in
	@SED@ -e 's:\@SH\@:$(SH):g' -e 's:\@SED\@:$(SED):g' $< > $@

$(scripts_SCRIPTS):

distclean-local:
	rm -f *\~ .\#*
