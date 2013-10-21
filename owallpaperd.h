#include <Python.h>
#include "structmember.h"

#include "helper.h"

/** Exception type for OWallpaperD errors */
extern PyObject *OWallpaperDError;

/** OWallpaperD type */
extern PyTypeObject OWallpaperDType;

/**
 * Wallpaper-switching daemon object, encapsulating the state of each Xinerama
 * screen, including the dimensions of the screen, a desktop window for each
 * screen, and which workspace was on each screen the last time that we checked
 */
typedef struct {
    PyObject_HEAD

    /** X11 display. */
    Display *display;

    /** X11 screen. */
    int screen;

    /** Number of Xinerama screens. */
    Py_ssize_t num_screens;

    /** Info for each Xinerama screen. */
    XineramaScreenInfo *screens;

    /** Desktop window for each Xinerama screen. */
    Window *windows;

    /** Workspace on each Xinerama screen. */
    long *workspaces;

    /** Python list of Wallpaper objects. */
    PyObject *wallpapers;
} OWallpaperD;

/** Wallpaper type */
extern PyTypeObject WallpaperType;

/**
 * Wallpaper object, storing the pixmaps for the wallpaper. We store a pixmap
 * for each Xinerama screen.
 */
typedef struct {
    PyObject_HEAD

    /** X11 display. */
    Display *display;

    /** X11 screen. */
    int screen;

    /** Number of Xinerama screens. */
    Py_ssize_t num_screens;

    /** Pixmap for each Xinerama screen. */
    Pixmap *pixmaps;
} Wallpaper;
