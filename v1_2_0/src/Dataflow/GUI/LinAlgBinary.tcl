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

itcl_class SCIRun_Math_LinAlgBinary {
    inherit Module
    constructor {config} {
        set name LinAlgBinary
        set_defaults
    }
    method set_defaults {} {
        global $this-op
	set $this-op "Mult"
    }
    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
            raise $w
            return;
        }

        toplevel $w
        wm minsize $w 150 20
        frame $w.f
        pack $w.f -padx 2 -pady 2 -side top -expand yes
        global $this-op
        make_labeled_radio $w.f.r "Binary Operations:" "" \
                top $this-op \
		{{"A X B" Mult} \
                {"A + B" Add}}
	pack $w.f.r -side top -expand 1 -fill x
	pack $w.f -expand 1 -fill x
    }
}
