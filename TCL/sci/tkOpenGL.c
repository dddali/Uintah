/* 
 * tkOpenGL.c --
 *
 *	This module implements "OpenGL" widgets.
 *
 */

#include "tkPort.h"
#include "tkInt.h"

#include <GL/glx.h>

/*
 * A data structure of the following type is kept for each OpenGL
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the OpenGL.  NULL
				 * means window has been deleted but
				 * widget record hasn't been cleaned up yet. */
    Display *display;		/* X's token for the window's display. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */
    char* geometry;

    /*
     * glXChooseVisual options
     */
    int direct;
    int buffersize;
    int level;
    int rgba;
    int doublebuffer;
    int stereo;
    int auxbuffers;
    int redsize;
    int greensize;
    int bluesize;
    int alphasize;
    int depthsize;
    int stencilsize;
    int accumredsize;
    int accumgreensize;
    int accumbluesize;
    int accumalphasize;

    int visualid;

    GLXContext cx;
    XVisualInfo* vi;

    /*
     * Information used when displaying widget:
     */

} OpenGL;

/*
 * Information used for argv parsing.
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_BOOLEAN, "-direct", "direct", "direct",
     "1", Tk_Offset(OpenGL, direct), 0},
    {TK_CONFIG_INT, "-buffersize", "bufferSize", "BufferSize",
     "8", Tk_Offset(OpenGL, buffersize), 0},
    {TK_CONFIG_INT, "-level", "level", "level",
     "0", Tk_Offset(OpenGL, level), 0},
    {TK_CONFIG_BOOLEAN, "-rgba", "rgba", "rgba",
     "1", Tk_Offset(OpenGL, rgba), 0},
    {TK_CONFIG_BOOLEAN, "-doublebuffer", "doublebuffer", "doublebuffer",
     "0", Tk_Offset(OpenGL, doublebuffer), 0},
    {TK_CONFIG_BOOLEAN, "-stereo", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, stereo), 0},
    {TK_CONFIG_INT, "-auxbuffers", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, auxbuffers), 0},
    {TK_CONFIG_INT, "-redsize", "glsize", "glsize",
     "2", Tk_Offset(OpenGL, redsize), 0},
    {TK_CONFIG_INT, "-greensize", "glsize", "glsize",
     "2", Tk_Offset(OpenGL, greensize), 0},
    {TK_CONFIG_INT, "-bluesize", "glsize", "glsize",
     "2", Tk_Offset(OpenGL, bluesize), 0},
    {TK_CONFIG_INT, "-alphasize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, alphasize), 0},
    {TK_CONFIG_INT, "-depthsize", "glsize", "glsize",
     "2", Tk_Offset(OpenGL, depthsize), 0},
    {TK_CONFIG_INT, "-stencilsize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, stencilsize), 0},
    {TK_CONFIG_INT, "-accumredsize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, accumredsize), 0},
    {TK_CONFIG_INT, "-accumgreensize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, accumgreensize), 0},
    {TK_CONFIG_INT, "-accumbluesize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, accumbluesize), 0},
    {TK_CONFIG_INT, "-accumalphasize", "glsize", "glsize",
     "0", Tk_Offset(OpenGL, accumalphasize), 0},
    {TK_CONFIG_INT, "-visual", "visual", "visual",
     "0", Tk_Offset(OpenGL, visualid), 0},
    {TK_CONFIG_STRING, "-geometry", "geometry", "Geometry",
     "100x100", Tk_Offset(OpenGL, geometry), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		OpenGLConfigure _ANSI_ARGS_((Tcl_Interp *interp,
			    OpenGL *OpenGLPtr, int argc, char **argv,
			    int flags));
static void		OpenGLDestroy _ANSI_ARGS_((ClientData clientData));
static void		OpenGLEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		OpenGLWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *, int argc, char **argv));

/*
 *--------------------------------------------------------------
 *
 * OpenGLCmd --
 *
 *	This procedure is invoked to process the "OpenGL" Tcl
 *	command.  It creates a new "OpenGL" widget.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	A new widget is created and configured.
 *
 *--------------------------------------------------------------
 */

int
OpenGLCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    XVisualInfo* vi;
    Tk_Window main = (Tk_Window) clientData;
    OpenGL *OpenGLPtr;
    Colormap cmap;
    Tk_Window tkwin;
    int attributes[50];
    int idx=0;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath(interp, main, argv[1], (char *) NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Tk_SetClass(tkwin, "OpenGL");

    /*
     * Allocate and initialize the widget record.
     */

    OpenGLPtr = (OpenGL *) ckalloc(sizeof(OpenGL));
    OpenGLPtr->interp = interp;
    OpenGLPtr->tkwin = tkwin;
    OpenGLPtr->display = Tk_Display(tkwin);
    OpenGLPtr->geometry=0;

    Tk_CreateEventHandler(OpenGLPtr->tkwin, StructureNotifyMask,
	    OpenGLEventProc, (ClientData) OpenGLPtr);
    Tcl_CreateCommand(interp, Tk_PathName(OpenGLPtr->tkwin), OpenGLWidgetCmd,
	    (ClientData) OpenGLPtr, (void (*)()) NULL);
    if (OpenGLConfigure(interp, OpenGLPtr, argc-2, argv+2, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    if(OpenGLPtr->visualid){
      int n;
      XVisualInfo tmpl;
      tmpl.visualid=OpenGLPtr->visualid;
      vi=XGetVisualInfo(Tk_Display(tkwin), VisualIDMask, &tmpl, &n);
      if(!vi || n!=1){
	Tcl_AppendResult(interp, "Error finding visual", NULL);
	return TCL_ERROR;
      }
    } else {
      /*
       * Pick the right visual...
       */
      attributes[idx++]=GLX_BUFFER_SIZE;
      attributes[idx++]=OpenGLPtr->buffersize;
      attributes[idx++]=GLX_LEVEL;
      attributes[idx++]=OpenGLPtr->level;
      if(OpenGLPtr->rgba)
	attributes[idx++]=GLX_RGBA;
      if(OpenGLPtr->doublebuffer)
	attributes[idx++]=GLX_DOUBLEBUFFER;
      if(OpenGLPtr->stereo)
	attributes[idx++]=GLX_STEREO;
      attributes[idx++]=GLX_AUX_BUFFERS;
      attributes[idx++]=OpenGLPtr->auxbuffers;
      attributes[idx++]=GLX_RED_SIZE;
      attributes[idx++]=OpenGLPtr->redsize;
      attributes[idx++]=GLX_GREEN_SIZE;
      attributes[idx++]=OpenGLPtr->greensize;
      attributes[idx++]=GLX_BLUE_SIZE;
      attributes[idx++]=OpenGLPtr->bluesize;
      attributes[idx++]=GLX_ALPHA_SIZE;
      attributes[idx++]=OpenGLPtr->alphasize;
      attributes[idx++]=GLX_DEPTH_SIZE;
      attributes[idx++]=OpenGLPtr->depthsize;
      attributes[idx++]=GLX_STENCIL_SIZE;
      attributes[idx++]=OpenGLPtr->stencilsize;
      attributes[idx++]=GLX_ACCUM_RED_SIZE;
      attributes[idx++]=OpenGLPtr->accumredsize;
      attributes[idx++]=GLX_ACCUM_GREEN_SIZE;
      attributes[idx++]=OpenGLPtr->accumgreensize;
      attributes[idx++]=GLX_ACCUM_BLUE_SIZE;
      attributes[idx++]=OpenGLPtr->accumbluesize;
      attributes[idx++]=GLX_ACCUM_ALPHA_SIZE;
      attributes[idx++]=OpenGLPtr->accumalphasize;
#if 0
      attributes[idx++]=GLX_SAMPLES_SGIS;
      attributes[idx++]=4;
#endif
      attributes[idx++]=None;
      vi = glXChooseVisual(Tk_Display(tkwin), Tk_ScreenNumber(tkwin),
			   attributes);
      if(!vi){
	Tcl_AppendResult(interp, "Error selecting visual", (char*)NULL);
	return TCL_ERROR;
      }
    }

#if 0
    OpenGLPtr->cx = glXCreateContext(Tk_Display(tkwin),
				     vi, 0, OpenGLPtr->direct);
    if(!OpenGLPtr->cx){
	Tcl_AppendResult(interp, "Error making GL context", (char*)NULL);
	return TCL_ERROR;
    }
#endif
    OpenGLPtr->cx=0;
    OpenGLPtr->vi=vi;
    
    cmap = XCreateColormap(Tk_Display(tkwin),
			   RootWindow(Tk_Display(tkwin), vi->screen),
			   vi->visual, AllocNone);
    
    if( Tk_SetWindowVisual(tkwin, vi->visual, vi->depth, cmap) != 1){
	Tcl_AppendResult(interp, "Error setting visual for window", (char*)NULL);
	return TCL_ERROR;
    }

    interp->result = Tk_PathName(OpenGLPtr->tkwin);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * OpenGLWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
OpenGLWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Information about OpenGL widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    OpenGL *OpenGLPtr = (OpenGL *) clientData;
    int result = TCL_OK;
    int length;
    char c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tk_Preserve((ClientData) OpenGLPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, OpenGLPtr->tkwin, configSpecs,
		    (char *) OpenGLPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, OpenGLPtr->tkwin, configSpecs,
		    (char *) OpenGLPtr, argv[2], 0);
	} else {
	    result = OpenGLConfigure(interp, OpenGLPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\":  must be configure, position, or size", (char *) NULL);
	goto error;
    }
    Tk_Release((ClientData) OpenGLPtr);
    return result;

    error:
    Tk_Release((ClientData) OpenGLPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenGLConfigure --
 *
 *	This procedure is called to process an argv/argc list in
 *	conjunction with the Tk option database to configure (or
 *	reconfigure) a OpenGL widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for OpenGLPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
OpenGLConfigure(interp, OpenGLPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    OpenGL *OpenGLPtr;			/* Information about widget. */
    int argc;				/* Number of valid entries in argv. */
    char **argv;			/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    if (Tk_ConfigureWidget(interp, OpenGLPtr->tkwin, configSpecs,
	    argc, argv, (char *) OpenGLPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    {
	int height, width;
        if (sscanf(OpenGLPtr->geometry, "%dx%d", &width, &height) != 2) {
            Tcl_AppendResult(interp, "bad geometry \"", OpenGLPtr->geometry,
                    "\": expected widthxheight", (char *) NULL);
            return TCL_ERROR;
        }
        Tk_GeometryRequest(OpenGLPtr->tkwin, width, height);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * OpenGLEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on OpenGLs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
OpenGLEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    OpenGL *OpenGLPtr = (OpenGL *) clientData;

    if (eventPtr->type == DestroyNotify) {
	Tcl_DeleteCommand(OpenGLPtr->interp, Tk_PathName(OpenGLPtr->tkwin));
	OpenGLPtr->tkwin = NULL;
	Tk_EventuallyFree((ClientData) OpenGLPtr, OpenGLDestroy);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * OpenGLDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a OpenGL at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the OpenGL is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
OpenGLDestroy(clientData)
    ClientData clientData;	/* Info about OpenGL widget. */
{
    OpenGL *OpenGLPtr = (OpenGL *) clientData;

    glXDestroyContext(OpenGLPtr->display, OpenGLPtr->cx);

    Tk_FreeOptions(configSpecs, (char *) OpenGLPtr, OpenGLPtr->display, 0);
    ckfree((char *) OpenGLPtr);
}


GLXContext OpenGLGetContext2(interp, name, dpy)
    Tcl_Interp* interp;
    char* name;
    Display* dpy;
{
    Tcl_CmdInfo info;
    OpenGL* OpenGLPtr;
    if(!Tcl_GetCommandInfo(interp, name, &info))
	return 0;
    OpenGLPtr=(OpenGL*)info.clientData;
    if(!OpenGLPtr->cx){
	Tk_Window tkwin;
	tkwin=Tk_NameToWindow(interp, name, Tk_MainWindow(interp));
	if(!dpy)
	    dpy=Tk_Display(tkwin);
	OpenGLPtr->cx = glXCreateContext(dpy, 
					 OpenGLPtr->vi, 0, OpenGLPtr->direct);
	if(!OpenGLPtr->cx){
	    Tcl_AppendResult(interp, "Error making GL context", (char*)NULL);
	    return 0;
	}
    }
    return OpenGLPtr->cx;
}

GLXContext OpenGLGetContext(interp, name)
    Tcl_Interp* interp;
    char* name;
{
    return OpenGLGetContext2(interp, name, 0);
}
