proc makeColorPicker {w var command cancel} {
    global $var

    frame $w.c

    frame $w.c.col -relief ridge -borderwidth 4 -height 1.5c -width 6c \
	    -background #000000
    set col $w.c.col

    frame $w.c.picks
    set picks $w.c.picks

    frame $picks.rgb -relief groove -borderwidth 4
    set rgb $picks.rgb
    scale $rgb.s1 -label Red -from 0.0 -to 1.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-r
    scale $rgb.s2 -label Green -from 0.0 -to 1.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-g
    scale $rgb.s3 -label Blue -from 0.0 -to 1.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-b
    pack $rgb.s1 -in $picks.rgb -side top -padx 2 -pady 2 -anchor nw -fill y
    pack $rgb.s2 -in $picks.rgb -side top -padx 2 -pady 2 -anchor nw -fill y
    pack $rgb.s3 -in $picks.rgb -side top -padx 2 -pady 2 -anchor nw -fill y

    frame $picks.hsv -relief groove -borderwidth 4 
    set hsv $picks.hsv
    scale $hsv.s1 -label Hue -from 0.0 -to 360.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-h
    scale $hsv.s2 -label Saturation -from 0.0 -to 1.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-s
    scale $hsv.s3 -label Value -from 0.0 -to 1.0 -length 6c -showvalue true \
	    -orient horizontal -resolution .01 \
	    -digits 3 -variable $w-v
    pack $hsv.s1 -in $picks.hsv -side top -padx 2 -pady 2 -anchor nw -fill y
    pack $hsv.s2 -in $picks.hsv -side top -padx 2 -pady 2 -anchor nw -fill y
    pack $hsv.s3 -in $picks.hsv -side top -padx 2 -pady 2 -anchor nw -fill y

    $rgb.s1 configure -command "cpsetrgb $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "
    $rgb.s2 configure -command "cpsetrgb $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "
    $rgb.s3 configure -command "cpsetrgb $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "
    $hsv.s1 configure -command "cpsethsv $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "
    $hsv.s2 configure -command "cpsethsv $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "
    $hsv.s3 configure -command "cpsethsv $w $var $col $rgb.s1 $rgb.s2 $rgb.s3 \
	    $hsv.s1 $hsv.s2 $hsv.s3 "

    frame $w.c.opts
    button $w.c.opts.ok -text OK -command "cpcommitcolor $var $rgb.s1 $rgb.s2 $rgb.s3 \"$command\""
    button $w.c.opts.cancel -text Cancel -command $cancel
    checkbutton $w.c.opts.rgb -text RGB -variable $w-rgb \
	    -command "cptogrgb $w $picks $rgb"
    checkbutton $w.c.opts.hsv -text HSV -variable $w-hsv \
	    -command "cptoghsv $w $picks $hsv"
    pack $w.c.opts.ok -in $w.c.opts -side left -padx 2 -pady 2 -anchor w
    pack $w.c.opts.cancel -in $w.c.opts -side left -padx 2 -pady 2 -anchor w
    pack $w.c.opts.rgb -in $w.c.opts -side left -padx 2 -pady 2 -anchor e
    pack $w.c.opts.hsv -in $w.c.opts -side left -padx 2 -pady 2 -anchor e

    global $w-rgb $w-hsv
    if [expr ([set $w-rgb] == 1) || ([set $w-hsv] != 1)] {
	set $w-rgb 1
	pack $rgb -in $picks -side left -padx 2 -pady 2 -expand 1 -fill x
    }
    if [expr [set $w-hsv] == 1] {
	pack $hsv -in $picks -side left -padx 2 -pady 2 -expand 1 -fill x
    }

    pack $w.c.opts $picks $col -in $w.c -side top \
	    -padx 2 -pady 2 -expand 1 -fill both
    pack $w.c
}

proc Max {n1 n2 n3} {
    if [expr $n1 >= $n2] {
	if [expr $n1 >= $n3] {
	    return $n1
	} else {
	    return $n3
	}
    } else {
	if [expr $n2 >= $n3] {
	    return $n2
	} else {
	    return $n3
	}
    }
}

proc Min {n1 n2 n3} {
    if [expr $n1 <= $n2] {
	if [expr $n1 <= $n3] {
	    return $n1
	} else {
	    return $n3
	}
    } else {
	if [expr $n2 <= $n3] {
	    return $n2
	} else {
	    return $n3
	}
    }
}

proc cpsetrgb {w var col rs gs bs hs ss vs val} {
    # Do inverse transformation to HSV
    set max [Max [$rs get] [$gs get] [$bs get]]
    set min [Min [$rs get] [$gs get] [$bs get]]
    # $ss set [expr ($max == 0.0) ? 0.0 : (($max-$min)/$max)]
    if {$max == 0.0} {
	$ss set 0.0
    } else {
	$ss set [expr ($max-$min)/$max]
    }
    if [expr [$ss get] != 0.0] {
	set rl [expr ($max-[$rs get])/($max-$min)]
	set gl [expr ($max-[$gs get])/($max-$min)]
	set bl [expr ($max-[$bs get])/($max-$min)]
	if [expr $max == [$rs get]] {
	    if [expr $min == [$gs get]] {
		$hs set [expr 60.0*(5.0+$bl)]
	    } else {
		$hs set [expr 60.0*(1.0-$gl)]
	    }
	} elseif [expr $max == [$gs get]] {
	    if [expr $min == [$bs get]] {
		$hs set [expr 60.0*(1.0+$rl)]
	    } else {
		$hs set [expr 60.0*(3.0-$bl)]
	    }
	} else {
	    if [expr $min == [$rs get]] {
		$hs set [expr 60.0*(3.0+$gl)]
	    } else {
		$hs set [expr 60.0*(5.0-$rl)]
	    }
	}
    } else {
	$hs set 0.0
    }
    $vs set $max

    cpsetcol $var $col [$rs get] [$gs get] [$bs get]

    update idletasks
}

proc cpsethsv {w var col rs gs bs hs ss vs val} {
    # Convert to RGB...
    while {[$hs get] >= 360.0} {
	$hs set [expr [$hs get] - 360.0]
    }
    while {[$hs get] < 0.0} {
	$hs set [expr [$hs get] + 360.0]
    }
    set h6 [expr [$hs get]/60.0]
    set i [expr int($h6)]
    set f [expr $h6-$i]
    set p1 [expr [$vs get]*(1.0-[$ss get])]
    set p2 [expr [$vs get]*(1.0-([$ss get]*$f))]
    set p3 [expr [$vs get]*(1.0-([$ss get]*(1-$f)))]
    switch $i {
	0 {$rs set [$vs get] ; $gs set $p3 ; $bs set $p1}
	1 {$rs set $p2 ; $gs set [$vs get] ; $bs set $p1}
	2 {$rs set $p1 ; $gs set [$vs get] ; $bs set $p3}
	3 {$rs set $p1 ; $gs set $p2 ; $bs set [$vs get]}
	4 {$rs set $p3 ; $gs set $p1 ; $bs set [$vs get]}
	5 {$rs set [$vs get] ; $gs set $p1 ; $bs set $p2}
	default {$rs set 0 ; $gs set 0 ; $bs set 0}
    }

    cpsetcol $var $col [$rs get] [$gs get] [$bs get]

    update idletasks
}

proc cpsetcol {var col r g b} {
    set ir [expr int($r * 65535)]
    set ig [expr int($g * 65535)]
    set ib [expr int($b * 65535)]

    $col config -background [format #%04x%04x%04x $ir $ig $ib]
}

proc cpcommitcolor {var rs gs bs command} {
    set r [$rs get]
    set g [$gs get]
    set b [$bs get]
    global $var-r $var-g $var-b
    set $var-r $r
    set $var-g $g
    set $var-b $b
    eval $command
}

proc cptogrgb {w picks rgb} {
    global $w-rgb

    if [expr [set $w-rgb] == 1] {
	pack $rgb -in $picks -side left
    } else {
	pack forget $rgb
    }
}

proc cptoghsv {w picks hsv} {
    global $w-hsv

    if [expr [set $w-hsv] == 1] {
	pack $hsv -in $picks -side left
    } else {
	pack forget $hsv
    }
}

