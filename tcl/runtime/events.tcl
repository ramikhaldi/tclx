#
# eventloop.tcl --
#
# Eventloop procedure.
#------------------------------------------------------------------------------
# Copyright 1992-1996 Karl Lehenbauer and Mark Diekhans.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies.  Karl Lehenbauer and
# Mark Diekhans make no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
#------------------------------------------------------------------------------
# $Id: events.tcl,v 8.0.4.1 1997/04/14 02:02:03 markd Exp $
#------------------------------------------------------------------------------
#

#@package: TclX-events mainloop

proc mainloop {} {
    global tcl_interactive

    if {[info exists tcl_interactive] && $tcl_interactive} {
        commandloop -async -interactive on -endcommand exit
    }
    set loopVar 0
    catch {vwait loopVar}
    exit
}


