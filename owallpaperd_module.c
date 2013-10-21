#include "owallpaperd.h"

PyObject *OWallpaperDError;

static PyModuleDef owallpaperdmodule = {
    PyModuleDef_HEAD_INIT,
    "owallpaperd",
    "Module for creating a wallpaper switching daemon.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_owallpaperd(void) 
{
    PyObject* m;

    if (PyType_Ready(&OWallpaperDType) < 0)
        return NULL;

    if (PyType_Ready(&WallpaperType) < 0)
        return NULL;

    m = PyModule_Create(&owallpaperdmodule);
    if (m == NULL)
        return NULL;

    OWallpaperDError = PyErr_NewException("owallpaperd.error", NULL, NULL);
    Py_INCREF(OWallpaperDError);
    PyModule_AddObject(m, "error", OWallpaperDError);

    OWallpaperDType.tp_new = PyType_GenericNew;
    Py_INCREF(&OWallpaperDType);
    PyModule_AddObject(m, "OWallpaperD", (PyObject*) &OWallpaperDType);

    WallpaperType.tp_new = PyType_GenericNew;
    Py_INCREF(&WallpaperType);
    PyModule_AddObject(m, "Wallpaper", (PyObject*) &WallpaperType);
    return m;
}
