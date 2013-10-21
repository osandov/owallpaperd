#include "owallpaperd.h"

static void OWallpaperD_dealloc(OWallpaperD *self)
{
    Py_ssize_t i;
    if (self->screens)
        XFree(self->screens);
    for (i = 0; i < self->num_screens; ++i)
        XDestroyWindow(self->display, self->windows[i]);
    if (self->windows)
        PyMem_Free(self->windows);
    if (self->workspaces)
        XFree(self->workspaces);
    if (self->display)
        XCloseDisplay(self->display);
    Py_XDECREF(self->wallpapers);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static int OWallpaperD_init(OWallpaperD *self, PyObject *args, PyObject *kwds)
{
    const char *display_name = NULL;
    int screen_num = -1, num_screens;
    Py_ssize_t i;

    static char *kwlist[] = {"display_name", "screen", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|si", kwlist,
                                     &display_name, &screen_num))
        return -1;

    /* Initialize X */
    self->display = XOpenDisplay(display_name);
    if (!self->display) {
        if (display_name)
            PyErr_Format(OWallpaperDError, "could not open display %s",
                         display_name);
        else
            PyErr_SetString(OWallpaperDError, "could not open default display");
        return -1;
    }

    if (screen_num == -1)
        self->screen = DefaultScreen(self->display);
    else
        self->screen = screen_num;

    XSelectInput(self->display, RootWindow(self->display, self->screen),
                 PropertyChangeMask);

    /* Get info for Xinerama screens */
    self->screens = XineramaQueryScreens(self->display, &num_screens);
    if (!self->screens) {
        PyErr_Format(OWallpaperDError, "Xinerama is not active");
        return -1;
    }
    self->num_screens = num_screens;

    /* Create all of the desktop windows and map them */
    self->windows = PyMem_New(Window, self->num_screens);
    if (!self->windows) {
        PyErr_NoMemory();
        return -1;
    }
    for (i = 0; i < self->num_screens; ++i) {
        XineramaScreenInfo *info = &self->screens[i];
        self->windows[i] = create_desktop_window(self->display, self->screen,
                                                 info);
    }
    for (i = 0; i < self->num_screens; ++i)
        XMapWindow(self->display, self->windows[i]);

    /* Allocate and initialize array for workspaces */
    self->workspaces = PyMem_New(long, self->num_screens);
    if (!self->workspaces) {
        PyErr_NoMemory();
        return -1;
    }

    for (i = 0; i < self->num_screens; ++i)
        self->workspaces[i] = -1;

    /* Create empty list of wallpapers */
    self->wallpapers = PyList_New(0);
    if (!self->wallpapers)
        return -1;

    return 0;
}

static PyObject *OWallpaperD_getwallpapers(OWallpaperD *self, PyObject *value,
                                           void *closure)
{
    Py_INCREF(self->wallpapers);
    return self->wallpapers;
}

static PyObject *OWallpaperD_getnum_screens(OWallpaperD *self, PyObject *value,
                                            void *closure)
{
    return PyLong_FromSsize_t(self->num_screens);
}

static PyGetSetDef OWallpaperD_getset[] = {
    {"wallpapers",
     (getter) OWallpaperD_getwallpapers, NULL,
     "List of currently loaded wallpapers.", NULL},
    {"num_screens",
     (getter) OWallpaperD_getnum_screens, NULL,
     "Number of Xinerama screens.", NULL},
    {NULL}
};

static PyObject *OWallpaperD_wait_for_workspace_change(OWallpaperD *self)
{
    Display *display;
    int screen;

    Atom OWALLPAPERD_WORKSPACES;
    XEvent event;
    Time currentTime;

    long *workspaces;
    Py_ssize_t i;
    PyObject *workspaces_tuple;

    display = self->display;
    screen = self->screen;

    OWALLPAPERD_WORKSPACES =
        XInternAtom(display, "OWALLPAPERD_WORKSPACES", True);
    if (OWALLPAPERD_WORKSPACES == None) {
        PyErr_SetString(OWallpaperDError,
                        "could not intern OWALLPAPERD_WORKSPACES atom");
        return NULL;
    }

    /*
     * We only want events that happen after this function is called, so get
     * the current time to compare against
     */
    currentTime = get_current_time(display, screen);
    if (!currentTime) {
        PyErr_SetString(OWallpaperDError,
                        "could not get current X server time");
        return NULL;
    }

    for (;;) {
        XNextEvent(display, &event);
        if (event.type == PropertyNotify &&
            event.xproperty.atom == OWALLPAPERD_WORKSPACES &&
            event.xproperty.time > currentTime)
        {
            int changed = 0;
            workspaces = get_workspaces(display, screen, self->num_screens);
            if (!workspaces) {
                PyErr_SetString(OWallpaperDError,
                                "could not get current workspaces");
                goto errout;
            }

            /* Make sure the workspaces have actually changed */
            for (i = 0; i < self->num_screens; ++i) {
                if (workspaces[i] != self->workspaces[i]) {
                    changed = 1;
                    self->workspaces[i] = workspaces[i];
                }
            }
            XFree(workspaces);
            if (changed)
                break;
        }
    }

    /* Make the tuple to return out to Python-land */
    workspaces_tuple = PyTuple_New(self->num_screens);
    if (!workspaces_tuple)
        return NULL;
    for (i = 0; i < self->num_screens; ++i) {
        PyObject *workspace;
        workspace = PyLong_FromLong(self->workspaces[i]);
        if (!workspace) {
            Py_DECREF(workspaces_tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(workspaces_tuple, i, workspace);
    }
    return workspaces_tuple;

errout:
    if (workspaces)
        XFree(workspaces);
    return NULL;
}

static PyObject *OWallpaperD_add_wallpaper(OWallpaperD *self, PyObject *args,
                                           PyObject *kwds)
{
    PyObject *wallpaper;
    Py_ssize_t num_args;
    PyObject *new_args = NULL;
    Py_ssize_t i;
    int success = 1;

    num_args = PyTuple_GET_SIZE(args) + 1;
    new_args = PyTuple_New(num_args);

    if (!new_args) {
        success = 0;
        goto out;
    }

    Py_INCREF(self);
    PyTuple_SET_ITEM(new_args, 0, (PyObject*) self);
    for (i = 1; i < num_args; ++i) {
        PyObject *arg = PyTuple_GET_ITEM(args, i - 1);
        Py_INCREF(arg);
        PyTuple_SET_ITEM(new_args, i, arg);
    }

    wallpaper = PyObject_Call((PyObject*) &WallpaperType, new_args, kwds);
    if (!wallpaper) {
        success = 0;
        goto out;
    }

    if (PyList_Append(self->wallpapers, wallpaper) == -1) {
        success = 0;
        goto out;
    }

out:
    Py_XDECREF(new_args);
    if (success)
        return wallpaper;
    else
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *OWallpaperD_set_wallpaper(OWallpaperD *self, PyObject *args)
{
    PyObject *wallpaper_o;
    Wallpaper *wallpaper;

    Display *display;
    int xinerama_screen;
    Window window;
    Pixmap pixmap;

    if (!PyArg_ParseTuple(args, "iO", &xinerama_screen, &wallpaper_o))
        return NULL;

    if (!PyObject_TypeCheck(wallpaper_o, &WallpaperType)) {
        PyErr_SetString(OWallpaperDError,
                        "first argument must be a Wallpaper object");
        return NULL;
    } else
        wallpaper = (Wallpaper*) wallpaper_o;

    /*
     * If the display, screen, and number of Xinerama screens for the given
     * Wallpaper don't match with our members, then we can't use this Wallpaper
     * object
     */
    if (wallpaper->display != self->display ||
        wallpaper->screen != self->screen ||
        wallpaper->num_screens != self->num_screens)
    {
        PyErr_SetString(OWallpaperDError,
                        "Wallpaper was not created for this OWallpaperD");
        return NULL;
    }

    if (xinerama_screen >= self->num_screens) {
        PyErr_SetString(PyExc_IndexError, "screen out of bounds");
        return NULL;
    }

    display = self->display;
    window = self->windows[xinerama_screen];
    pixmap = wallpaper->pixmaps[xinerama_screen];

    /* Actually set the wallpaper */
    XKillClient(display, AllTemporary);
    XSetCloseDownMode(display, RetainTemporary);

    XSetWindowBackgroundPixmap(display, window, pixmap);
    XClearWindow(display, window);

    XFlush(display);
    XSync(display, False);

    Py_RETURN_NONE;
}

static PyMethodDef OWallpaperD_methods[] = {
    {"wait_for_workspace_change",
     (PyCFunction) OWallpaperD_wait_for_workspace_change, METH_NOARGS,
"Block until the workspace changes on a Xinerama screen and return a tuple\n"
"containing the workspace number for each Xinerama screen."
    },
    {"add_wallpaper",
     (PyCFunction) OWallpaperD_add_wallpaper, METH_VARARGS | METH_KEYWORDS,
    "Load a wallpaper into memory.\n"
    "\n"
    "Keyword arguments:\n"
    "image -- the path of the wallpaper\n"
    "mode -- mode for rendering wallpaper on screen ('center', 'fill, 'full',\n"
    "or 'tile')\n"
    "background_color -- background color when rendering"
    },
    {"set_wallpaper",
     (PyCFunction) OWallpaperD_set_wallpaper, METH_VARARGS,
"Set the current wallpaper on a given Xinerama screen to the given Wallpaper\n"
"object."
    },
    {NULL}
};

PyTypeObject OWallpaperDType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "owallpaperd.OWallpaperD",        /* tp_name */
    sizeof(OWallpaperD),              /* tp_basicsize */
    0,                                /* tp_itemsize */
    (destructor) OWallpaperD_dealloc, /* tp_dealloc */
    0,                                /* tp_print */
    0,                                /* tp_getattr */
    0,                                /* tp_setattr */
    0,                                /* tp_reserved */
    0,                                /* tp_repr */
    0,                                /* tp_as_number */
    0,                                /* tp_as_sequence */
    0,                                /* tp_as_mapping */
    0,                                /* tp_hash  */
    0,                                /* tp_call */
    0,                                /* tp_str */
    0,                                /* tp_getattro */
    0,                                /* tp_setattro */
    0,                                /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,               /* tp_flags */
    "Wallpaper switching daemon support.", /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    0,                                /* tp_iter */
    0,                                /* tp_iternext */
    OWallpaperD_methods,              /* tp_methods */
    0,                                /* tp_members */
    OWallpaperD_getset,               /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc) OWallpaperD_init,      /* tp_init */
};
