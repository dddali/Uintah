#
#  The contents of this file are subject to the University of Utah Public
#  License (the "License"); you may not use this file except in compliance
#  with the License.
#  
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations under
#  the License.
#  
#  The Original Source Code is SCIRun, released March 12, 2001.
#  
#  The Original Source Code was developed by the University of Utah.
#  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
#  University of Utah. All Rights Reserved.
#

# GUI for VULCANConverter module
# by Allen R. Sanderson
# May 2003

catch {rename Fusion_Fields_VULCANConverter ""}

itcl_class Fusion_Fields_VULCANConverter {
    inherit Module
    constructor {config} {
        set name VULCANConverter
        set_defaults
    }

    method set_defaults {} {

	global $this-datasets
	set $this-datasets ""

	global $this-nmodes
	set $this-nmodes 0

	global $this-allowUnrolling
	set $this-allowUnrolling 0

	global $this-unrolling
	set $this-unrolling 0
    }

    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
            raise $w
            return
        }

#	if {[winfo exists $w]} {
#	    set child [lindex [winfo children $w] 0]
#
#	    # $w withdrawn by $child's procedures
#	    raise $child
#	    return;
#	}

	# When building the UI prevent the selection from taking place
	# since it is not valid.

	toplevel $w

	# Modes
	global $this-nmodes
	frame $w.modes
	label $w.modes.label -text "Mode Summing" -width 15 \
	    -anchor w -just left
	pack $w.modes.label -side left
	pack $w.modes -side top -pady 5

	set_modes [set $this-nmodes] 0


	# Unrolling
	global $this-allowUnrolling
	frame $w.unrolling
	label $w.unrolling.label -text "Mesh unrolling" -width 15 \
	    -anchor w -just left
	checkbutton $w.unrolling.button -variable $this-unrolling

	pack $w.unrolling.button $w.unrolling.label -side left
	pack $w.unrolling -side top -pady 5

	set_unrolling [set $this-allowUnrolling]


	# Input dataset label
	frame $w.label
	label $w.label.l -text "Inputs: (Execute to show list)" -width 30 \
	    -just left

	pack $w.label.l  -side left
	pack $w.label -side top -pady 5


	# Input Dataset
	frame $w.datasets	
	pack $w.datasets -side top -pady 5

	global $this-datasets
	set_names [set $this-datasets]


	makeSciButtonPanel $w $w $this
	moveToCursor $w
    }

    method set_modes {nmodes reset} {

	global $this-nmodes
	set $this-nmodes $nmodes

        set w .ui[modname]

	if [ expr [winfo exists $w] ] {

	    if { $nmodes > 0 } {
		
		pack $w.modes.label -side left
		
		for {set i 0} {$i <= $nmodes} {incr i 1} {

		    if { [catch { set t [set $this-mode-$i] } ] } {
			if { $i < $nmodes } {
			    set $this-mode-$i 0
			} else {
			    set $this-mode-$i 1
			}
		    }

		    if { $reset } {
			if { $i < $nmodes } {
			    set $this-mode-$i 0
			} else {
			    set $this-mode-$i 1
			}
		    }

		    if [ expr [winfo exists $w.modes.$i] ] {
			$w.modes.$i.label configure -text "$i" -width 2
		    } else {
			frame $w.modes.$i
			
			label $w.modes.$i.label -text "$i" \
			    -width 2 -anchor w -just left
			checkbutton $w.modes.$i.button -variable $this-mode-$i
			
			pack $w.modes.$i.button $w.modes.$i.label -side left

		    }

		    pack $w.modes.$i -side left
		}
		
		$w.modes.$nmodes.label configure -text "All" -width 4
	    } else {
		pack forget $w.modes.label
	    }


	    if { $nmodes > 0 } {
		set i [expr $nmodes + 1]
	    } else {
		set i 0
	    }

	    # Destroy all the left over entries from prior runs.
	    while {[winfo exists $w.modes.$i]} {
		destroy $w.modes.$i
		incr i
	    }
	}
    }

    method set_unrolling { allowUnrolling } {

	global $this-allowUnrolling
	set $this-allowUnrolling $allowUnrolling

        set w .ui[modname]

	if [ expr [winfo exists $w] ] {

	    if { $allowUnrolling } {		
		pack $w.unrolling -side top
	    } else {
		pack forget $w.unrolling
	    }
	}
    }

    method set_names {datasets} {

	global $this-datasets
	set $this-datasets $datasets

        set w .ui[modname]

	if [ expr [winfo exists $w] ] {

	    for {set i 0} {$i < 10} {incr i 1} {
		if [ expr [winfo exists $w.datasets.$i] ] {
		    pack forget $w.datasets.$i
		}
	    }

	    set i 0

	    foreach dataset $datasets {

		if [ expr [winfo exists $w.datasets.$i] ] {
		    $w.datasets.$i configure -text $dataset
		} else {
		    set len [expr [string length $dataset] + 5 ]
		    label $w.datasets.$i -text $dataset -width $len \
			-anchor w -just left
		}

		pack $w.datasets.$i -side top

		incr i 1
	    }
	}
    }
}
