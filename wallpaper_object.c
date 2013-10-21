#include "owallpaperd.h"

static void Wallpaper_dealloc(Wallpaper *self)
{
    Py_ssize_t i;
    if (self->pixmaps) {
        for (i = 0; i < self->num_screens; ++i) {
            Pixmap pixmap = self->pixmaps[i];
            if (pixmap)
                XFreePixmap(self->display, pixmap);
        }
        PyMem_Free(self->pixmaps);
    }
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static int Wallpaper_init(Wallpaper *self, PyObject *args, PyObject *kwds)
{
    Py_ssize_t i;
    PyObject *owallpaperD_o;
    OWallpaperD *owallpaperD;

    const char *image_path;
    const char *mode_string = NULL;
    WallpaperMode mode;
    uint32_t background_color = 0x0;

    static char *kwlist[] = {"owallpaperD", "image", "mode",
                             "background_color", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Os|sI", kwlist,
                                     &owallpaperD_o, &image_path, &mode_string,
                                     &background_color))
        return -1;

    if (!PyObject_TypeCheck(owallpaperD_o, &OWallpaperDType)) {
        PyErr_SetString(OWallpaperDError,
            "first argument must be a OWallpaperD object");
        return -1;
    }

    owallpaperD = (OWallpaperD*) owallpaperD_o;
    self->display = owallpaperD->display;
    self->screen = owallpaperD->screen;
    self->num_screens = owallpaperD->num_screens;

    mode = wallpaper_mode_from_string(mode_string);
    if (mode == WALLPAPER_MODE_NONE) {
        PyErr_SetString(OWallpaperDError,
                        "unknown wallpaper mode (should be 'center', 'fill', 'full', or 'tile'");
        return -1;
    }
    
    /* Create the wallpaper pixmap for each Xinerama screen */
    self->pixmaps = PyMem_New(Pixmap, self->num_screens);
    if (!self->pixmaps)
        return -1;
    memset(self->pixmaps, 0, sizeof(Pixmap) * self->num_screens);
    for (i = 0; i < self->num_screens; ++i) {
        Pixmap pixmap;
        int error;
        error = create_wallpaper(self->display, self->screen,
                                 owallpaperD->windows[i],
                                 &owallpaperD->screens[i], image_path,
                                 mode, background_color, &pixmap);
        if (error) {
            if (error == EINVAL)
                PyErr_SetString(OWallpaperDError, "could not load image file");
            else if (error == ENOSYS)
                PyErr_SetString(OWallpaperDError,
                                "unimplemented wallpaper mode");
            else
                PyErr_SetString(OWallpaperDError,
                                "unknown error loading wallpaper");
            return -1;
        }
        self->pixmaps[i] = pixmap;
    }

    return 0;
}

PyTypeObject WallpaperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "owallpaperd.Wallpaper",        /* tp_name */
    sizeof(Wallpaper),              /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor) Wallpaper_dealloc, /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_reserved */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash  */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,             /* tp_flags */
    "Wallpaper object.",            /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc) Wallpaper_init,      /* tp_init */
};
