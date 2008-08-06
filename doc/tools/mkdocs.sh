#!/usr/bin/env bash

# Applications
OPENJADE="/usr/bin/openjade"
PDFJADETEX="/usr/bin/pdfjadetex"
TIDY="/usr/bin/tidy"
ZIP="/usr/bin/zip"

# Paths
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

# OpenJade configuration
SP_CHARSET_FIXED="0"
SP_BCTF="utf-8"

export SP_CHARSET_FIXED SP_BCTF

## Print error message and exit
function print_error() {
    echo "ERROR: $@"
    exit 1
}

## Print status message
function print_status() {
    echo "[0;1;mSTATUS[0m: $1"
}

## Clean up files
function do_clean() {
    print_status "cleaning up"
    rm -rf ${LOGDIR} ${FINDIR} ${CURDIR}/index.xml
}

## Make all targets 
mk_all() {
    do_prep

    mk_html
    mk_pdf
    mk_rtf
    mk_arc

    do_pp
}

## Prepare for document generation
function do_prep() {
    print_status "versioning index.xml"
    ${TOOLDIR}/version-process.pl index.in.xml index.xml

    print_status "versioning indices"
    mkdir -p ${FINDIR} || print_error "unable to create directory ${FINDIR}"
    ${TOOLDIR}/version-process.pl ${TOOLDIR}/empty ${FINDIR}/inc.html
    ${TOOLDIR}/version-process.pl ${TOOLDIR}/empty ${FINDIR}/inc2.html
}

## Post process creating indicies
function do_pp() {
    print_status "post-processing indices"
    echo '</ul>' >> ${FINDIR}/inc.html	
    echo '</ul>' >> ${FINDIR}/inc2.html	
    print_status "making php index"
    cp ${TOOLDIR}/index.php ${FINDIR}/index.php
}

## Make tarballs of documentations
function mk_arc() {
    cd ${FINDIR}
    cp -r ${HTMLDIR} ${DOCSNAME}
    mkdir -p ${ARCDIR}
    print_status "creating tar.bz2"
    tar -jcf ${ARCDIR}/${DOCSNAME}.tar.bz2 ${DOCSNAME}
    print_status "creating tar.gz"
    tar -zcf ${ARCDIR}/${DOCSNAME}.tar.gz ${DOCSNAME}

    print_status "generating arc indices"
    ${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.tar.bz2 HTML Files, tar + bzip2 >> ${FINDIR}/inc.html
    ${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.tar.gz HTML Files, tar + gzip >> ${FINDIR}/inc.html
    ${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.tar.bz2 HTML Files, tar + bzip2 >> ${FINDIR}/inc2.html
    ${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.tar.gz HTML Files, tar + gzip >> ${FINDIR}/inc2.html

    if test -x ${ZIP}; then
        print_status "creating zip"
        zip -9rq ${ARCDIR}/${DOCSNAME}.zip ${DOCSNAME}
        rm -r ${DOCSNAME}

        ${TOOLDIR}/mkdulink.pl ${DPFX} ${ARCDIR}/${DOCSNAME}.zip HTML Files, zip format >> ${FINDIR}/inc2.html
        ${TOOLDIR}/mkdulink.pl . ${ARCDIR}/${DOCSNAME}.zip HTML Files, zip format >> ${FINDIR}/inc.html
    fi
}

## Make RTF version of documentation
function mk_rtf() {
    print_status "generating rtf"

    mkdir -p ${LOGDIR} || print_error "unable to create directory ${LOGDIR}"
    mkdir -p ${RTFDIR} || print_error "unable to create directory ${RTFDIR}"

    ${OPENJADE} -t rtf -d ${DBP} ${DBX} index.xml >${LOGDIR}/rtf.log 2>${LOGDIR}/rtf.err
    if test ${?} -gt 0; then
	print_error "rtf generation failed! see rtf logs!"
    fi
    mv index.rtf ${RTFDIR}/${DOCSNAME}.rtf

    print_status "generating rtf indices"
    ${TOOLDIR}/mkdulink.pl . ${RTFDIR}/${DOCSNAME}.rtf RTF format >> ${FINDIR}/inc.html
    ${TOOLDIR}/mkdulink.pl ${DPFX} ${RTFDIR}/${DOCSNAME}.rtf RTF format >> ${FINDIR}/inc2.html
}

## Make HTML version of documentation, split up and single file.
function mk_html() {
    mk_html_singlefile
    mk_html_multifile
}

## Make HTML version of documentation, single file.
function mk_html_singlefile() {
    print_status "starting html-singlefile"

    mkdir -p ${LOGDIR} || print_error "unable to create directory ${LOGDIR}"
    mkdir -p ${NOCHDIR} || print_error "unable to create directory ${NOCHDIR}"

    ${OPENJADE} -t xml -V nochunks -d ${DBC} ${DBX} ${CURDIR}/index.xml \
        > ${NOCHDIR}/${DOCSNAME}.html 2>${LOGDIR}/html-singlefile.log
    if test ${?} -gt 0; then
	print_error "html generation failed! see html-singlefile logs!"
    fi

    if test -x ${TIDY}; then
        print_status "tidy - html-singlefile"
        ${TIDY} -cibqm ${NOCHDIR}/${DOCSNAME}.html \
            >${LOGDIR}/htsinglefile-tidy.log 2>${LOGDIR}/htsinglefile-tidy.err
        if test ${?} -gt 1; then
	    print_error "tidy error on html-singlefile"
        fi
    fi

    print_status "generating singlefile indices"
    ${TOOLDIR}/mkdulink.pl . ${NOCHDIR}/${DOCSNAME}.html HTML, One Big File >> ${FINDIR}/inc.html
    ${TOOLDIR}/mkdulink.pl ${DPFX} ${NOCHDIR}/${DOCSNAME}.html HTML, One Big File >> ${FINDIR}/inc2.html
}

## Make HTML version of documentation, multiple files.
function mk_html_multifile() {
    print_status "starting html-multifile"

    mkdir -p ${LOGDIR} || print_error "unable to create directory ${LOGDIR}"
    mkdir -p ${HTMLDIR} || print_error "unable to create directory ${HTMLDIR}"

    ( cd ${HTMLDIR} && \
	mkdir -p ${DIRS} && \
	${OPENJADE} -t xml -d ${DBC} ${DBX} ${CURDIR}/index.xml ) \
        >${LOGDIR}/html-multifile.log 2>${LOGDIR}/html-multifile.err
    if test ${?} -gt 0; then
	print_error "html generation failed! see html-multifile logs!"
    fi

    if test -x ${TIDY}; then
        print_status "tidy - html-multifile"
        for i in $(find ${HTMLDIR} -name '*.html'); do 
	    print_status "tidy - html-multifile - $i"
	    echo -e "\n\n****** $i\n" >> ${LOGDIR}/htmultifile-tidy.log 
	    echo -e "\n\n****** $i\n" >> ${LOGDIR}/htmultifile-tidy.err 

	    ${TIDY} -cibqm $i >> ${LOGDIR}/htmultifile-tidy.log 2> ${LOGDIR}/htmultifile-tidy.err
	    if test ${?} -gt 1; then
	        print_error "tidy error on $i"
	    fi
        done
    fi

    print_status "generating multifile indices"
    ${TOOLDIR}/mklink.pl . ${HTMLDIR}/index.html HTML, Many files >> ${FINDIR}/inc.html
    ${TOOLDIR}/mklink.pl ${DPFX} ${HTMLDIR}/index.html HTML, Many files >> ${FINDIR}/inc2.html

}

## Generate PDF documentation
function mk_pdf() {
    if ! test -x ${PDFJADETEX}; then
        print_status "skipping PDF generation as ${PDFJADETEX} does not exist."
        return
    fi

    print_status "starting pdf"

    mkdir -p ${LOGDIR} || print_error "unable to create directory ${LOGDIR}"
    mkdir -p ${PDFDIR} || print_error "unable to create directory ${PDFDIR}"

    # Step 1, generate TeX
    print_status "starting tex generation"
    ${OPENJADE} -t tex -o ${PDFDIR}/index.tex -d ${DBP} ${DBX} ${CURDIR}/index.xml \
        >${LOGDIR}/pdf-tex.log 2>${LOGDIR}/pdf-tex.err
    if test ${?} -gt 0; then
	print_error "tex generation failed! see pdf logs!"
    fi

    # Step 2, generate PDF
    print_status "starting pdf generation"
    ( cd ${PDFDIR} &&
	${PDFJADETEX} ${PDFDIR}/index.tex ) >${LOGDIR}/pdf-pdf.log 2>${LOGDIR}/pdf-pdf.err
    find ${PDFDIR} -type f | grep -v '\.pdf' | xargs rm

    print_status "generating pdf indicies"
    ${TOOLDIR}/mklink.pl . ${PDFDIR}/index.pdf PDF >> ${FINDIR}/inc.html
    ${TOOLDIR}/mklink.pl ${DPFX} ${PDFDIR}/index.pdf PDF >> ${FINDIR}/inc2.html
}

############ PROGRAM EXECUTION BEGINS

if test ! -x "${OPENJADE}"; then
    print_error "${OPENJADE} does not exist, can not generate documentation"
fi

case $1 in
    *clean) 
	do_clean
	;;
    *all)
	do_clean
	mk_all
	;;
    *help)
	echo "$0 (clean|all) [arg]"
	exit
	;;
    *)
	exec $0 all
	;;
esac