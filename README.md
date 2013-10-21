OWallpaperD
===========
OWallpaperD is a Python module coded in C for writing a wallpaper switching
daemon. It relies on the window manager setting the `OWALLPAPERD_WORKSPACES`
atom on the root window. This is implemented as an XMonad extension in my
configuration, which isn't in xmonad-contrib but can be found in my dotfiles
repo. (There's no reason that support can't be implemented in other window
managers, I just happen to use XMonad.) OWallpaperD also depends on Xinerama
and Imlib2.

The module is imported as `owallpaperd` and exports the main object,
`OWallpaperD`, which encapsulates all of the necessary state for the wallpaper
daemon. Wallpapers are added with the `add_wallpaper` method and changed per
Xinerama screen with `set_wallpaper`. The object also provides a
`wait_for_workspace_change` method which blocks until the workspace changes on
some Xinerama screen and returns a tuple containing which workspace is visible
on each screen. An example is included.
