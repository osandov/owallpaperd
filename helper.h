#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

/** The mode in which to render wallpapers */
typedef enum {
    WALLPAPER_MODE_NONE,
    WALLPAPER_MODE_CENTER,
    WALLPAPER_MODE_FILL,
    WALLPAPER_MODE_FULL,
    WALLPAPER_MODE_TILE
} WallpaperMode;

/**
 * Create a desktop window to cover an entire Xinerama screen; this is the
 * window on which we set the background image to the wallpaper.
 */
Window create_desktop_window(Display *display, int screen,
                             XineramaScreenInfo *info);

/** Get an array of the current workspace on each Xinerama screen. */
long *get_workspaces(Display *display, int screen, int num_screens);

/** Get the current X server time. */
Time get_current_time(Display *display, int screen);

/**
 * Convert a string to a WallpaperMode: valid strings are "center", "fill",
 * "full", and "tile".
 */
WallpaperMode wallpaper_mode_from_string(const char *mode_string);

/**
 * Create a wallpaper pixmap.
 * @param window The desktop window to make the wallpaper for.
 * @param info Xinerama screen info.
 * @param image_path The path for the image file.
 * @param mode The mode for rendering the wallpaper.
 * @param background_color The background color on which to render the
 * wallpaper.
 * @param pixmap_out Return for the rendered pixmap.
 * @return Zero on success, non-zero on failure.
 */
int create_wallpaper(Display *display, int screen, Window window,
                     XineramaScreenInfo *info,
                     const char *image_path, WallpaperMode mode,
                     unsigned long background_color, Pixmap *pixmap_out);
