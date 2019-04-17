from distutils.core import setup, Extension

module = Extension(
    "pylure", 
    sources = [
        "luremodule.c",
        'clib/ast_builder.c',
        'clib/data_bool.c',
        'clib/data_double.c',
        'clib/data_int.c',
        'clib/data_string.c',
        'clib/data_version.c',
        'clib/function.c',
        'clib/function_md5mod.c',
        'clib/hashmap.c',
        'clib/lex.yy.c',
        'clib/lure.c',
        'clib/md5hash.c',
        'clib/node.c',
        'clib/node_binop.c',
        'clib/node_function.c',
        'clib/node_identity.c',
        'clib/node_in.c',
        'clib/node_like.c',
        'clib/node_list.c',
        'clib/node_literal.c',
        'clib/re.c',
        'clib/util.c',
        'clib/y.tab.c',
    ],
    include_dirs=["clib/"],
)

setup(
    name = "Lure",
    version = "1.0",
    description = "A Simple Rule Engine in Python",
    ext_modules = [module],
)