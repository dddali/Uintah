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

#    File   : UnuRmap.tcl
#    Author : Martin Cole
#    Date   : Mon Sep  8 09:46:23 2003

catch {rename Teem_Unu_UnuRmap ""}

itcl_class Teem_Unu_UnuRmap {
    inherit Module
    constructor {config} {
        set name UnuRmap
        set_defaults
    }
    method set_defaults {} {
        global $this-rescale
        set $this-rescale 0
	
	global $this-length
	set $this-length 0

	global $this-min
	set $this-min 0

	global $this-max
	set $this-max 0
    }

    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
            return
        }

        toplevel $w

        frame $w.f
	pack $w.f -padx 2 -pady 2 -side top -expand yes
	
	frame $w.f.options
	pack $w.f.options -side top -expand yes
	
        iwidgets::entryfield $w.f.options.length -labeltext "Length:" \
	    -textvariable $this-length
        pack $w.f.options.length -side top -expand yes -fill x	

        checkbutton $w.f.options.rescale \
	    -text "Rescale to the Map Domain" \
	    -variable $this-rescale
        pack $w.f.options.rescale -side top -expand yes -fill x

        iwidgets::entryfield $w.f.options.min -labeltext "Min:" \
	    -textvariable $this-min
        pack $w.f.options.min -side top -expand yes -fill x

        iwidgets::entryfield $w.f.options.max -labeltext "Max:" \
	    -textvariable $this-max
        pack $w.f.options.max -side top -expand yes -fill x

	makeSciButtonPanel $w.f $w $this
	moveToCursor $w

	pack $w.f -expand 1 -fill x
    }
}



