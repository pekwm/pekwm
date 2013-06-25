#!/bin/sh
#
# Copyright © 2013 the pekwm development team
#

screenshot_scrot()
{
    scrot -z "$1"
}

screenshot_xwd_netpbm()
{
    xwd -root | xwdtopnm 2>/dev/null | pnmtopng > "$1"
}

screenshot_imagemagick()
{
    import -window root "$1"
}

is_in_path()
{
    which $1 >/dev/null 2>&1
    return $?
}

detect_command()
{
    is_in_path "scrot"
    if test $? -eq 0; then
        command="scrot"
        return
    fi

    is_in_path "import"
    if test $? -eq 0; then
        command="magick"
        return
    fi

    is_in_path "xwd" && is_in_path "xwdtopnm" && is_in_path "pnmtopng"
    if test $? -eq 0; then
        command="netpbm"
        return
    fi
}

usage()
{
    echo "usage: pekwm_screenshot.sh [-c scrot|netpbm|magick] [-d delay] [-o output.png]"
    echo ""
    echo "    -c scrot|netpbm|magick (defaults to autodetect)"
    echo "       Command to use when creating screenshot."
    echo "    -d seconds (defaults to 0)"
    echo "       Number of seconds to wait before taking screenshot"
    echo "    -h"
    echo "       Display this information"
    echo "    -o output.png (defaults to ~/screenshot_YYYYMMDD_HHmmSS.png)"
    echo "       Name of screenshot output file."
    echo ""
    exit 0
}

usage_command()
{
    if test -z "$command"; then
        echo "Unable to find any supported commands for taking screenshots"
    else
        echo "Unsupported screenshot command $command"
    fi

    echo ""
    echo "Supported screenshot commands are:"
    echo ""
    echo "  * scrot, http://linuxbrit.co.uk/software/"
    echo "  * imagemagick, http://www.imagemagick.org/"
    echo "  * netpbm (+ xwd), http://netpbm.sourceforge.net/"
    echo ""
    exit 1
}

main()
{
    # Initialize for strict mode
    command=""
    delay="0"
    output=""

    # Parse options
    eval set -- "$OPTIONS"

    while true; do
        case "$1" in
            -c)
                command="$2"
                shift 2;;
            -d)
                delay=$((0 + $2))
                shift 2;;
            -h)
                usage;;
            -o)
                output="$2"
                shift 2;;
            --)
                break;;
        esac
    done

    # No command specified, try autodetect
    if test -z "$command"; then
        detect_command
        if test -z "$command"; then
            usage_command
        fi
    fi

    # No output specified, format
    if test -z "$output"; then
        output="$HOME/screenshot_$(date '+%Y%m%d_%H%M%S').png"
    fi

    # Wait for N seconds if specified
    if test "$delay" -ne "0"; then
        echo "Taking screenshot in $delay seconds..."
        sleep $delay
    fi

    # Grab screenshot
    case "$command" in
        scrot)
            screenshot_scrot $output
            ;;
        magick)
            screenshot_imagemagick $output
            ;;
        netpbm)
            screenshot_xwd_netpbm $output
            ;;
        *)
            usage_command
            ;;
    esac

    if test $? -eq 0; then
        echo "Successfully captured screen to $output"
    else
        echo "Failed to capture screen to $output!"
    fi

    exit 0
}

OPTIONS=$(getopt -o c:d:ho: -n 'pekwm_screenshot.sh' -- "$@")
main

