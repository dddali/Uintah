itcl_class Insight_DataIO_ColorImageReaderUChar2D {
    inherit Module
    constructor {config} {
        set name ColorImageReaderUChar2D
        set_defaults
    }

    method set_defaults {} {
    }

    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
	    set child [lindex [winfo children $w] 0]
	    
	    # $w withdrawn by $child's procedures
	    raise $child
	    return;
        }
        #toplevel $w

	set defext ".mhd"
	set title "Open image file"
	
	# file types to appers in filter box
	set types {
	    {{Meta Image}        {.mhd} }
	    {{PNG Image}        {.png} }
	    {{All Files}       {.*}    }
	}

	makeOpenFilebox \
		-parent . \
		-filevar $this-FileName \
		-command "$this-c needexecute; destroy " \
		-cancel "destroy " \
		-title "Open Image File" \
                -filetypes $types \
		-defaultextension $defext
    }
}


