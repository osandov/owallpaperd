#include <errno.h>
#include <string.h>
#include <Imlib2.h>
#include "helper.h"

/* See helper.h. */
Window create_desktop_window(Display *display, int screen,
                             XineramaScreenInfo *info)
{
    Atom _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_DESKTOP;
    Window root_window, window;
    XClassHint *class_hint;
    short x, y, w, h;

    /* Create the desktop window to cover an entire Xinerama screen */
    x = info->x_org;
    y = info->y_org;
    w = info->width;
    h = info->height;

    root_window = RootWindow(display, screen);
    window = XCreateSimpleWindow(display, root_window, x, y, w, h, 0, 0, 0);

    /* Set the window class */
    class_hint = XAllocClassHint();
    class_hint->res_name = "desktop_window";
    class_hint->res_class = "OWallpaperD";
    XSetClassHint(display, window, class_hint);
    XFree(class_hint);

    /* Set the window type */
    _NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    _NET_WM_WINDOW_TYPE_DESKTOP =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(display, window, _NET_WM_WINDOW_TYPE, XA_ATOM, 32,
                    PropModeReplace,
                    (unsigned char*) &_NET_WM_WINDOW_TYPE_DESKTOP, 1);

    XLowerWindow(display, window);
    return window;
}

/* See helper.h. */
long *get_workspaces(Display *display, int screen, int num_screens)
{
    Atom OWALLPAPERD_WORKSPACES, r_type;
    int r_format, status;
    unsigned long left = (unsigned long) num_screens;
    unsigned long actual;

    long *workspaces = NULL;

    OWALLPAPERD_WORKSPACES =
        XInternAtom(display, "OWALLPAPERD_WORKSPACES", False);
   
    do {
        status = XGetWindowProperty(display, RootWindow(display, screen),
                                    OWALLPAPERD_WORKSPACES, 0L, left, False,
                                    XA_CARDINAL, &r_type, &r_format, &actual,
                                    &left, (unsigned char**) &workspaces);

        if (status != Success || r_type != XA_CARDINAL || actual == 0) {
            if (workspaces) {
                XFree(workspaces);
                workspaces = NULL;
            }
            goto out;
        }
    } while (left);

out:
    return workspaces;
}

static Bool timestamp_predicate(Display *display, XEvent *xevent, XPointer arg)
{
    Window window = (Window) arg;
    Atom timestamp_atom;

    timestamp_atom = XInternAtom(display, "OWALLPAPERD_TIMESTAMP_PROP", False);

    return xevent->type == PropertyNotify &&
           xevent->xproperty.window == window &&
           xevent->xproperty.atom == timestamp_atom;
}

/* See helper.h. */
Time get_current_time(Display *display, int screen)
{
    Window window;
    XEvent event;
    const unsigned char c = 'a';
    Atom timestamp_atom;

    window = RootWindow(display, screen);
    timestamp_atom = XInternAtom(display, "OWALLPAPERD_TIMESTAMP_PROP", False);
    if (timestamp_atom == None)
        return 0;

    XChangeProperty(display, window, timestamp_atom, timestamp_atom, 8,
                    PropModeReplace, &c, 1);
    XIfEvent(display, &event, timestamp_predicate, (XPointer) window);

    return event.xproperty.time;
}

/* See helper.h. */
WallpaperMode wallpaper_mode_from_string(const char *mode_string)
{
    if (!mode_string)
        return WALLPAPER_MODE_FULL;
    else if (strcmp(mode_string, "center") == 0)
        return WALLPAPER_MODE_CENTER;
    else if (strcmp(mode_string, "fill") == 0)
        return WALLPAPER_MODE_FILL;
    else if (strcmp(mode_string, "full") == 0)
        return WALLPAPER_MODE_FULL;
    else if (strcmp(mode_string, "tile") == 0)
        return WALLPAPER_MODE_TILE;
    else
        return WALLPAPER_MODE_NONE;
}

/** Render the given image file. Code adapted from hsetroot.
 * @param root_image Imlib2 context on which to render.
 * @param image_path Path of the image file.
 * @param mode Mode for rendering image onto root_image.
 * @param root_width Width of root_image.
 * @param root_height Height of root image.
 * @return Zero on success, non-zero on failure.
 */
static int render_wallpaper(Imlib_Image root_image, const char *image_path,
                            WallpaperMode mode, unsigned int root_width,
                            unsigned int root_height)
{
    Imlib_Image buffer = 0;
    int image_width, image_height;
    int error = 0;
    int top, left, x, y;
    double aspect;

    buffer = imlib_load_image(image_path);

    if (!buffer) {
        error = EINVAL;
        goto out;
    }

    imlib_context_set_image(buffer);
    image_width = imlib_image_get_width();
    image_height = imlib_image_get_height();

    imlib_context_set_image(root_image);

    switch (mode) {
        case WALLPAPER_MODE_CENTER:
            left = (root_width - image_width) / 2;
            top = (root_height - image_height) / 2;
            imlib_blend_image_onto_image(buffer, 0, 0, 0,
                                         image_width, image_height, 0, 0,
                                         image_width, image_height);
            break;
        case WALLPAPER_MODE_FILL:
            imlib_blend_image_onto_image(buffer, 0, 0, 0,
                                         image_width, image_height, 0, 0,
                                         root_width, root_height);
            break;
        case WALLPAPER_MODE_FULL:
            aspect = (double) root_width / image_width;
            if ((int) (image_height * aspect) > root_height)
                aspect = (double) root_height / image_height;
            top = (root_height - (int) (image_height * aspect)) / 2;
            left = (root_width - (int) (image_width * aspect)) / 2;
            imlib_blend_image_onto_image(buffer, 0, 0, 0,
                                         image_width, image_height,
                                         left, top,
                                         (int) (image_width * aspect),
                                         (int) (image_height * aspect));
            break;
        case WALLPAPER_MODE_TILE:
            left = (root_width - image_width) / 2;
            top = (root_height - image_height) / 2;

            while (left > 0)
                left -= image_width;
            while (top > 0)
                top -= image_height;

            for (x = left; x < root_width; x += image_width) {
                for (y = top; y < root_height; y += image_height) {
                    imlib_blend_image_onto_image(buffer, 0, 0, 0,
                                                 image_width, image_height,
                                                 x, y,
                                                 image_width, image_height);
                }
            }
            break;
        default:
            error = ENOSYS;
    }

out:
    if (buffer) {
        imlib_context_set_image(buffer);
        imlib_free_image();
    }
    imlib_context_set_image(root_image);
    return error;
}

/** See helper.h. Code adapted from hsetroot. */
int create_wallpaper(Display *display, int screen, Window window,
                     XineramaScreenInfo *info,
                     const char *image_path, WallpaperMode mode,
                     unsigned long background_color, Pixmap *pixmap_out)
{
    Imlib_Context *context;
    Imlib_Image image;
    Pixmap pixmap;
    Visual *visual;
    Colormap colormap;
    unsigned int width, height, depth;
    int error;

    context = imlib_context_new();
    imlib_context_push(context);

    imlib_context_set_display(display);
    visual = DefaultVisual(display, screen);
    colormap = DefaultColormap(display, screen);
    width = info->width;
    height = info->height;
    depth = DefaultDepth(display, screen);

    pixmap = XCreatePixmap(display, window, width, height, depth);

    imlib_context_set_visual(visual);
    imlib_context_set_colormap(colormap);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color_range(imlib_create_color_range());

    image = imlib_create_image(width, height);
    imlib_context_set_image(image);

    imlib_context_set_color((background_color & 0xff0000) >> 16,
                            (background_color & 0xff00) >> 8, 
                            (background_color & 0xff), 0xff);
    imlib_image_fill_rectangle(0, 0, width, height);

    imlib_context_set_dither(1);
    imlib_context_set_blend(1);

    error = render_wallpaper(image, image_path, mode, width, height);
    if (error)
        return error;

    imlib_render_image_on_drawable(0, 0);
    imlib_free_image();
    imlib_free_color_range();
    imlib_context_pop();
    imlib_context_free(context);

    *pixmap_out = pixmap;
    return 0;
}
