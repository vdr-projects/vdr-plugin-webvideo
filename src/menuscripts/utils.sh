#!/bin/sh

function XMLEscape() {
    echo "$1" | sed "s/\&/\&amp;/g;s/>/\&gt;/g;s/</\&lt;/g;s/'/\&apos;/g"
}

function outputWebviMenu() {
    local url=$(XMLEscape "$1")
    local title=$(XMLEscape "$2")
    echo '<?xml version="1.0" encoding="UTF-8"?>'
    echo -n '<wvmenu><title>'"$title"'</title>'
    echo -n '<ul><li>'
    echo -n '<a href="'"$url"'">'"$title"'</a>'
    echo '</li></ul></wvmenu>'
}
