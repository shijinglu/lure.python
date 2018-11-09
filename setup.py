from distutils.core import setup, Extension

module = Extension(
    "lureModule", 
    sources = [
        "luremodule.c",
        "clib/hashmap.c",
        "clib/lex.yy.c",
        "clib/lure.c",
        "clib/lure_extension.c",
        "clib/lure_main.c",
        "clib/md5hash.c",
        "clib/y.tab.c",
    ],
    include_dirs=["clib/"],
    )

setup(
    name = "Lure",
    version = "1.0",
    description = "A Simple Rule Engine in Python",
    ext_modules = [module],
)