C       
C       File:          whof77_UIPort_Impl.f
C       Symbol:        whof77.UIPort-v1.0
C       Symbol Type:   class
C       Babel Version: 0.7.0
C       SIDL Created:  20020813 11:07:51 CDT
C       Generated:     20020813 11:07:52 CDT
C       Description:   Server-side implementation for whof77.UIPort
C       
C       WARNING: Automatically generated; only changes within splicers preserved
C       
C       


C       
C       Symbol "whof77.UIPort" (version 1.0)
C       


C       DO-NOT-DELETE splicer.begin(_miscellaneous_code_start)
C       Insert extra code here...
C       DO-NOT-DELETE splicer.end(_miscellaneous_code_start)




C       
C       Class constructor called when the class is created.
C       

        subroutine whof77_UIPort__ctor_impl(self)
        implicit none
        integer*8 self
C       DO-NOT-DELETE splicer.begin(whof77.UIPort._ctor)
C       Insert the implementation here...
C       DO-NOT-DELETE splicer.end(whof77.UIPort._ctor)
        end


C       
C       Class destructor called when the class is deleted.
C       

        subroutine whof77_UIPort__dtor_impl(self)
        implicit none
        integer*8 self
C       DO-NOT-DELETE splicer.begin(whof77.UIPort._dtor)
C       Insert the implementation here...
C       DO-NOT-DELETE splicer.end(whof77.UIPort._dtor)
        end


C       
C       Execute some encapsulated functionality on the component. 
C       @return 0 if ok, -1 if internal error but component may be used further,
C       -2 if error so severe that component cannot be further used safely.
C       

        subroutine whof77_UIPort_ui_impl(self, retval)
        implicit none
        integer*8 self
        integer*4 retval
C       DO-NOT-DELETE splicer.begin(whof77.UIPort.ui)
C       Insert the implementation here...
C       DO-NOT-DELETE splicer.end(whof77.UIPort.ui)
        end


C       DO-NOT-DELETE splicer.begin(_miscellaneous_code_end)
C       Insert extra code here...
C       DO-NOT-DELETE splicer.end(_miscellaneous_code_end)
