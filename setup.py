from distutils.core import setup, Extension

base_module = Extension('owallpaperd',
        libraries=['X11', 'Xinerama', 'Imlib2'],
        sources= ['owallpaperd_module.c', 'owallpaperd_object.c',
                  'wallpaper_object.c', 'helper.c'])

setup (name = 'owallpaperd',
        version = '1.0',
        description = 'Module for creating a wallpaper switching daemon.',
        ext_modules = [base_module])
