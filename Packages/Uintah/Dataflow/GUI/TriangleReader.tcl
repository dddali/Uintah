
#catch {rename TriangleReader ""}

itcl_class Uintah_Readers_TriangleReader {
    inherit Module
    constructor {config} {
	set name TriangleReader
	set_defaults
    }
    method set_defaults {} {
	global $this-filename
	set $this-filename \
	    "/home/sci/data19/kuzimmer/PSECore/src/Uintah/honlai.tri"
    }
    
#Dd: added this:
    method filedir { filebase } {
	set n [string last "/" "$filebase"]
	if { $n != -1} {
	    return [ string range $filebase 0 $n ]
	} else {
	    return ""
	}
    }

    method ui {} { 
        set w .ui[modname] 
	if {[winfo exists $w]} {
	    wm deiconify $w
	    raise $w
	    return;
	}
	global $this-filename
	global env

        toplevel $w 
        wm minsize $w 100 50 
  
        set n "$this-c needexecute " 
  
        frame $w.f1 -relief groove -borderwidth 2
	pack $w.f1 -in $w -side left
	
	
	if { [string compare [set $this-filename] ""] == 0 } {
	    puts "inside compare"
	    set dir $env(PSE_DATA)
	    iwidgets::fileselectionbox $w.f1.fb \
		-directory $env(PSE_DATA)
	    #-dblfilescommand  "$this selectfile"
	    puts "after iwidgets"
	    $w.f1.fb.filter delete 0 end
	    $w.f1.fb.filter insert 0 "$env(PSE_DATA)/*"
	    $w.f1.fb filter
	    pack $w.f1.fb -padx 2 -pady 2 -side top
	} else {
	    iwidgets::fileselectionbox $w.f1.fb \
	    -directory [filedir [set $this-filename ] ] 
	    #-dblfilescommand  "$this selectfile"


	    $w.f1.fb.filter delete 0 end
	    $w.f1.fb.filter insert 0 [filedir [set $this-filename]]/*
	    $w.f1.fb filter
	    $w.f1.fb.selection delete 0 end
	    $w.f1.fb.selection insert 0 [set $this-filename]
	    pack $w.f1.fb -padx 2 -pady 2 -side top

	}
	
	puts "outside compare"
	frame $w.f1.f -relief flat
	pack $w.f1.f -side top -padx 2 -pady 2 -expand yes -fill x
	
	button $w.f1.select -text Select -command "$this selectfile"
	pack $w.f1.select -side left -padx 2 -pady 2
	button $w.f1.close -text Close -command "destroy $w"
	pack $w.f1.close -side left -padx 2 -pady 2

    } 

    method selectfile {} {
	global $this-filename
	set w .ui[modname]
	puts "third [$w.f1.fb get]"
	set $this-filename [$w.f1.fb get]
	puts "fourth [set $this-filename]"
	$this-c needexecute
    }
    
	
}
