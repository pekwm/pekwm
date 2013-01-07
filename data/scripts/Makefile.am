AUTOMAKE_OPTIONS = foreign

scriptsdir = $(pkgdatadir)/scripts
scripts_SCRIPTS = pekwm_ws_menu.sh pekwm_themeset.sh

CLEANFILES = pekwm_ws_menu.sh pekwm_themeset.sh
EXTRA_DIST = pekwm_ws_menu.sh.in pekwm_themeset.sh.in

all-local: $(scripts_SCRIPTS)

pekwm_ws_menu.sh:
	@SED@ -e 's:<SH>:$(SH):g' -e 's:<SED>:$(SED):g' $(srcdir)/$@.in > $@

pekwm_themeset.sh:
	@SED@ -e 's:<SH>:$(SH):g' -e 's:<SED>:$(SED):g' $(srcdir)/$@.in > $@

distclean-local:
	rm -f *\~ .\#*
