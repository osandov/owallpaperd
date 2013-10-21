import owallpaperd
import random
import os.path
import time
import random

wallpapers = [
    "~/pokemon/eevee.jpg",
    "~/pokemon/vaporeon.jpg",
    "~/pokemon/jolteon.jpg",
    "~/pokemon/flareon.jpg",
    "~/pokemon/espeon.jpg",
    "~/pokemon/umbreon.jpg",
    "~/pokemon/leafeon.jpg",
    "~/pokemon/glaceon.jpg"
]

wd = owallpaperd.OWallpaperD()

for wallpaper in [os.path.expanduser(w) for w in wallpapers]:
    wd.add_wallpaper(wallpaper)

while True:
    try:
        ws = wd.wait_for_workspace_change()
        for (s, ws) in enumerate(ws):
            wd.set_wallpaper(s, wd.wallpapers[ws % len(wd.wallpapers)])
    except KeyboardInterrupt:
        break
