#!/bin/sh

# creates documents. fuck make. seriously. make can go blow a goat while 
# taking it from a monkey. i bet if i webcasted it, i could make good money.

DOCSNAME=pekwm-doc
CURDIR=`pwd`
FINDIR=${CURDIR}/fin
HTMLDIR=${FINDIR}/html
NOCHDIR=${FINDIR}/nochunks
TEXTDIR=${FINDIR}/text
MANDIR=${FINDIR}/man
PDFDIR=${FINDIR}/pdf
RTFDIR=${FINDIR}/rtf
ARCDIR=${FINDIR}/arc
TOOLDIR=${CURDIR}/tools
DIRS="config development overview usage faq"
PKGNAME=${DOCSNAME}.tar.bz2
ALLDOCS="html nochunks man text pdf rtf arc"
DPFX=docs

LOGDIR="${CURDIR}/logs"

# DBDIR="/usr/share/sgml/docbook/dsssl-stylesheets"
# DBC="${DBDIR}/html/custom.dsl"
# DBP="${DBDIR}/print/plain.dsl"
# DBX="${DBDIR}/dtds/decls/xml.dcl"

 DBDIR="/usr/share/sgml/docbook/dsssl-stylesheets"
 DBC="${DBDIR}/html/pekwm.dsl"
 DBP="${DBDIR}/print/plain.dsl"
 DBX="/usr/share/sgml/xml.dcl"

printerr() {
	echo "ERROR! $1!"
	exit 1
}
status() {
	echo "[0;1;mSTATUS[0m: $1"
}
verify() {
	status "VERIFY ACTION: $1"
	echo "IF YOU WANT TO DO THIS, TYPE 'yes'"
	read VERIFY
	if [[ $VERIFY != 'yes' ]]; then 
		echo "VERIFY FAILED!"
		exit 1
	fi
}

cleanup() {
	status "cleaning up"
	rm -rf ${LOGDIR} ${FINDIR} ${CURDIR}/index.xml
}

do_scp() {
	status "Beginning SCP"
	DNAME=$1
	status "Creating tarball"
    ( cd ${FINDIR} && tar -cf ${CURDIR}/${DNAME}.tar * )
	status "bzip2"
    bzip2 -9 ${CURDIR}/${DNAME}.tar
	status "scp ${DNAME}.tar.bz2"
	cd ${CURDIR}
    scp ${DNAME}.tar.bz2 babblica.net: > ${LOGDIR}/scp.log 2> ${LOGDIR}/scp.err
	status "remotely setting up docs under ${DNAME}"
    ssh babblica.net \( rm -rf web/pekwm/${DNAME} \; \
    mkdir -p /home/pekwm/html/${DNAME} \; \
    tar -C /home/pekwm/html/${DNAME} -jxf ${DNAME}.tar.bz2 \; \
    chgrp -R pekwm /home/pekwm/html/${DNAME} \; \
    chmod -R ug+rw /home/pekwm/html/${DNAME} \; \
    rm -f ${DNAME}.tar.bz2 \) > ${LOGDIR}/ssh.log 2> ${LOGDIR}/ssh.err
	if [ ${?} -gt 0 ]
	then
		printerr "ssh stuff failed for some reason."
	fi
	status "rm ${DNAME}.tar.bz2"
	rm -f ${DNAME}.tar.bz2
}

cvs_add() {
	cleanup
	status "Recursively scheduling things for cvs addition"
	find ${CURDIR} -type f -not -path '*CVS*' -exec cvs add {} \;
}

mk_all() {
	mk_prep
	mk_pdf
	mk_html
	mk_rtf
#	mk_man
#	mk_text
	mk_arc
	mk_pp
}

mk_prep() {
	status "versioning index.xml"
	${TOOLDIR}/version-process.pl index.in.xml index.xml
#	status "generating km-actions"
#	( cd config && ./generate_actions.sh > keys_mouse/actions.xml )
	status "versioning indices"
	mkdir -p ${FINDIR}
	${TOOLDIR}/version-process.pl ${TOOLDIR}/empty ${FINDIR}/inc.html
	${TOOLDIR}/version-process.pl ${TOOLDIR}/empty ${FINDIR}/inc2.html
}

mk_pp() { # post-process
	status "post-processing indices"
	echo '</ul>' >> ${FINDIR}/inc.html	
	echo '</ul>' >> ${FINDIR}/inc2.html	
	status "making php index"
	cp ${TOOLDIR}/index.php ${FINDIR}/index.php
	status "tidying php index"
#	tidy -cibqm ${FINDIR}/index.php >${LOGDIR}/php.log 2>${LOGDIR}/php.err
#	if [ ${?} -gt 1 ]
#	then
#		printerr "tidy error on php index"
#	fi
}

mk_arc() {
	cd ${FINDIR}
	cp -r ${HTMLDIR} ${DOCSNAME}
	mkdir -p ${ARCDIR}
	status "creating tar.bz2"
	tar -jcf ${ARCDIR}/${DOCSNAME}.tar.bz2 ${DOCSNAME}
	status "creating tar.gz"
	tar -zcf ${ARCDIR}/${DOCSNAME}.tar.gz ${DOCSNAME}
	status "creating zip"
	zip -9rq ${ARCDIR}/${DOCSNAME}.zip ${DOCSNAME}
	rm -r ${DOCSNAME}
	status "generating arc indices"
	${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.tar.bz2 HTML Files, tar + bzip2 >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.tar.gz HTML Files, tar + gzip >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.zip HTML Files, zip format >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.tar.bz2 HTML Files, tar + bzip2 >> ${FINDIR}/inc2.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.tar.gz HTML Files, tar + gzip >> ${FINDIR}/inc2.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.zip HTML Files, zip format >> ${FINDIR}/inc2.html
}

mk_rtf() {
	mkdir -p ${RTFDIR}
	status "generating rtf"
	/usr/bin/openjade -t rtf -d ${DBP} ${DBX} index.xml >${LOGDIR}/rtf.log 2>${LOGDIR}/rtf.err
	if [ ${?} -gt 0 ]
	then
		printerr "rtf generation failed! see rtf logs!"
	fi
	mv index.rtf ${RTFDIR}/${DOCSNAME}.rtf
	status "generating rtf indices"
	${TOOLDIR}/mkdulink.pl . ${RTFDIR}/${DOCSNAME}.rtf RTF format >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${RTFDIR}/${DOCSNAME}.rtf RTF format >> ${FINDIR}/inc2.html
}

mk_html() {
	mk_html_chunky
	mk_html_creamy
}

mk_text() {
	mkdir -p ${TEXTDIR}
	status "generating text docs"
	vilistextum -l ${NOCHDIR}/${DOCSNAME}.html ${TEXTDIR}/${DOCSNAME}.txt >${LOGDIR}/text.log 2>${LOGDIR}/text.err
	if [ ${?} -gt 0 ]
	then
		printerr "text generation failed! see text logs!"
	fi
	status "generating text indices"
	${TOOLDIR}/mkdulink.pl . ${TEXTDIR}/${DOCSNAME}.txt Plain Text Format >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${TEXTDIR}/${DOCSNAME}.txt Plain Text Format >> ${FINDIR}/inc2.html
}

mk_html_creamy() {
	status "starting html-creamy"
	mkdir -p ${NOCHDIR}
	/usr/bin/openjade -t xml -V nochunks -d ${DBC} ${DBX} ${CURDIR}/index.xml > ${NOCHDIR}/${DOCSNAME}.html 2>${LOGDIR}/html-creamy.log
	if [ ${?} -gt 0 ]
	then
		printerr "html generation failed! see html-creamy logs!"
	fi
	status "tidy - html-creamy"
	tidy -cibqm ${NOCHDIR}/${DOCSNAME}.html >${LOGDIR}/htcreamy-tidy.log 2>${LOGDIR}/htcreamy-tidy.err
	if [ ${?} -gt 1 ]
	then
		printerr "tidy error on html-creamy"
	fi
	status "generating creamy indices"
	${TOOLDIR}/mkdulink.pl . ${NOCHDIR}/${DOCSNAME}.html HTML, One Big File >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${NOCHDIR}/${DOCSNAME}.html HTML, One Big File >> ${FINDIR}/inc2.html
}

mk_man() {
	# this depends on mk_html_creamy()
	status "making man page"
	mkdir -p ${MANDIR}
	html2pod ${NOCHDIR}/${DOCSNAME}.html > ${MANDIR}/pekwm.pod
	pod2man ${MANDIR}/pekwm.pod > ${MANDIR}/temp.man
	cat ${MANDIR}/temp.man | sed "s/User Contributed Perl Documentation/PEKWM Documentation/" > ${MANDIR}/pekwm.man
	rm ${MANDIR}/pekwm.pod ${MANDIR}/temp.man
	status "generating man page indices"
	${TOOLDIR}/mkdulink.pl . ${MANDIR}/pekwm.man Manual >> ${FINDIR}/inc.html
	${TOOLDIR}/mkdulink.pl ${DPFX} ${MANDIR}/pekwm.man Manual >> ${FINDIR}/inc2.html
}

mk_html_chunky() {
	status "starting html-chunky"
	mkdir -p ${HTMLDIR}
	mkdir -p ${LOGDIR}
	( cd ${HTMLDIR} && \
	mkdir -p ${DIRS} && \
	/usr/bin/openjade -t xml -d ${DBC} ${DBX} ${CURDIR}/index.xml ) >${LOGDIR}/html-chunky.log 2>${LOGDIR}/html-chunky.err
	if [ ${?} -gt 0 ]
	then
		printerr "html generation failed! see html-chunky logs!"
	fi
	status "starting tidy"
	for i in `find ${HTMLDIR} -name '*.html'`; do 
		status "tidy - $i"
		echo -e "\n\n****** $i\n" >> ${LOGDIR}/htchunky-tidy.log 
		echo -e "\n\n****** $i\n" >> ${LOGDIR}/htchunky-tidy.err 
		tidy -cibqm $i >> ${LOGDIR}/htchunky-tidy.log 2> ${LOGDIR}/htchunky-tidy.err
		if [ ${?} -gt 1 ]
		then
			printerr "tidy error on $i"
		fi
	done
	status "generating chunky indices"
	${TOOLDIR}/mklink.pl . ${HTMLDIR}/index.html HTML, Many files >> ${FINDIR}/inc.html
	${TOOLDIR}/mklink.pl ${DPFX} ${HTMLDIR}/index.html HTML, Many files >> ${FINDIR}/inc2.html

}

mk_pdf() {
	status "starting pdf"

	mkdir -p ${LOGDIR}
	mkdir -p ${PDFDIR}

	# Step 1, generate TeX
	status "starting tex generation"
	openjade -t tex -o ${PDFDIR}/index.tex -d ${DBP} ${DBX} ${CURDIR}/index.xml >${LOGDIR}/pdf-tex.log 2>${LOGDIR}/pdf-tex.err
	if [ ${?} -gt 0 ]
	then
		printerr "tex generation failed! see pdf logs!"
	fi

	# Step 2, generate PDF
	status "starting pdf generation"
	( cd ${PDFDIR} &&
	pdfjadetex ${PDFDIR}/index.tex ) >${LOGDIR}/pdf-pdf.log 2>${LOGDIR}/pdf-pdf.err
	find ${PDFDIR} -type f | grep -v '\.pdf' | xargs rm

	status "generating pdf indicies"
	${TOOLDIR}/mklink.pl . ${PDFDIR}/index.pdf PDF >> ${FINDIR}/inc.html
	${TOOLDIR}/mklink.pl ${DPFX} ${PDFDIR}/index.pdf PDF >> ${FINDIR}/inc2.html
}

############ PROGRAM EXECUTION BEGINS

case $1 in
	*clean) 
		cleanup
	;;
	*all)
		cleanup
		mk_all
	;;
	*scp)
		cleanup
		DPFX=$2
		verify "UPLOAD ${DPFX}"
		mk_all
		do_scp $2
	;;
	*help)
		echo "$0 (clean|all|scp) [arg]"
		exit
	;;
	*)
		exec $0 all
	;;
esac

#cleanup
#mk_all
