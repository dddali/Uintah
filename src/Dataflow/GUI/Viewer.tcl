#
#  For more information, please see: http://software.sci.utah.edu
# 
#  The MIT License
# 
#  Copyright (c) 2004 Scientific Computing and Imaging Institute,
#  University of Utah.
# 
#  License for the specific language governing rights and limitations under
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#

itcl_class SCIRun_Render_Viewer {
    inherit Module

    # List of ViewWindows that are children of this Viewer
    protected openViewersList

    constructor {config} {
	set name Viewer
	set_defaults
    }

    destructor {
	foreach rid $openViewersList {
	    deleteViewWindow $rid
	}
    }

    method number {} {
	set parts [split $this _]
	return [expr 1+[lindex $parts end]]
    }

    method set_defaults {} {
	set make_progress_graph 0
	set make_time 0
	set openViewersList ""
    }

    method makeViewWindowID {} {
	set nextrid 0
	set id $this-ViewWindow_$nextrid
	while {[::info commands $id] != ""} {
	    incr nextrid
	    set id $this-ViewWindow_$nextrid	    
	}
	return $id
    }

    method addViewer {} {
	set rid [makeViewWindowID]
	$this-c addviewwindow $rid
	ViewWindow $rid -viewer $this 
	lappend openViewersList $rid
	return $rid
    }

    method deleteViewWindow { rid } {
	$this-c deleteviewwindow $rid
	listFindAndRemove openViewersList $rid
	destroy .ui[$rid modname]
	$rid delete
    }

    method duplicateViewer {old_vw} {
	# Button "New Viewer" was pressed.
	# Create a new viewer with the same eyep, lookat, up and fov.
	set rid [$this addViewer]

	# Use position of previous viewer if not the first one
	set new_id [lindex [split $rid _] end]
	set old_id [lindex [split $old_vw _] end]

	set new_win $this-ViewWindow_$new_id
	set old_win $this-ViewWindow_$old_id

	setGlobal $new_win-view-eyep-x [set $old_win-view-eyep-x]
	setGlobal $new_win-view-eyep-y [set $old_win-view-eyep-y]
	setGlobal $new_win-view-eyep-z [set $old_win-view-eyep-z]

	setGlobal $new_win-view-lookat-x [set $old_win-view-lookat-x]
	setGlobal $new_win-view-lookat-y [set $old_win-view-lookat-y]
	setGlobal $new_win-view-lookat-z [set $old_win-view-lookat-z]

	setGlobal $new_win-view-up-x [set $old_win-view-up-x]
	setGlobal $new_win-view-up-y [set $old_win-view-up-y]
	setGlobal $new_win-view-up-z [set $old_win-view-up-z]

	setGlobal $new_win-view-fov [set $old_win-view-fov]

	$new_win-c redraw
    }
    
    method ui {} {
	if { [llength $openViewersList] == 0 } {;# If there are no open viewers
	    $this addViewer ;# then create one
	} else { ;# else, raise them all.
	    foreach rid $openViewersList {
		SciRaise .ui[$rid modname]
	    }
	}
    }

    method ui_embedded {} {
	set rid [makeViewWindowID]
	lappend openViewersList $rid
	return [EmbeddedViewWindow $rid -viewer $this]
    }
}



itcl_class ViewWindow {
    public viewer
    
    # parameters to hold current state of detachable part
    protected IsAttached 
    protected IsDisplayed
    # hold names of detached and attached windows
    protected detachedFr
    protected attachedFr

    method modname {} {
	return [string trimleft $this :]
    }

    method number {} {
	set parts [split $this _]
	return [expr 1+[lindex $parts end]]
    }

    method set_defaults {} {
	# set defaults values for parameters that weren't set in a script
	initGlobal $this-saveFile "MyImage.ppm"
	initGlobal $this-saveType "ppm"

	# Animation parameters
	initGlobal $this-current_time 0
	initGlobal $this-tbeg 0
	initGlobal $this-tend 1
	initGlobal $this-framerate 15
	initGlobal $this-totframes 30
	initGlobal $this-caxes 0
	initGlobal $this-raxes 1

	# Need to initialize the background color
	initGlobal $this-bgcolor-r 0
	initGlobal $this-bgcolor-g 0
	initGlobal $this-bgcolor-b 0

	# Need to initialize the scene material scales
	initGlobal $this-ambient-scale 1.0
	initGlobal $this-diffuse-scale 1.0
	initGlobal $this-specular-scale 0.4
	initGlobal $this-emission-scale 1.0
	initGlobal $this-shininess-scale 1.0

	# Initialize point size, line width, and polygon offset
	initGlobal $this-point-size 1.0
	initGlobal $this-line-width 1.0
	initGlobal $this-polygon-offset-factor 1.0
	initGlobal $this-polygon-offset-units 0.0

	# Set up lights
	initGlobal $this-global-light0 1 ; # light 0 is the head light
	initGlobal $this-global-light1 0
	initGlobal $this-global-light2 0
	initGlobal $this-global-light3 0
	trace variable $this-global-light0 w "$this traceLight 0"
	trace variable $this-global-light1 w "$this traceLight 1"
	trace variable $this-global-light2 w "$this traceLight 2"
	trace variable $this-global-light3 w "$this traceLight 3"
	initGlobal $this-lightVectors \
	    {{ 0 0 1 } { 0 0 1 } { 0 0 1 } { 0 0 1 }}
	initGlobal $this-lightColors \
	    {{1.0 1.0 1.0} {1.0 1.0 1.0} {1.0 1.0 1.0} {1.0 1.0 1.0}}

	initGlobal $this-sbase 0.4
	initGlobal $this-sr 1
	initGlobal $this-do_stereo 0

	initGlobal $this-def-color-r 1.0
	initGlobal $this-def-color-g 1.0
	initGlobal $this-def-color-b 1.0

	initGlobal $this-ortho-view 0
	initGlobal $this-currentvisual 0

	initGlobal $this-trackViewWindow0 1

	initGlobal $this-geometry [wm geometry .ui[modname]]
	trace variable $this-geometry w "$this traceGeom"

        # CollabVis code begin
        if { [set $this-have_collab_vis] } {
	    initGlobal $this-view_server 0
        }
        # CollabVis code end

	setGlobal $this-global-light 1
	setGlobal $this-global-fog 0
	setGlobal $this-global-type Gouraud
	setGlobal $this-global-debug 0
	setGlobal $this-global-clip 1
	setGlobal $this-global-cull 0
	setGlobal $this-global-dl 0
	setGlobal $this-global-movie 0
	setGlobal $this-global-movieName "./movie.%04d"
	setGlobal $this-global-movieFrame 0
	setGlobal $this-global-resize 0
	setGlobal $this-x-resize 700
	setGlobal $this-y-resize 512
	setGlobal $this-do_bawgl 0
	setGlobal $this-tracker_state 0
	setGlobal $this-currentvisual 0
	setGlobal $this-currentvisualhelper 0
    }

    destructor {
	destroy .ui[modname]
    }

    constructor {config} {
	set w .ui[modname]

	# create the window 
	toplevel $w

	# (immediately withdraw it so that on the Mac it will size correctly.)
	wm withdraw $w

	wm protocol $w WM_DELETE_WINDOW "$viewer deleteViewWindow $this"
	wm title $w "Viewer [$viewer number] Window [number]"
	wm iconname $w "Viewer [$viewer number] Window [number]"
	wm minsize $w 100 100
	set_defaults 

	frame $w.menu -relief raised -borderwidth 3
	pack $w.menu -fill x

	menubutton $w.menu.file -text "File" -underline 0 \
		-menu $w.menu.file.menu
	menu $w.menu.file.menu
	$w.menu.file.menu add command -label "Save Image..." \
	    -underline 0 -command "$this makeSaveImagePopup"
	$w.menu.file.menu add command -label "Record Movie..." \
	    -underline 0 -command "$this makeSaveMoviePopup"

	frame $w.wframe -borderwidth 3 -relief sunken
	pack $w.wframe -side bottom -expand yes -fill both -padx 4 -pady 4

	# Edit Menu
	set editmenu $w.menu.edit.menu
	menubutton $w.menu.edit -text "Edit" -underline 0 -menu $editmenu
	menu $editmenu
	$editmenu add command -label "View/Camera..." -underline 0 \
		-command "$this makeViewPopup"
	$editmenu add command -label "Light Sources..." -underline 0 \
	    -command "$this makeLightSources"
	$editmenu add command -label "Background..." -underline 0 \
		-command "$this makeBackgroundPopup"
	$editmenu add command -label "Clipping Planes..." -underline 0 \
	    -command "$this makeClipPopup"
	$editmenu add command -label "Point Size..." -underline 0 \
		-command "$this makePointSizePopup"
	$editmenu add command -label "Line Width..." -underline 0 \
		-command "$this makeLineWidthPopup"
	$editmenu add command -label "Polygon Offset..." -underline 0 \
		-command "$this makePolygonOffsetPopup"
	$editmenu add command -label "Scene Materials..." -underline 0 \
		-command "$this makeSceneMaterialsPopup"

	# Open-GL Visual Menu
	menubutton $w.menu.visual -text "Visual" -underline 0 \
	    -menu $w.menu.visual.menu
	menu $w.menu.visual.menu
	set i 0
	global $this-currentvisual
	foreach t [$this-c listvisuals $w] {
	    $w.menu.visual.menu add radiobutton -value $i -label $t \
		-variable $this-currentvisual \
		-font "-Adobe-Helvetica-bold-R-Normal-*-12-75-*"
	    incr i
	}

	# New ViewWindow button
	button $w.menu.newviewer -text "NewViewer" \
	    -command "$viewer duplicateViewer [modname]" -borderwidth 0
	
	pack $w.menu.file -side left
	pack $w.menu.edit -side left
	pack $w.menu.visual -side left
	pack $w.menu.newviewer -side left

	# create the scrolled frame
	iwidgets::scrolledframe $w.bsframe -width 640 -height 90 \
		-vscrollmode none -hscrollmode dynamic \
		-sbwidth 10 -relief groove
	pack $w.bsframe -side bottom -before $w.wframe -anchor w -fill x

	# get the childsite to add stuff to
	set bsframe [$w.bsframe childsite]

	# Performance Stats Report window
	frame $bsframe.pf
	pack $bsframe.pf -side left -anchor n
	label $bsframe.pf.perf1 -width 32 -text "? polygons in ? seconds"
	pack $bsframe.pf.perf1 -side top -anchor n
	label $bsframe.pf.perf2 -width 32 -text "? polygons/second"
	pack $bsframe.pf.perf2 -side top -anchor n
	label $bsframe.pf.perf3 -width 32 -text "? frames/sec"
	pack $bsframe.pf.perf3 -side top -anchor n

	# Mouse Mode Report Window
	canvas $bsframe.mousemode -width 175 -height 60 \
		-relief groove -borderwidth 2
	pack $bsframe.mousemode -side left -fill y -pady 2 -padx 2
	global $bsframe.mousemode.text
	set mouseModeText $bsframe.mousemode.text
	$bsframe.mousemode create text 5 30 -tag mouseModeText \
		-text " Current Mouse Mode " -anchor w

	# View Buttons Frame
	frame $bsframe.v1
	pack $bsframe.v1 -side left

	# AutoView Button
	button $bsframe.v1.autoview -text "Autoview" \
	    -command "$this-c autoview" -width 10
	pack $bsframe.v1.autoview -fill x -pady 2 -padx 2
	Tooltip $bsframe.v1.autoview \
           "Instructs the Viewer to move the camera to a position that will\n"\
           "allow all geometry to be rendered visibly in the viewing window."

	# Views... Menu Button
	frame $bsframe.v1.views             
	pack $bsframe.v1.views -side left -anchor nw -fill x -expand 1
	menubutton $bsframe.v1.views.def -text "Views..." \
	    -menu $bsframe.v1.views.def.m \
	    -relief raised -padx 2 -pady 2 -width 10	
	Tooltip $bsframe.v1.views.def \
	  "Allows the user to easily specify that the viewer align the axes\n"\
          "such that they are perpendicular and/or horizontal to the viewer."
	create_view_menu $bsframe.v1.views.def.m
	pack $bsframe.v1.views.def -side left -pady 2 -padx 2 -fill x

	# Set Home View Frame
	frame $bsframe.v2 -relief groove -borderwidth 2
	pack $bsframe.v2 -side left -padx 2 -pady 2
	button $bsframe.v2.sethome -text "Set Home View" -padx 2 \
		-command "$this-c sethome" -width 15
	Tooltip $bsframe.v2.sethome \
	    "Tells the Viewer to remember the current camera position."	
	button $bsframe.v2.gohome -text "Go home" \
	    -command "$this-c gohome" -width 15
	Tooltip $bsframe.v2.gohome \
	    "Tells the Viewer to recall the last saved camera position."
	pack $bsframe.v2.sethome $bsframe.v2.gohome \
	    -side top -fill x -pady 2 -padx 4

	# Detach Frame button
	button $bsframe.more -text "+" -padx 3 \
		-font "-Adobe-Helvetica-bold-R-Normal-*-12-75-*" \
		-command "$this addMFrame $w"
	Tooltip $bsframe.more \
	    "Shows/hides the Viewer's geometry settings panel."
	pack $bsframe.more -pady 2 -padx 2 -anchor se -side right

	# Initialization of attachment
	toplevel $w.detached
	frame $w.detached.f
	pack $w.detached.f -side top -anchor w -fill y -expand yes
	
	wm title $w.detached "VIEWWINDOW settings"
	wm sizefrom  $w.detached user
	wm positionfrom  $w.detached user
	wm protocol $w.detached WM_DELETE_WINDOW "$this removeMFrame $w"
	wm withdraw $w.detached
	
	# This is the frame for the geometry controls
	iwidgets::scrolledframe $w.msframe -width 640 -height 240 \
	    -vscrollmode dynamic -hscrollmode dynamic \
	    -sbwidth 10 -relief groove

	# get the childsite to add stuff to
	set msframe [$w.msframe childsite]

	frame $msframe.f -relief solid
	pack $msframe.f -side top -fill x

	set IsAttached 1
	set IsDisplayed 0
	
	set detachedFr $w.detached
	set attachedFr $w.msframe
	init_frame $detachedFr.f "Double-click here to attach - - - - - - - - - - - - - - - - - - - - -"
	init_frame $msframe.f "Double-click here to detach - - - - - - - - - - - - - - - - - - - - -"
	# End initialization of attachment

	switchvisual [set $this-currentvisual]
	trace variable $this-currentvisual w "$this currentvisualtrace"

	$this-c startup
	
	pack slaves $w

        # To avoid Mac bizarro behavior of not sizing the window correctly
        # this hack is necessary when loading from a script.
	if { [envBool SCI_REGRESSION_TESTING] } {
           # The added benefit of this is that I can make the Viewer Window
           # appear after all the other windows and thus on systems without
           # pbuffers, we don't get the drawing window obscured.  Three seconds
           # seems to be enough time.
           after 3000 "SciRaise $w"
        } else {
           SciRaise $w
        }
    }
    # end constructor()

    method bindEvents {w} {
	bind $w <Expose> "$this-c redraw"
	bind $w <Configure> "$this-c redraw"

	bind $w <ButtonPress-1> "$this-c mtranslate start %x %y"
	bind $w <Button1-Motion> "$this-c mtranslate move %x %y"
	bind $w <ButtonRelease-1> "$this-c mtranslate end %x %y"
	bind $w <ButtonPress-2> "$this-c mrotate start %x %y %t"
	bind $w <Button2-Motion> "$this-c mrotate move %x %y %t"
	bind $w <ButtonRelease-2> "$this-c mrotate end %x %y %t"
	bind $w <ButtonPress-3> "$this-c mscale start %x %y"
	bind $w <Button3-Motion> "$this-c mscale move %x %y"
	bind $w <ButtonRelease-3> "$this-c mscale end %x %y"

	bind $w <Control-ButtonPress-1> "$this-c mdolly start %x %y"
	bind $w <Control-Button1-Motion> "$this-c mdolly move %x %y"
	bind $w <Control-ButtonRelease-1> "$this-c mdolly end %x %y"
	bind $w <Control-ButtonPress-2> "$this-c mrotate_eyep start %x %y %t"
	bind $w <Control-Button2-Motion> "$this-c mrotate_eyep move %x %y %t"
	bind $w <Control-ButtonRelease-2> "$this-c mrotate_eyep end %x %y %t"
	bind $w <Control-ButtonPress-3> "$this-c municam start %x %y %t"
	bind $w <Control-Button3-Motion> "$this-c municam move %x %y %t"
	bind $w <Control-ButtonRelease-3> "$this-c municam end %x %y %t"

	bind $w <Shift-ButtonPress-1> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-ButtonPress-2> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-ButtonPress-3> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-Button1-Motion> "$this-c mpick move %x %y %s 1"
	bind $w <Shift-Button2-Motion> "$this-c mpick move %x %y %s 2"
	bind $w <Shift-Button3-Motion> "$this-c mpick move %x %y %s 3"
	bind $w <Shift-ButtonRelease-1> "$this-c mpick end %x %y %s %b"
	bind $w <Shift-ButtonRelease-2> "$this-c mpick end %x %y %s %b"
	bind $w <Shift-ButtonRelease-3> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonPress-1> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-ButtonPress-2> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-ButtonPress-3> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-Button1-Motion> "$this-c mpick move %x %y %s 1"
	bind $w <Lock-Button2-Motion> "$this-c mpick move %x %y %s 2"
	bind $w <Lock-Button3-Motion> "$this-c mpick move %x %y %s 3"
	bind $w <Lock-ButtonRelease-1> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonRelease-2> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonRelease-3> "$this-c mpick end %x %y %s %b"
    }

    method create_other_viewers_view_menu { m } {
	if { [winfo exists $m] } {
	    destroy $m
	}
	menu $m
	set myparts [split [modname] -]
	set myviewer .ui[lindex $myparts 0]
	set mywindow [lindex $myparts 1]
	set actual 0
	foreach w [winfo children .] {
	    set parts [split $w -]
	    set viewer [lindex $parts 0]
	    set window [lindex $parts 1]
	    if { [string equal $myviewer $viewer] } {
		if { ![string equal $mywindow $window] } {
		    set num [lindex [split $window _] end]
		    $m add command -label "Get View from Viewer $num" \
			-command "set $this-pos ViewWindow$actual; \
                                  $this-c Views"
		}
		incr actual
	    }
	}
    }
		    
    method create_view_menu { m } {
	menu $m -postcommand \
	    "$this create_other_viewers_view_menu $m.otherviewers"
	$m add checkbutton -label "Track View Window 0" \
	    -variable $this-trackViewWindow0
	$m add cascade -menu $m.otherviewers -label "Other Viewers"

	foreach sign1 {1 0} {
	    foreach dir1 {x y z} {
		set pn1 [expr $sign1?"+":"-"]
		set posneg1 [expr $sign1?"+":"-"]
		set sub $m.$posneg1$dir1
		$m add cascade -menu $sub \
		    -label "Look down $pn1[string toupper $dir1] Axis"
		menu $sub
		foreach dir2 { x y z } {
		    if { ![string equal $dir1 $dir2] } {
			foreach sign2 { 1 0 } {
			    set pn2 [expr $sign2?"+":"-"]
			    $sub add command -label \
				"Up vector $pn2[string toupper $dir2]" \
				-command "setGlobal $this-pos ${dir1}${sign1}_${dir2}${sign2};
                                              $this-c Views" 
			}
		    }
		}
	    }
	    $m add separator
	}
    }

    method removeMFrame {w} {
	if { $IsAttached != 0 } {
	    pack forget $attachedFr
	    set height [expr [winfo height $w]-[winfo height $w.msframe]]
	    wm geometry $w [winfo width $w]x${height}
	    update
	} else { 
	    wm withdraw $detachedFr
	}
	
	set bsframe [$w.bsframe childsite]
	$bsframe.more configure -command "$this addMFrame $w" -text "+"
	set IsDisplayed 0
    }
    
    method addMFrame {w} {
	if { $IsAttached!=0} {
	    pack $attachedFr -anchor w -side bottom -before $w.bsframe -fill x
	    set w1 [winfo width $w]
	    set w2 [winfo width $w.msframe]
	    set width [expr $w1 > $w2 ? $w1 : $w2]
	    set height [expr [winfo height $w]+[winfo reqheight $w.msframe]]
	    wm geometry $w ${width}x${height}
	    update
	} else {
	    wm deiconify $detachedFr
	}
	set bsframe [$w.bsframe childsite]
	$bsframe.more configure -command "$this removeMFrame $w" -text "-"
	set IsDisplayed 1
    }

    method init_frame {m msg} {
	if { ![winfo exists $m] } return
	
	bind $m <Double-ButtonPress-1> "$this switch_frames"

	# Label indicating click here to detach/attach
	label $m.cut -anchor w -text $msg -font \
	    "-Adobe-Helvetica-bold-R-Normal-*-12-75-*"
	pack $m.cut -side top -anchor w -pady 5 -padx 5
	bind $m.cut <Double-ButtonPress-1> "$this switch_frames"
	
	frame $m.eframe
	
	# Global Lighting Checkbutton
	checkbutton $m.eframe.light -text "Lighting" \
	    -variable $this-global-light -command "$this-c redraw"
	Tooltip $m.eframe.light \
	    "Toggles on/off whether lights effect the rendering."

	# Global Fog Checkbutton
	checkbutton $m.eframe.fog -text "Fog" \
	    -variable $this-global-fog -command "$this-c redraw"
	Tooltip $m.eframe.fog \
	    "Toggles on/off fog.  This will make objects further\n" \
	    "away from the viewer appear dimmer and make it easier\n" \
	    "to judge distances."

	# Global BBox Checkbutton
	checkbutton $m.eframe.bbox -text "BBox" \
	    -variable $this-global-debug  -command "$this-c redraw"
	Tooltip $m.eframe.bbox \
	    "Toggles on/off whether only the bounding box of every piece\n" \
	    "of geometry is displayed.  Individual bounding boxes may be\n" \
	    "toggled on/off using the 'Options' button in the 'Objects' frame."

	# Global Clip Checkbutton
	checkbutton $m.eframe.clip -text "Use Clip" \
	    -variable $this-global-clip -command "$this-c redraw"
	Tooltip $m.eframe.clip "Toggles on/off whether clipping is enabled."

	# Global Cull Checkbutton
	checkbutton $m.eframe.cull -text "Back Cull" \
	    -variable $this-global-cull -command "$this-c redraw"
	Tooltip $m.eframe.cull \
	    "Toggles on/off whether polygons that face away from\n" \
	    "the camera are rendered."

	# Global Display List Checkbutton
	checkbutton $m.eframe.dl -text "Display List" \
	    -variable $this-global-dl -command "$this-c redraw"
	Tooltip $m.eframe.dl \
	    "Toggles on/off whether GL display lists are used."
	
	pack $m.eframe -anchor n -padx 2 -side left
	pack  $m.eframe.light $m.eframe.fog $m.eframe.bbox $m.eframe.clip \
	    $m.eframe.cull $m.eframe.dl -in $m.eframe -side top -anchor w
	  
	# Render Style Radio Buttons
	frame $m.eframe.separator -relief sunken -borderwidth 4 -height 2
	radiobutton $m.eframe.wire -text Wire -value Wire \
	    -variable $this-global-type -command "$this-c redraw"
	radiobutton $m.eframe.flat -text Flat -value Flat \
	    -variable $this-global-type -command "$this-c redraw"
	radiobutton $m.eframe.gouraud -text Gouraud -value Gouraud \
	    -variable $this-global-type -command "$this-c redraw"

	pack $m.eframe.separator -in $m.eframe \
	    -side top -anchor w -expand y -fill x
	pack $m.eframe.wire $m.eframe.flat $m.eframe.gouraud \
	    -in $m.eframe -side top -anchor w
	    
	# Geometry Objects Options List
	frame $m.objlist -relief groove -borderwidth 2
	pack $m.objlist -side left -padx 2 -pady 2 -fill y -expand yes
	label $m.objlist.title -text "Objects:"
	pack $m.objlist.title -side top
	canvas $m.objlist.canvas -width 370 -height 128 \
	    -scrollregion "0 0 370 128" -borderwidth 0 \
	    -xscrollcommand "$m.objlist.xscroll set" -xscrollincrement 10 \
	    -yscrollcommand "$m.objlist.yscroll set" -yscrollincrement 10
	
	frame $m.objlist.canvas.frame -relief sunken -borderwidth 2
	pack $m.objlist.canvas.frame
	$m.objlist.canvas create window 0 1 \
	    -window $m.objlist.canvas.frame -anchor nw
	
	# Scrollbars for Geometry Objects Options List
	scrollbar $m.objlist.xscroll -relief sunken -orient horizontal \
		-command "$m.objlist.canvas xview"
	scrollbar $m.objlist.yscroll -relief sunken -orient vertical \
		-command "$m.objlist.canvas yview"
	pack $m.objlist.yscroll -fill y -side left -padx 2 -pady 2
	pack $m.objlist.canvas -side top -padx 2 -pady 2 -fill both -expand yes
	pack $m.objlist.xscroll -fill x -side top  -padx 2 -pady 2
	
        # CollabVis code begin
        if {[set $this-have_collab_vis]} {
	    checkbutton $m.view_server -text "Remote" -variable \
                $this-view_server -onvalue 2 -offvalue 0 \
                -command "$this-c doServer"
	    pack $m.view_server -side top
        }
	# CollabVis code end

	# Show Axes Check Button
        checkbutton $m.caxes -text "Show Axes" -variable $this-caxes \
	    -onvalue 1 -offvalue 0 \
	    -command "$this-c centerGenAxes; $this-c redraw"
	Tooltip $m.caxes \
	    "Toggles on/off the the set of three axes displayed at 0,0,0."

	# Orientation Axes Checkbutton
        checkbutton $m.raxes -text "Orientation" -variable $this-raxes \
	    -onvalue 1 -offvalue 0 \
	    -command "$this-c rotateGenAxes; $this-c redraw"
	Tooltip $m.raxes \
	    "Toggles on/off the orientation axes displayed in\n" \
	    "the upper right corner of the viewer window."

	# Ortho View Checkbutton
	checkbutton $m.ortho -text "Ortho View" -variable $this-ortho-view \
	    -onvalue 1 -offvalue 0 -command "$this-c redraw"
	Tooltip $m.ortho  \
	    "Toggles on/off the use of an orthographic projection.\n" \
	    "SCIRun defaults to using the prospective projection."

	pack $m.caxes -side top -anchor w
	pack $m.raxes -side top -anchor w
	pack $m.ortho -side top -anchor w
    
	# Stereo View Options
	checkbutton $m.stereo -text "Stereo" -variable $this-do_stereo \
		-command "$this-c redraw"
	Tooltip $m.stereo \
	    "Switch into stereo rendering mode.  Special hardware may be\n" \
	    "necessary to use this function."
	pack $m.stereo -side top -anchor w

	# Stereo Fusion Scale
	scale $m.sbase -variable $this-sbase -length 100 -from 0.1 -to 2 \
		-resolution 0.02 -orient horizontal -label "Fusion Scale:" \
		-command "$this-c redraw"
	Tooltip $m.sbase \
	    "Specifies how far the left and right eye images are\n" \
	    "offset when rendering in stereo mode."
	pack $m.sbase -side top -anchor w
    }

    method resize {} {
	set w .ui[modname]
	set wmovie .ui[modname]-saveMovie

	if { [set $this-global-resize] == 0 } {
	    wm geometry $w "="
	    pack configure $w.wframe -expand yes -fill both

	    set color "#505050"
	    if { $IsAttached == 1 } {
		$wmovie.x configure -foreground $color
		$wmovie.e1 configure -state disabled -foreground $color
		$wmovie.e2 configure -state disabled -foreground $color
	    } else {
		set m $w.detached.f
		$wmovie.x configure -foreground $color
		$wmovie.e1 configure -state disabled -foreground $color
		$wmovie.e2 configure -state disabled -foreground $color
	    }
	} else {
	    if { $IsAttached == 1 } { $this switch_frames }
	    set xsize [set $this-x-resize]
	    set ysize [set $this-y-resize]
	    set size "$xsize\x$ysize"
	    set xsize [expr $xsize + 14]
	    set ysize [expr $ysize + 123]
	    set geomsize "$xsize\x$ysize"
	    wm geometry $w "=$geomsize"
	    pack configure $w.wframe -expand no -fill none
	    $w.wframe.draw configure -geometry $size
	    $wmovie.x configure -foreground black
	    $wmovie.e1 configure -state normal -foreground black
	    $wmovie.e2 configure -state normal -foreground black
	}
    }

    method switch_frames {} {
	set w .ui[modname]
	if { $IsDisplayed } {
	    if { $IsAttached!=0} {
		pack forget $attachedFr
		set hei [expr [winfo height $w]-[winfo reqheight $w.msframe]]
		append geom [winfo width $w]x${hei}
		wm geometry $w $geom
		wm deiconify $detachedFr
		set IsAttached 0
	    } else {
		wm withdraw $detachedFr
		pack $attachedFr -anchor w -side bottom \
		    -before $w.bsframe -fill x
		set hei [expr [winfo height $w]+[winfo reqheight $w.msframe]]
		append geom [winfo width $w]x${hei}
		wm geometry $w $geom
		set IsAttached 1
	    }
	    update
	}
    }

    method updatePerf { p1 p2 p3 } {
	set w .ui[modname]
	set bsframe [$w.bsframe childsite]
	$bsframe.pf.perf1 configure -text $p1
	$bsframe.pf.perf2 configure -text $p2
	$bsframe.pf.perf3 configure -text $p3
    }

    method currentvisualtrace { args } {
	upvar \#0 $this-currentvisual visual $this-currentvisualhelper helper
	if { $visual != $helper } {
	    switchvisual $visual
	}
    }

    method switchvisual { idx } {
	set w .ui[modname]
	if { [winfo exists $w.wframe.draw] } {
	    destroy $w.wframe.draw
	}
	$this-c switchvisual $w.wframe.draw $idx 640 512
	if { [winfo exists $w.wframe.draw] } {
	    bindEvents $w.wframe.draw
	    pack $w.wframe.draw -expand yes -fill both
	}
	upvar \#0 $this-currentvisual visual $this-currentvisualhelper helper
	set helper $visual
    }	

    method makeViewPopup {} {
	set w .view[modname]

	if { [winfo exists $w] } {
	    SciRaise $w
	    return
	}

	toplevel $w
	wm title $w "View"
	wm iconname $w view
	wm minsize $w 100 100
	set view $this-view
	makePoint $w.eyep "Eye Point" $view-eyep "$this-c redraw"
	pack $w.eyep -side left -expand yes -fill x
	makePoint $w.lookat "Look at Point" $view-lookat "$this-c redraw"
	pack $w.lookat -side left -expand yes -fill x
	makeNormalVector $w.up "Up Vector" $view-up "$this-c redraw"
	pack $w.up -side left -expand yes -fill x
	global $view-fov
	frame $w.f -relief groove -borderwidth 2
	pack $w.f
	scale $w.f.fov -label "Field of View:"  -variable $view-fov \
	    -orient horizontal -from 0 -to 180 -tickinterval 90 -digits 3 \
	    -showvalue true -command "$this-c redraw"
	    
	pack $w.f.fov -expand yes -fill x
    }

    method makeSceneMaterialsPopup {} {
	set w .scenematerials[modname]
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
	}

	toplevel $w
	wm title $w "Scene Materials"
	wm iconname $w materials
	wm minsize $w 100 100

	foreach property {ambient diffuse specular shininess emission} {
	    frame $w.$property
	    set text "[string totitle $property] Scale"
	    label $w.$property.l -width 16 -text $text -anchor w
	    entry $w.$property.e -relief sunken -width 6 \
		-textvariable $this-$property-scale
	    bind $w.$property.e <Return> "$this-c redraw"
	    pack $w.$property.l $w.$property.e -side left -fill x
	    pack $w.$property -side top -fill both
	}
    }

    method makeBackgroundPopup {} {
	set w .bg[modname]
	if [winfo exists $w] {
	    SciRaise $w
	    return
	}
	makeColorPicker $w $this-bgcolor "$this-c redraw" "destroy $w"
	wm title $w "Choose Background Color"
    }

    method updateMode {msg} {
	set bsframe [.ui[modname].bsframe childsite]
	$bsframe.mousemode itemconfigure mouseModeText -text $msg
    }   

    method addObject {objid name} {
	addObjectToFrame $objid $name $detachedFr
	set msframe [$attachedFr childsite]
	addObjectToFrame $objid $name $msframe
    }

    method addObjectToFrame {objid name frame} {
	set w .ui[modname]
	set m $frame.f
	frame  $m.objlist.canvas.frame.objt$objid
	checkbutton $m.objlist.canvas.frame.obj$objid -text $name \
		-relief flat -variable "$this-$name" -command "$this-c redraw"
	
	set newframeheight [winfo reqheight $m.objlist.canvas.frame.obj$objid]
	
	set menun $m.objlist.canvas.frame.menu$objid.menu

	menubutton $m.objlist.canvas.frame.menu$objid -text "Options..." \
		-relief raised -menu $menun
	menu $menun

	$menun add radiobutton -label "Use Global Controls" \
	    -variable $this-$objid-type -value "Default"\
	    -command "$this-c redraw"

	$menun add separator

	$menun add checkbutton -label Lighting -variable $this-$objid-light \
		-command "$this-c redraw"
	$menun add checkbutton -label BBox -variable $this-$objid-debug \
		-command "$this-c redraw"
	$menun add checkbutton -label Fog -variable $this-$objid-fog \
		-command "$this-c redraw"
	$menun add checkbutton -label "Use Clip" -variable $this-$objid-clip \
		-command "$this-c redraw"
	$menun add checkbutton -label "Back Cull" -variable $this-$objid-cull \
		-command "$this-c redraw"
	$menun add checkbutton -label "Display List" -variable $this-$objid-dl\
		-command "$this-c redraw"

	$menun add separator

	$menun add radiobutton -label Wire -variable $this-$objid-type \
	    -command "$this-c redraw"	    
	$menun add radiobutton -label Flat -variable $this-$objid-type \
	    -command "$this-c redraw"
	$menun add radiobutton -label Gouraud -variable $this-$objid-type \
	    -command "$this-c redraw"

	setGlobal "$this-$objid-type" Default
	setGlobal "$this-$objid-light" 1
	setGlobal "$this-$objid-fog" 0
	setGlobal "$this-$objid-debug" 0
	setGlobal "$this-$objid-clip" 1
	setGlobal "$this-$objid-cull" 0
	setGlobal "$this-$objid-dl" 0

	pack $m.objlist.canvas.frame.objt$objid \
	    -side top -anchor w -fill x -expand y
	pack $m.objlist.canvas.frame.obj$objid \
	    -in $m.objlist.canvas.frame.objt$objid -side left
	pack $m.objlist.canvas.frame.menu$objid \
	    -in $m.objlist.canvas.frame.objt$objid -side right -padx 1 -pady 1

	update idletasks
	set width [winfo width $m.objlist.canvas.frame]
	set height [lindex [$m.objlist.canvas cget -scrollregion] end]

	incr height [expr $newframeheight+20]

	$m.objlist.canvas configure -scrollregion "0 0 $width $height"

	set view [$m.objlist.canvas xview]
	$m.objlist.xscroll set [lindex $view 0] [lindex $view 1]

	set view [$m.objlist.canvas yview]
	$m.objlist.yscroll set [lindex $view 0] [lindex $view 1]
    }

    method addObject2 {objid} {
	addObjectToFrame_2 $objid $detachedFr
	set msframe [$attachedFr childsite]
	addObjectToFrame_2 $objid $msframe
    }
    
    method addObjectToFrame_2 {objid frame} {
	set w .ui[modname]
	set m $frame.f
	pack $m.objlist.canvas.frame.objt$objid \
	    -side top -anchor w -fill x -expand y
	pack $m.objlist.canvas.frame.obj$objid  \
	    -in $m.objlist.canvas.frame.objt$objid -side left
	pack $m.objlist.canvas.frame.menu$objid \
	    -in $m.objlist.canvas.frame.objt$objid -side right -padx 1 -pady 1
    }
    
    method removeObject {objid} {
	removeObjectFromFrame $objid $detachedFr
	set msframe [$attachedFr childsite]
	removeObjectFromFrame $objid $msframe
    }

    method removeObjectFromFrame {objid frame} {
	set w .ui[modname]
	set m $frame.f
	pack forget $m.objlist.canvas.frame.objt$objid
    }

    method makeLineWidthPopup {} {
	set w .lineWidth[modname]
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
	}
	toplevel $w
	wm title $w "Line Width"
	wm minsize $w 250 100
	frame $w.f
	scale $w.f.scale -label "Line Width:" -command "$this-c redraw" \
	    -variable $this-line-width -orient horizontal -from 1 -to 5 \
	    -resolution .1 -showvalue true -tickinterval 1 -digits 0
	    
	pack $w.f.scale -fill x -expand 1
	pack $w.f -fill x -expand 1
    }	

    method makePolygonOffsetPopup {} {
	set w .polygonOffset[modname]
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
	}
	toplevel $w
	wm title $w "Polygon Offset"
	wm minsize $w 250 100
	frame $w.f
	scale $w.f.factor -label "Offset Factor:" -command "$this-c redraw" \
	    -variable $this-polygon-offset-factor \
	    -orient horizontal -from -4 -to 4 \
	    -resolution .01 -showvalue true -tickinterval 2 -digits 3
	    
	scale $w.f.units -label "Offset Units:" -command "$this-c redraw" \
	    -variable $this-polygon-offset-units \
	    -orient horizontal -from -4 -to 4 -resolution .01 \
	    -showvalue true -tickinterval 2 -digits 3
	    
	pack $w.f.factor -fill x -expand 1
	pack $w.f -fill x -expand 1
    }	

    method makePointSizePopup {} {
	set w .psize[modname]
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
	}
	toplevel $w
	wm title $w "Point Size"
	wm minsize $w 250 100 
	frame $w.f
	scale $w.f.scale -label "Pixel Size:" -command "$this-c redraw" \
	    -variable $this-point-size -orient horizontal -from 1 -to 5 \
	    -resolution .1 -showvalue true -tickinterval 1 -digits 0
	    
	pack $w.f.scale -fill x -expand 1
	pack $w.f -fill x -expand 1
    }	

    method makeClipPopup {} {
	set w .clip[modname]
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
	}
	toplevel $w
	wm title $w "Clipping Planes"
	wm minsize $w 200 100 

	initGlobal $this-clip-num 6
	initGlobal $this-clip-selected 1
	for {set i 1} {$i <= 6} {incr i 1} {
	    initGlobal $this-clip-visible-$i 0
	    initGlobal $this-clip-normal-d-$i 0.0
	    initGlobal $this-clip-normal-x-$i 1.0
	    initGlobal $this-clip-normal-y-$i 0.0
	    initGlobal $this-clip-normal-z-$i 0.0
	}

	set menup [tk_optionMenu $w.which $this-clip-selected 1 2 3 4 5 6]
	for {set i 0}  {$i < 6} {incr i} {
	    $menup entryconfigure $i -command "$this useClip"
	}
	
	pack $w.which
	checkbutton $w.visibile -text "Visible" -variable $this-clip-visible \
	    -relief flat -command "$this setClip;$this-c redraw"
	pack $w.visibile

	makePlane $w.normal "Plane Normal" $this-clip-normal \
	    "$this setClip ; $this-c redraw"
	pack $w.normal -side left -expand yes -fill x
	frame $w.f -relief groove -borderwidth 2
	pack $w.f -expand yes -fill x

	useClip
    }

    method useClip {} {
	upvar \#0 $this-clip-selected cs
	upvar \#0 $this-clip-normal-x-$cs x $this-clip-normal-y-$cs y
	upvar \#0 $this-clip-normal-z-$cs z $this-clip-normal-d-$cs d
 	upvar \#0 $this-clip-visible-$cs visible
	
	setGlobal $this-clip-normal-x $x
	setGlobal $this-clip-normal-y $y
	setGlobal $this-clip-normal-z $z
	setGlobal $this-clip-visible  $visible
	.clip[modname].normal.e newvalue $d
    }

    method setClip {} {
	upvar \#0 $this-clip-selected cs
	upvar \#0 $this-clip-normal-x x $this-clip-normal-y y
	upvar \#0 $this-clip-normal-z z $this-clip-normal-d d
 	upvar \#0 $this-clip-visible visible

	setGlobal $this-clip-normal-x-$cs $x
	setGlobal $this-clip-normal-y-$cs $y
	setGlobal $this-clip-normal-z-$cs $z
	setGlobal $this-clip-normal-d-$cs $d
	setGlobal $this-clip-visible-$cs  $visible
    }

    method makeLightSources {} {
	set w .ui[modname]-lightSources

        if {[winfo exists $w]} {
	    SciRaise $w
            return
        }
	toplevel $w ;# create the window        
	wm withdraw $w ;# immediately withdraw it to avoid flicker
        wm title $w "Light Position and Colors"
	frame $w.tf -relief flat
	pack $w.tf -side top
	for {set i 0} {$i < 4} {incr i 1} {
	    $this makeLightControl $w.tf $i
	}

	label $w.l -text \
	    "Click on number to move light. Note: Headlight will not move."
	label $w.o -text \
	    "Click in circle to change light color/brightness"

 	button $w.breset -text "Reset Lights" -command "$this resetLights $w"
	button $w.bclose -text Close -command "destroy $w"
	pack $w.l $w.o $w.breset $w.bclose -side top -expand yes -fill x

	moveToCursor $w "leave_up"
	wm deiconify $w
    }
	
    method makeLightControl { w i } {
	frame $w.f$i -relief flat
	pack $w.f$i -side left
	canvas $w.f$i.c -bg "#BDBDBD" -width 100 -height 100
	pack $w.f$i.c -side top
	set c $w.f$i.c
	checkbutton $w.f$i.b$i -text "on/off" \
	    -variable $this-global-light$i -command "$this lightSwitch $i"
	pack $w.f$i.b$i

	upvar $this-lightColors lightColors $this-lightVectors lightVectors
	set ir [expr int([lindex [lindex $lightColors $i] 0] * 65535)]
	set ig [expr int([lindex [lindex $lightColors $i] 1] * 65535)]
	set ib [expr int([lindex [lindex $lightColors $i] 2] * 65535)]
       
	set window .ui[modname]
	set color [format "#%04x%04x%04x" $ir $ig $ib]
	set news [$c create oval 5 5 95 95 -outline "#000000" \
		      -fill $color -tags lc ]

	set x [expr int([lindex [lindex $lightVectors $i] 0] * 50) + 50]
	set y [expr int([lindex [lindex $lightVectors $i] 1] * -50) + 50]
	set t  $i
	if { $t == 0 } { set t "HL" }
	set newt [$c create text $x $y -fill "#555555" -text $t -tags lname ]
	$c bind lname <B1-Motion> "$this moveLight $c $i %x %y"
	$c bind lc <ButtonPress-1> "$this lightColor $w $c $i"
	$this lightSwitch $i
    }

    method lightColor { w c i } {
	upvar \#0 $this-lightColors lightColors
 	setGlobal $this-def-color-r [lindex [lindex $lightColors $i] 0]
 	setGlobal $this-def-color-g [lindex [lindex $lightColors $i] 1]
 	setGlobal $this-def-color-b [lindex [lindex $lightColors $i] 2]
	if { [winfo exists $w.color] } { destroy $w.color } 
	makeColorPicker $w.color $this-def-color \
	    "$this setColor $w.color $c $i $this-def-color " \
	    "destroy $w.color"
    }

    method setColor { w c i color} {
	upvar \#0 $color-r r $color-g g $color-b b $this-lightColors lColors
	set lColors [lreplace $lColors $i $i "$r $g $b"]
	
	set ir [expr int($r * 65535)]
	set ig [expr int($g * 65535)]
	set ib [expr int($b * 65535)]
	
	set window .ui[modname]
	$c itemconfigure lc -fill [format "#%04x%04x%04x" $ir $ig $ib]
	$this lightSwitch $i
    }
    
    method resetLights { w } {
	upvar $this-lightColors lCol $this-lightVectors lVec
	for { set i 0 } { $i < 4 } { incr i 1 } {
	    if { $i == 0 } {
		set $this-global-light$i 1
		set c $w.tf.f$i.c
		$c itemconfigure lc -fill \
		    [format "#%04x%04x%04x" 65535 65535 65535 ]
		set lCol [lreplace $lCol $i $i [list 1.0 1.0 1.0]]
		$this lightSwitch $i
	    } else {
		set $this-global-light$i 0
		set coords [$w.tf.f$i.c coords lname]
		set curX [lindex $coords 0]
		set curY [lindex $coords 1]
		set xn [expr 50 - $curX]
		set yn [expr 50 - $curY]
		$w.tf.f$i.c move lname $xn $yn
		set vec [list 0 0 1 ]
		set $this-lightVectors \
		    [lreplace [set $this-lightVectors] $i $i $vec]
		$w.tf.f$i.c itemconfigure lc -fill \
		    [format "#%04x%04x%04x" 65535 65535 65535 ]
		set lightColor [list 1.0 1.0 1.0]
		set $this-lightColors \
		    [lreplace [set $this-lightColors] $i $i $lightColor]
		$this lightSwitch $i
	    }
	}
    }

    method moveLight { c i x y } {
	if { $i == 0 } return
	upvar $this-global-light$i light lCol $this-lightVectors lVec
	set cw [winfo width $c]
	set ch [winfo height $c]
	set selected [$c find withtag current]
	set coords [$c coords current]
	set curX [lindex $coords 0]
	set curY [lindex $coords 1]
	set xn $x
	set yn $y
	set len2 [expr (( $x-50 )*( $x-50 ) + ($y-50) * ($y-50))]
	if { $len2 < 2025 } { 
	    $c move $selected [expr $xn-$curX] [expr $yn-$curY]
	} else { 
	    # keep the text inside the circle
	    set scale [expr 45 / sqrt($len2)]
	    set xn [expr 50 + ($x - 50) * $scale]
	    set yn [expr 50 + ($y - 50) * $scale]
	    $c move $selected [expr $xn-$curX] [expr $yn-$curY]
	}
	set newz [expr ($len2 >= 2025) ? 0 : sqrt(2025 - $len2)]
	set newx [expr $xn - 50]
	set newy [expr $yn - 50]
	# normalize the vector
	set len3 [expr sqrt($newx*$newx + $newy*$newy + $newz*$newz)]
	set vec "[expr $newx/$len3] [expr -$newy/$len3] [expr $newz/$len3]"
	set lVec [lreplace [set $this-lVec] $i $i $vec]
	if { $light } {
	    $this lightSwitch $i
	}
    }

    method lightSwitch {i} {
	upvar \#0 $this-global-light$i light $this-lightVectors lVec
	upvar \#0 $this-lightColors lCol
	$this-c edit_light $i $light [lindex $lVec $i] [lindex $lCol $i]
    }

    method traceLight {which name1 name2 op } {
	set w .ui[modname]-lightSources
	if {![winfo exists $w]} {
	    $this lightSwitch $which
	}
    }

    method traceGeom { args } {
	upvar \#0 $this-geometry geometry
	wm geometry .ui[modname] $geometry
    }

    method makeSaveImagePopup {} {
	upvar \#0 $this-resx resx $this-resy resy
	set resx [winfo width .ui[modname].wframe.draw]
	set resy [winfo height .ui[modname].wframe.draw]
	
	set w .ui[modname]-saveImage
	if {[winfo exists $w]} {
	    SciRaise $w
	    return
        }
	toplevel $w -class TkFDialog

	set initdir [pwd]
	set defext "" ;# extension to append if no extension supplied by user
	
	set defname "MyImage.ppm" ;# filename to appear initially
	set title "Save ViewWindow Image"

	# file types to appers in filter box
	set types {
	    {{All Files}    {.*}}
	    {{PPM File}     {.ppm}}
	    {{Raw File}     {.raw}}
	}
	
	makeSaveFilebox \
	    -parent $w \
	    -filevar $this-saveFile \
	    -command "$this doSaveImage; wm withdraw $w" \
	    -commandname Save \
	    -cancel "wm withdraw $w" \
	    -title $title \
	    -filetypes $types \
	    -initialfile $defname \
	    -initialdir $initdir \
	    -defaultextension $defext \
	    -formatvar $this-saveType \
	    -formats {ppm raw "by_extension"} \
	    -imgwidth $this-resx \
	    -imgheight $this-resy
	moveToCursor $w
	wm deiconify $w
    }
    
    method doSaveImage {} {
	upvar \#0 $this-saveFile file $this-saveType type
	upvar \#0 $this-resx resx $this-resy resy
	$this-c dump_viewwindow $file $type $resx $resy
	$this-c redraw
    }

    method makeSaveMoviePopup {} {
	set w .ui[modname]-saveMovie

	# check license env var
	if { [$this-c have_mpeg] && ![envBool SCIRUN_MPEG_LICENSE_ACCEPT]} {
	    tk_messageBox -message "License information describing the mpeg_encode software can be found in SCIRun's Thirdparty directory, in the mpeg_encode/README file.\n\nThe MPEG software is freely distributed and may be used for any non-commercial purpose.  However, patents are held by several companies on various aspects of the MPEG video standard. Companies or individuals who want to develop commercial products that include this code must acquire licenses from these companies. For information on licensing, see Appendix F in the standard. For more information, please see the mpeg_encode README file.\n\nIf you are allowed to use the MPEG functionality based on the above license, you may enable MPEG movie recording in SCIRun (accessible via the SCIRun Viewer's \"File->Record Movie\" menu) by setting the value of SCIRUN_MPEG_LICENSE_ACCEPT to \"true\". This can be done by uncommenting the reference to the SCIRUN_MPEG_LICENSE_ACCEPT variable in your scirunrc and changing the value from false to true." -type ok -icon info -parent .ui[modname] -title "MPEG License"
	    return
	}

	if {[winfo exists $w]} {
	   SciRaise $w
           return
        }
	toplevel $w

        wm title $w "Record Movie"

	label $w.l -text "Record Movie as:"
        pack $w.l -side top -anchor w
	checkbutton $w.resize -text "Resize: " \
	    -variable $this-global-resize \
	    -offvalue 0 -onvalue 1 -command "$this resize; $this-c redraw"
	entry $w.e1 -textvariable $this-x-resize -width 4
	label $w.x -text x
	entry $w.e2 -textvariable $this-y-resize -width 4

        radiobutton $w.none -text "Stop Recording" \
            -variable $this-global-movie -value 0 -command "$this-c redraw"
	radiobutton $w.raw -text "PPM Frames" \
            -variable $this-global-movie -value 1 -command "$this-c redraw"
	radiobutton $w.mpeg -text "Mpeg" \
	    -variable $this-global-movie -value 2 -command "$this-c redraw"
	
	if { ![$this-c have_mpeg] } {
	    $w.mpeg configure -state disabled -disabledforeground ""
	} 

	frame $w.moviebase
	label $w.moviebase.label -text "Name:" -width 6
        entry $w.moviebase.entry -relief sunken -width 13 \
	    -textvariable "$this-global-movieName" 
        pack $w.moviebase.label $w.moviebase.entry -side left

	frame $w.movieframe
	label $w.movieframe.label -text "Frame:" -width 6
        entry $w.movieframe.entry -relief sunken -width 13 \
	    -textvariable "$this-global-movieFrame" 
        pack $w.movieframe.label $w.movieframe.entry -side left

        pack $w.none $w.raw $w.mpeg -side top -anchor w
        pack $w.moviebase  -side top -anchor w -pady 5 
        pack $w.movieframe -side top -anchor w -pady 2
	pack $w.resize $w.e1 $w.x $w.e2 -side left -anchor w -pady 5

	bind $w.e1 <Return> "$this resize"
	bind $w.e2 <Return> "$this resize"
	if {[set $this-global-resize] == 0} {
	    set color "#505050"
	    $w.x configure -foreground $color
	    $w.e1 configure -state disabled -foreground $color
	    $w.e2 configure -state disabled -foreground $color
	}
    }

    # TODO: Remvoe this method
    method setFrameRate { args } {
    }
}


itcl_class EmbeddedViewWindow {
    public viewer
    
    method modname {} {
	set n $this
	if {[string first "::" "$n"] == 0} {
	    set n "[string range $n 2 end]"
	}
	return $n
    }

    method set_defaults {} {

	# set defaults values for parameters that weren't set in a script

        # CollabVis code begin 
        global $this-have_collab_vis 
        # CollabVis code end

	global $this-saveFile
	global $this-saveType
	if {![info exists $this-File]} {set $this-saveFile "out.raw"}
	if {![info exists $this-saveType]} {set $this-saveType "raw"}

	# Animation parameters
	global $this-current_time
	if {![info exists $this-current_time]} {set $this-current_time 0}
	global $this-tbeg
	if {![info exists $this-tbeg]} {set $this-tbeg 0}
	global $this-tend
	if {![info exists $this-tend]} {set $this-tend 1}
	global $this-framerate
	if {![info exists $this-framerate]} {set $this-framerate 15}
	global $this-totframes
	if {![info exists $this-totframes]} {set $this-totframes 30}
	global $this-caxes
        if {![info exists $this-caxes]} {set $this-caxes 0}
	global $this-raxes
        if {![info exists $this-raxes]} {set $this-raxes 1}

	# Need to initialize the background color
	global $this-bgcolor-r
	if {![info exists $this-bgcolor-r]} {set $this-bgcolor-r 0}
	global $this-bgcolor-g
	if {![info exists $this-bgcolor-g]} {set $this-bgcolor-g 0}
	global $this-bgcolor-b
	if {![info exists $this-bgcolor-b]} {set $this-bgcolor-b 0}

	# Need to initialize the scene material scales
	global $this-ambient-scale
	if {![info exists $this-ambient-scale]} {set $this-ambient-scale 1.0}
	global $this-diffuse-scale
	if {![info exists $this-diffuse-scale]} {set $this-diffuse-scale 1.0}
	global $this-specular-scale
	if {![info exists $this-specular-scale]} {set $this-specular-scale 0.4}
	global $this-emission-scale
	if {![info exists $this-emission-scale]} {set $this-emission-scale 1.0}
	global $this-shininess-scale
	if {![info exists $this-shininess-scale]} {set $this-shininess-scale 1.0}
	# Initialize point size, line width, and polygon offset
	global $this-point-size
	if {![info exists $this-point-size]} {set $this-point-size 1.0}
	global $this-line-width
	if {![info exists $this-line-width]} {set $this-line-width 1.0}
	global $this-polygon-offset-factor
 	if {![info exists $this-polygon-offset-factor]} \
	    {set $this-polygon-offset-factor 1.0}
	global $this-polygon-offset-units
	if {![info exists $this-polygon-offset-units]} \
	    {set $this-polygon-offset-units 0.0}

	# Set up lights
	global $this-global-light0 # light 0 is the head light
	if {![info exists $this-global-light0]} { set $this-global-light0 1 }
	global $this-global-light1 
	if {![info exists $this-global-light1]} { set $this-global-light1 0 }
	global $this-global-light2 
	if {![info exists $this-global-light2]} { set $this-global-light2 0 }
	global $this-global-light3 
	if {![info exists $this-global-light3]} { set $this-global-light3 0 }
# 	global $this-global-light4 
# 	if {![info exists $this-global-light4]} { set $this-global-light4 0 }
# 	global $this-global-light5 
# 	if {![info exists $this-global-light5]} { set $this-global-light5 0 }
# 	global $this-global-light6
# 	if {![info exists $this-global-light6]} { set $this-global-light6 0 }
# 	global $this-global-light7 
# 	if {![info exists $this-global-light7]} { set $this-global-light7 0 }
	global $this-lightVectors
	if {![info exists $this-lightVectors]} { 
	    set $this-lightVectors \
		[list { 0 0 1 } { 0 0 1 } { 0 0 1 } { 0 0 1 }]
# 		     { 0 0 1 } { 0 0 1 } { 0 0 1 } { 0 0 1 }]
	}
	if {![info exists $this-lightColors]} {
	    set $this-lightColors \
		[list {1.0 1.0 1.0} {1.0 1.0 1.0} \
		     {1.0 1.0 1.0} {1.0 1.0 1.0} ]
# 		     {1.0 1.0 1.0} {1.0 1.0 1.0} \
# 		     {1.0 1.0 1.0} {1.0 1.0 1.0} ]
	}

	global $this-sbase
	if {![info exists $this-sbase]} {set $this-sbase 0.4}
	global $this-sr
	if {![info exists $this-sr]} {set $this-sr 1}
	global $this-do_stereo
	if {![info exists $this-do_stereo]} {set $this-do_stereo 0}

	global $this-def-color-r
	global $this-def-color-g
	global $this-def-color-b
	set $this-def-color-r 1.0
	set $this-def-color-g 1.0
	set $this-def-color-b 1.0

        # CollabVis code begin
        if {[set $this-have_collab_vis]} {
	    global $this-view_server
	    if {![info exists $this-view_server]} {set $this-view_server 0}
        }
        # CollabVis code end

	global $this-ortho-view
	if {![info exists $this-ortho-view]} { set $this-ortho-view 0 }
    }

    destructor {
    }

    constructor {config} {
	$viewer-c addviewwindow $this
	set_defaults
	init_frame
    }

    method setWindow {w width height} {
	$this-c listvisuals .standalone

	if {[winfo exists $w]} {
	    destroy $w
	}

	global emb_win
	set emb_win $w

	#$this-c switchvisual $w 0 640 670
	$this-c switchvisual $w 0 $width $height
	
	if {[winfo exists $w]} {
	    bindEvents $w
	}
	$this-c startup
    }

    method bindEvents {w} {
	bind $w <Expose> "$this-c redraw"
	bind $w <Configure> "$this-c redraw"

	bind $w <ButtonPress-1> "$this-c mtranslate start %x %y"
	bind $w <Button1-Motion> "$this-c mtranslate move %x %y"
	bind $w <ButtonRelease-1> "$this-c mtranslate end %x %y"
	bind $w <ButtonPress-2> "$this-c mrotate start %x %y %t"
	bind $w <Button2-Motion> "$this-c mrotate move %x %y %t"
	bind $w <ButtonRelease-2> "$this-c mrotate end %x %y %t"
	bind $w <ButtonPress-3> "$this-c mscale start %x %y"
	bind $w <Button3-Motion> "$this-c mscale move %x %y"
	bind $w <ButtonRelease-3> "$this-c mscale end %x %y"

	bind $w <Control-ButtonPress-1> "$this-c mdolly start %x %y"
	bind $w <Control-Button1-Motion> "$this-c mdolly move %x %y"
	bind $w <Control-ButtonRelease-1> "$this-c mdolly end %x %y"
	bind $w <Control-ButtonPress-2> "$this-c mrotate_eyep start %x %y %t"
	bind $w <Control-Button2-Motion> "$this-c mrotate_eyep move %x %y %t"
	bind $w <Control-ButtonRelease-2> "$this-c mrotate_eyep end %x %y %t"
	bind $w <Control-ButtonPress-3> "$this-c municam start %x %y %t"
	bind $w <Control-Button3-Motion> "$this-c municam move %x %y %t"
	bind $w <Control-ButtonRelease-3> "$this-c municam end %x %y %t"

	bind $w <Shift-ButtonPress-1> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-ButtonPress-2> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-ButtonPress-3> "$this-c mpick start %x %y %s %b"
	bind $w <Shift-Button1-Motion> "$this-c mpick move %x %y %s 1"
	bind $w <Shift-Button2-Motion> "$this-c mpick move %x %y %s 2"
	bind $w <Shift-Button3-Motion> "$this-c mpick move %x %y %s 3"
	bind $w <Shift-ButtonRelease-1> "$this-c mpick end %x %y %s %b"
	bind $w <Shift-ButtonRelease-2> "$this-c mpick end %x %y %s %b"
	bind $w <Shift-ButtonRelease-3> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonPress-1> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-ButtonPress-2> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-ButtonPress-3> "$this-c mpick start %x %y %s %b"
	bind $w <Lock-Button1-Motion> "$this-c mpick move %x %y %s 1"
	bind $w <Lock-Button2-Motion> "$this-c mpick move %x %y %s 2"
	bind $w <Lock-Button3-Motion> "$this-c mpick move %x %y %s 3"
	bind $w <Lock-ButtonRelease-1> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonRelease-2> "$this-c mpick end %x %y %s %b"
	bind $w <Lock-ButtonRelease-3> "$this-c mpick end %x %y %s %b"
    }

    method removeMFrame {w} {
    }
    
    method addMFrame {w} {
    }

    method init_frame {} {
	global $this-global-light
	global $this-global-fog
	global $this-global-type
	global $this-global-debug
	global $this-global-clip
	global $this-global-cull
	global $this-global-dl
	global $this-global-movie
	global $this-global-movieName
	global $this-global-movieFrame
	global $this-global-resize
	global $this-x-resize
	global $this-y-resize
	global $this-do_stereo
	global $this-sbase
	global $this-sr
	global $this-do_bawgl
	global $this-tracker_state
	
	set $this-global-light 1
	set $this-global-fog 0
	set $this-global-type Gouraud
	set $this-global-debug 0
	set $this-global-clip 1
	set $this-global-cull 0
	set $this-global-dl 0
	set $this-global-movie 0
	set $this-global-movieName "movie"
	set $this-global-movieFrame 0
	set $this-global-resize 0
	set $this-x-resize 700
	set $this-y-resize 512
	set $this-do_bawgl 0
	set $this-tracker_state 0
    }

    method resize { } {
    }

    method switch_frames {} {
    }

    method updatePerf {p1 p2 p3} {
    }

    method switchvisual {idx width height} {
	set w .ui[modname]
	if {[winfo exists $w.wframe.draw]} {
	    destroy $w.wframe.draw
	}
	$this-c switchvisual $w.wframe.draw $idx $width $height
	if {[winfo exists $w.wframe.draw]} {
	    bindEvents $w.wframe.draw
	    pack $w.wframe.draw -expand yes -fill both
	}
    }	

    method makeViewPopup {} {
    }

    method makeSceneMaterialsPopup {} {
    }

    method makeBackgroundPopup {} {
    }

    method updateMode {msg} {
    }   

    method addObject {objid name} {
	global "$this-$objid-light"
	global "$this-$objid-fog"
	global "$this-$objid-type"
	global "$this-$objid-debug"
	global "$this-$objid-clip"
	global "$this-$objid-cull"
	global "$this-$objid-dl"

	set "$this-$objid-type" Default
	set "$this-$objid-light" 1
	set "$this-$objid-fog" 0
	set "$this-$objid-debug" 0
	set "$this-$objid-clip" 0
	set "$this-$objid-cull" 0
	set "$this-$objid-dl" 0

	# $this-c autoview
    }

    method addObjectToFrame {objid name frame} {
    }

    method addObject2 {objid} {
	# $this-c autoview
    }
    
    method addObjectToFrame_2 {objid frame} {
    }
    

    method removeObject {objid} {
    }

    method removeObjectFromFrame {objid frame} {
    }

    method makeLineWidthPopup {} {
    }	

    method makePolygonOffsetPopup {} {
    }	

    method makePointSizePopup {} {
    }	

    method makeClipPopup {} {
    }

    method useClip {} {
    }

    method setClip {} {
    }

    method invertClip {} {
    }

    method makeAnimationPopup {} {
    }

    method setFrameRate {rate} {
    }

    method frametime {} {
    }

    method rstep {} {
    }

    method rew {} {
    }

    method rplay {} {
    }

    method play {} {
    }

    method step {} {
    }

    method ff {} {
    }

    method makeLightSources {} {
    }
	
    method makeLightControl { w i } {
    }

    method lightColor { w c i } {
    }

    method setColor { w c  i color} {
    }

    method resetLights { w } {
    }

    method moveLight { c i x y } {
    }

    method lightSwitch {i} {
    }
	
    method makeSaveImagePopup {} {
	global $this-saveFile
	global $this-saveType
	global $this-resx
	global $this-resy
	global $this-aspect
	global emb_win

	set $this-resx [winfo width $emb_win]
	set $this-resy [winfo height $emb_win]
	
	set w .ui[modname]-saveImage

	if {[winfo exists $w]} {
	    SciRaise $w
	    return
        }

	toplevel $w -class TkFDialog

	set initdir [pwd]

	#######################################################
	# to be modified for particular reader

	# extansion to append if no extension supplied by user
	set defext ""
	
	# name to appear initially
	set defname "MyImage.ppm"
	set title "Save ViewWindow Image"

	# file types to appers in filter box
	set types {
	    {{All Files}    {.*}}
	    {{Raw File}     {.raw}}
	    {{PPM File}     {.ppm}}
	}
	
	######################################################
	
	makeSaveFilebox \
		-parent $w \
		-filevar $this-saveFile \
		-command "$this doSaveImage; wm withdraw $w" \
		-cancel "wm withdraw $w" \
		-title $title \
		-filetypes $types \
	        -initialfile $defname \
		-initialdir $initdir \
		-defaultextension $defext \
		-formatvar $this-saveType \
                -formats {ppm raw "by_extension"} \
	        -imgwidth $this-resx \
	        -imgheight $this-resy
	moveToCursor $w
	wm deiconify $w
    }
    
    method changeName { w type} {
    }

    method doSaveImage {} {
	global $this-saveFile
	global $this-saveType
	$this-c dump_viewwindow [set $this-saveFile] [set $this-saveType] [set $this-resx] [set $this-resy]
	$this-c redraw
    }
}

