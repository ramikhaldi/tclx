/*
 * tclXsocket.c --
 *
 * Socket utility functions and commands.
 *---------------------------------------------------------------------------
 * Copyright 1991-1997 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
x * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id: tclXsocket.c,v 8.0.4.1 1997/04/14 02:01:56 markd Exp $
 *-----------------------------------------------------------------------------
 */

#include "tclExtdInt.h"

/*
 * Prototypes of internal functions.
 */
static int
ReturnGetHostError _ANSI_ARGS_((Tcl_Interp *interp,
                                char       *host));

static struct hostent *
InfoGetHost _ANSI_ARGS_((Tcl_Interp *interp,
                         int         objc,
                         Tcl_Obj   **objv));


/*-----------------------------------------------------------------------------
 * ReturnGetHostError --
 *
 *   Return an error message when gethostbyname or gethostbyaddr fails.
 *
 * Parameters:
 *   o interp (O) - The error message is returned in the result.
 *   o host (I) - Host name or address that got the error.
 * Globals:
 *   o h_errno (I) - The list of file handles to parse, may be empty.
 * Returns:
 *   Always returns TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
ReturnGetHostError (interp, host)
    Tcl_Interp *interp;
    char       *host;
{
    char  *errorMsg;
    char  *errorCode;

    switch (h_errno) {
      case HOST_NOT_FOUND:
        errorCode = "HOST_NOT_FOUND";
        errorMsg = "host not found";
        break;
      case TRY_AGAIN:
        errorCode = "TRY_AGAIN";
        errorMsg = "try again";
        break;
      case NO_RECOVERY:
        errorCode = "NO_RECOVERY";
        errorMsg = "unrecordable server error";
        break;
#ifdef NO_DATA
      case NO_DATA:
        errorCode = "NO_DATA";
        errorMsg = "no data";
        break;
#endif
    }
    Tcl_SetErrorCode (interp, "INET", errorCode, errorMsg, (char *)NULL);
    Tcl_AppendResult (interp, "host lookup failure: ",
                      host, " (", errorMsg, ")",
                      (char *) NULL);
    return TCL_ERROR;
}

/*-----------------------------------------------------------------------------
 * TclXGetHostInfo --
 *    Return a host address, name (if it can be obtained) and port number.
 * Used by the fstat command.
 *     
 * Parameters:
 *   o interp (O) - List is returned in the result.
 *   o channel (I) - Channel associated with the socket.
 *   o remoteHost (I) -  TRUE to get remote host information, FALSE to get 
 *     local host info.
 * Returns:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
int
TclXGetHostInfo (interp, channel, remoteHost)
    Tcl_Interp *interp;
    Tcl_Channel channel;
    int         remoteHost;
{
    struct sockaddr_in sockaddr;
    struct hostent *hostEntry;
    char *hostName;
    char portText [32];

    if (remoteHost) {
        if (TclXOSgetpeername (interp, channel,
                               &sockaddr, sizeof (sockaddr)) != TCL_OK)
            return TCL_ERROR;
    } else {
        if (TclXOSgetsockname (interp, channel, &sockaddr,
                               sizeof (sockaddr)) != TCL_OK)
            return TCL_ERROR;
    }

    hostEntry = gethostbyaddr ((char *) &(sockaddr.sin_addr),
                               sizeof (sockaddr.sin_addr),
                               AF_INET);
    if (hostEntry != NULL)
        hostName = hostEntry->h_name;
    else
        hostName = "";

    Tcl_AppendElement (interp, inet_ntoa (sockaddr.sin_addr));

    Tcl_AppendElement (interp, hostName);

    sprintf (portText, "%u", ntohs (sockaddr.sin_port));
    Tcl_AppendElement (interp, portText);
       
    return TCL_OK;
}

/*-----------------------------------------------------------------------------
 * InfoGetHost --
 *
 *   Validate arguments and call gethostbyaddr for the host_info options
 * that return info about a host.  This looks up host information either by
 * name or address.
 *
 * Parameters:
 *   o interp (O) - The error message is returned in the result.
 *   o objc, objv (I) - Command argments as Tcl objects.  Host name or IP 
 *     address is expected in objv [2].
 * Returns:
 *   Pointer to the host entry or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
static struct hostent *
InfoGetHost (interp, objc, objv)
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    struct hostent *hostEntry;
    struct in_addr address;

    char *command =    Tcl_GetStringFromObj (objv [0], NULL);
    char *subCommand = Tcl_GetStringFromObj (objv [1], NULL);
    char *host       = Tcl_GetStringFromObj (objv [2], NULL);

    if (objc != 3) {
        TclX_StringAppendObjResult (interp, tclXWrongArgs, command, " ", 
	    subCommand, " host", (char *) NULL);
        return NULL;
    }

    if (TclXOSInetAtoN (NULL, host, &address) == TCL_OK) {
        hostEntry = gethostbyaddr ((char *) &address,
                                   sizeof (address),
                                   AF_INET);
    } else {
        hostEntry = gethostbyname (host);
    }
    if (hostEntry == NULL) {
        ReturnGetHostError (interp, host);
        return NULL;
    }
    return hostEntry;
}

/*-----------------------------------------------------------------------------
 * Tcl_HostInfoObjCmd --
 *     Implements the TCL host_info command:
 *
 *      host_info addresses host
 *      host_info official_name host
 *      host_info aliases host
 *
 * Results:
 *   For hostname, a list of address associated with the host.
 *-----------------------------------------------------------------------------
 */
int
Tcl_HostInfoObjCmd (clientData, interp, objc, objv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj   **objv;
{
    struct hostent *hostEntry;
    struct in_addr  inAddr;
    int             idx;
    char           *subCommand;
    Tcl_Obj        *listObj;
    Tcl_Obj        *resultPtr;

    if (objc < 2)
	return TclX_WrongArgs (interp, objv [0], "option ...");

    resultPtr = Tcl_GetObjResult (interp);
    subCommand = Tcl_GetStringFromObj (objv [1], NULL);

    if (STREQU (subCommand, "addresses")) {
        hostEntry = InfoGetHost (interp, objc, objv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy ((VOID *) hostEntry->h_addr_list [idx],
                   (VOID *) &inAddr,
                   hostEntry->h_length);

	    listObj = Tcl_NewStringObj (inet_ntoa (inAddr), -1);
	    Tcl_ListObjAppendElement (interp, resultPtr, listObj);
        }
        return TCL_OK;
    }

    if (STREQU (subCommand, "address_name")) {
        hostEntry = InfoGetHost (interp, objc, objv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_addr_list [idx] != NULL; idx++) {
            bcopy ((VOID *) hostEntry->h_addr_list [idx],
                   (VOID *) &inAddr,
                   hostEntry->h_length);
	    listObj = Tcl_NewStringObj (hostEntry->h_name, -1);
	    Tcl_ListObjAppendElement (interp, resultPtr, listObj);
        }
        return TCL_OK;
    }

    if (STREQU (subCommand, "official_name")) {
        hostEntry = InfoGetHost (interp, objc, objv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        Tcl_SetStringObj (resultPtr, hostEntry->h_name, -1);
        return TCL_OK;
    }

    if (STREQU (subCommand, "aliases")) {
        hostEntry = InfoGetHost (interp, objc, objv);
        if (hostEntry == NULL)
            return TCL_ERROR;

        for (idx = 0; hostEntry->h_aliases [idx] != NULL; idx++) {
	    listObj = Tcl_NewStringObj (hostEntry->h_aliases [idx], -1);
	    Tcl_ListObjAppendElement (interp, resultPtr, listObj);
        }
        return TCL_OK;
    }

    TclX_StringAppendObjResult (interp, "invalid option \"", subCommand,
                      "\", expected one of \"addresses\", \"official_name\"",
                      " or \"aliases\"", (char *) NULL);
    return TCL_ERROR;
}


