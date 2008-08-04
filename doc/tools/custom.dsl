<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY docbook.dsl SYSTEM "docbook.dsl" CDATA dsssl>
]>

<style-sheet>

<style-specification use="docbook">
<style-specification-body>

(define %generate-legalnotice-link%
;; put the legal notice in a separate file
#t)

(define ($legalnotice-link-file$ legalnotice)
;; filename of the legalnotice file
(string-append "legalnotice"%html-ext%))

(define %html-ext%
;; html extenstion
".html")

(define %root-filename%
;; index file of the book
"index")

(define %use-id-as-filename%
;; filenames same as id attribute in title tags
#t)

(define %body-attr%
;; html body settings
	(list
	 (list "BGCOLOR" "#FFFFFF")
	 (list "TEXT" "#000000")
         (list "STYLE" "font-family: verdana, arial, helvetica, sans-serif;")))

(define (chunk-skip-first-element-list)
;; forces the Table of Contents on separate page
'())

(define (list-element-list)
;; fixes bug in Table of Contents generation
'())

(define %shade-verbatim%
;; verbatim sections will be shaded if t(rue)
#t)

(define %section-autolabel%
;; for enumerated sections (1.1, 1.1.1, 1.2, etc.)
#t)

;; custom formatting
(element para
  (make element 
    gi: "P" 
		attributes: '(("STYLE" "text-indent: 1em;"))
		(process-children)))

(element tgroup
	(make element
		gi: "TABLE"
		attributes: '(("CELLSPACING" "1") ("BORDER" "1") ("CELLPADDING" "1") ("WIDTH" "100%") ("BGCOLOR" "#C8CCC8"))))

(element varlistentry
	(make element
		gi: "DT"
		attributes: '(("STYLE" "text-decoration: underline"))))

(element screen
  (make element
    gi: "DIV"
		attributes: '(("ALIGN" "center"))
		(make element 
			gi: "TABLE"
			attributes: '(("CELLSPACING" "1") ("CELLPADDING" "1") ("WIDTH" "100%") ("BGCOLOR" "#C8CCC8"))
			(make element 
				gi: "TR"
				(make element 
					gi: "TD"
					attributes: '(("BGCOLOR" "#FEFFEC")) 
					(make element
						gi: "PRE" ;; (process-children)
						attributes: '(("STYLE" "padding: 0.25em") ("STYLE" "text-align: left")
                                                              ("STYLE" "margin-top: 0.25em") ("STYLE" "margin-bottom: 0.2em"))))))))

;; make role=strong equate to bold for emphasis tag
(element emphasis
	(if (equal? (attribute-string "role") "strong")
		(make element gi: "STRONG" (process-children))
		(make element gi: "EM" (process-children))))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>

