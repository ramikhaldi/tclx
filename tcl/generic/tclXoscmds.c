/*
 * tclXoscmds.c --
 *
 * Tcl commands to access unix system calls that are portable to other
 * platforms.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1997 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXoscmds.c,v 8.0.4.1 1997/04/14 02:01:51 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"


/*-----------------------------------------------------------------------------
 * Tcl_AlarmObjCmd --
 *     Implements the TCL Alarm command:
 *         alarm seconds
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_AlarmObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    double seconds;

    if (objc != 2)
	return TclX_WrongArgs (interp, objv [0], "seconds");

    if (Tcl_GetDoubleFromObj (interp, objv[1], &seconds) != TCL_OK)
	return TCL_ERROR;

    if (TclXOSsetitimer (interp, &seconds, "alarm") != TCL_OK)
        return TCL_ERROR;

    Tcl_SetDoubleObj (Tcl_GetObjResult (interp), seconds);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_LinkObjCmd --
 *     Implements the TCL link command:
 *         link ?-sym? srcpath destpath
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *-----------------------------------------------------------------------------
 */
int
Tcl_LinkObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    char *srcPath, *destPath;
    Tcl_DString  srcPathBuf, destPathBuf;
    char *argv0String;
    char *srcPathString;
    char *destPathString;

    Tcl_DStringInit (&srcPathBuf);
    Tcl_DStringInit (&destPathBuf);

    if ((objc < 3) || (objc > 4))
	return TclX_WrongArgs (interp, objv [0], "?-sym? srcpath destpath");

    if (objc == 4) {
        char *argv1String = Tcl_GetStringFromObj (objv [1], NULL);

        if (!STREQU (argv1String, "-sym")) {
            TclX_StringAppendObjResult (
                interp,
                "invalid option, expected: \"-sym\", got: ",
                Tcl_GetStringFromObj (objv [1], NULL),
                (char *) NULL);
            return TCL_ERROR;
        }
    }

    srcPathString = Tcl_GetStringFromObj (objv [objc - 2], NULL);
    srcPath = Tcl_TranslateFileName (interp, srcPathString, &srcPathBuf);
    if (srcPath == NULL)
        goto errorExit;

    destPathString = Tcl_GetStringFromObj (objv [objc - 1], NULL);
    destPath = Tcl_TranslateFileName (interp, destPathString, &destPathBuf);
    if (destPath == NULL)
        goto errorExit;

    argv0String = Tcl_GetStringFromObj (objv [0], NULL);
    if (objc == 4) {
        if (TclX_OSsymlink (interp, srcPath, destPath, argv0String) != TCL_OK)
            goto errorExit;
    } else {
        if (TclX_OSlink (interp, srcPath, destPath, argv0String) != TCL_OK)
            goto errorExit;
    }

    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_OK;

  errorExit:
    Tcl_DStringFree (&srcPathBuf);
    Tcl_DStringFree (&destPathBuf);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * Tcl_NiceObjCmd --
 *     Implements the TCL nice command:
 *         nice ?priorityincr?
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_NiceObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    Tcl_Obj *resultPtr = Tcl_GetObjResult (interp);
    int priorityIncr, priority;
    long longPriorityIncr;
    char *argv0String;

    if (objc > 2)
	return TclX_WrongArgs (interp, objv [0], "?priorityincr?");

    argv0String = Tcl_GetStringFromObj (objv [0], NULL);

    /*
     * Return the current priority if an increment is not supplied.
     */
    if (objc == 1) {
        if (TclXOSgetpriority (interp, &priority, argv0String) != TCL_OK)
            return TCL_ERROR;
	Tcl_SetIntObj (Tcl_GetObjResult (interp), priority);
        return TCL_OK;
    }

    /*
     * Increment the priority.
     */
    if (Tcl_GetIntFromObj (interp, objv [1], &longPriorityIncr) != TCL_OK)
        return TCL_ERROR;

    priorityIncr = (int)longPriorityIncr;

    if (TclXOSincrpriority (interp, priorityIncr, &priority,
                            argv0String) != TCL_OK)
        return TCL_ERROR;

    Tcl_SetIntObj (resultPtr, priority);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_SleepObjCmd --
 *     Implements the TCL sleep command:
 *         sleep seconds
 *
 * Results:
 *      Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SleepObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    long time;

    if (objc != 2)
	return TclX_WrongArgs (interp, objv [0], "seconds");

    if (Tcl_GetIntFromObj (interp, objv [1], &time) != TCL_OK)
        return TCL_ERROR;

    TclXOSsleep ((int) time);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_SyncObjCmd --
 *     Implements the TCL sync command:
 *         sync
 *
 * Results:
 *      Standard TCL results.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SyncObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    Tcl_Channel  channel;
    char        *fileHandle;

    if ((objc < 1) || (objc > 2))
	return TclX_WrongArgs (interp, objv [0], "?filehandle?");

    if (objc == 1) {
	TclXOSsync ();
	return TCL_OK;
    }

    fileHandle = Tcl_GetStringFromObj (objv [1], NULL);
    channel = TclX_GetOpenChannel (interp, fileHandle, TCL_WRITABLE);
    if (channel == NULL)
        return TCL_ERROR;

    if (Tcl_Flush (channel) < 0) {
	Tcl_SetStringObj (Tcl_GetObjResult (interp),
                          Tcl_PosixError (interp), -1);
        return TCL_ERROR;
    }
    return TclXOSfsync (interp, channel);
}

/*-----------------------------------------------------------------------------
 * Tcl_SystemObjCmd --
 *     Implements the TCL system command:
 *     system command
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_SystemObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    char *systemString;
    int exitCode;

    if (objc != 2)
	return TclX_WrongArgs (interp, objv [0], "command");

    systemString = Tcl_GetStringFromObj (objv [1], NULL);
    if (TclXOSsystem (interp, systemString, &exitCode) != TCL_OK)
        return TCL_ERROR;

    Tcl_SetIntObj (Tcl_GetObjResult (interp), exitCode);
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * Tcl_UmaskObjCmd --
 *     Implements the TCL umask command:
 *     umask ?octalmask?
 *
 * Results:
 *  Standard TCL results, may return the UNIX system error message.
 *
 *-----------------------------------------------------------------------------
 */
int
Tcl_UmaskObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    int    mask;
    char  *umaskString;
    char   numBuf [32];

    if ((objc < 1) || (objc > 2))
	return TclX_WrongArgs (interp, objv [0], "?octalmask?");

    /*
     * FIX: Should include leading 0 to make it a legal number.
     */
    if (objc == 1) {
        mask = umask (0);
        umask ((unsigned short) mask);
        sprintf (numBuf, "%o", mask);
	Tcl_SetStringObj (Tcl_GetObjResult (interp), numBuf, -1);
    } else {
	umaskString = Tcl_GetStringFromObj (objv [1], NULL);
        if (!Tcl_StrToInt (umaskString, 8, &mask)) {
            TclX_StringAppendObjResult (interp, 
                                        "Expected octal number got: ",
                                        Tcl_GetStringFromObj (objv [1],
                                                              NULL),
                                        (char *) NULL);
            return TCL_ERROR;
        }

        umask ((unsigned short) mask);
    }
    return TCL_OK;
}


