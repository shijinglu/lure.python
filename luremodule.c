#include <Python.h>
#include <string.h>
#include "clib/lure.h"
#include "clib/logger.h"
#include "clib/node.h"

#define EXPR_PY_CAPSULE_NAME "capsule_name_lure_expr"
#define CONTEXT_PY_CAPSULE_NAME "capsule_name_lure_context"
#define DEFAULT_CONTEXT_SIZE 32

/* pylure.Context()
 *   .addInt("CITY_ID", 123)
 *   .addDouble("PI", 3.14)
 *   .addString("USER", "alice")
 */
typedef struct {
    PyObject_HEAD;
    ContextPtr ctx;
    char **all_keys; /* copy of all keys to avoid keys being gc-ed */
    int n_keys; 
    int keys_cap; /* capacity to hold keys. */
} Context;

/* create a duplication of the given key, add it to all_keys, and return the dup key. */
static char *context_dupkey(Context *ctx, const char * key) {
    if (ctx->n_keys + 2 >= ctx->keys_cap ) {
        ctx->keys_cap *= 2;
        ctx->all_keys = (char **)realloc(ctx->all_keys, ctx->keys_cap * sizeof(char *));
    }
    char *key2 = strdup(key);
    ctx->all_keys[ctx->n_keys++] = key2;
    return key2;
}

static void Context_dealloc(Context *self) {
    for(int i = 0; i < self->n_keys; i++) {
        free(self->all_keys[i]);
    }
    free(self->all_keys);
    hashmap_free((map_t)self->ctx);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Context_init(PyObject *self, PyObject *args, PyObject *kwords) {
    Context *c_self = (Context *)self;
    c_self->ctx = (ContextPtr)hashmap_new();
    if (c_self->ctx == NULL)
        return -1;
    c_self->keys_cap = DEFAULT_CONTEXT_SIZE;
    c_self->all_keys = (char **)calloc(c_self->keys_cap, sizeof(char *));
    c_self->n_keys = 0;
    return 0;
}

static PyObject * Context_addBool(Context *self, PyObject *args) {
    char *key;
    int val;
    PyArg_ParseTuple(args, "si", &key, &val);
    setBoolContext(self->ctx, context_dupkey(self, key), (bool)val);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject * Context_addInt(Context *self, PyObject *args) {
    char *key;
    int val;
    PyArg_ParseTuple(args, "si", &key, &val);
    setIntContext(self->ctx, context_dupkey(self, key), val);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject * Context_addDouble(Context *self, PyObject *args) {
    char *key;
    double val;
    PyArg_ParseTuple(args, "sd", &key, &val);
    setDoubleContext(self->ctx, context_dupkey(self, key), val);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject * Context_addString(Context *self, PyObject *args) {
    char *key;
    char *val;
    PyArg_ParseTuple(args, "ss", &key, &val);
    setStringContext(self->ctx, context_dupkey(self, key), val);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject * Context_addCustom(Context *self, PyObject *args) {
    char *key;
    char *val;
    char *ext_key;
    PyArg_ParseTuple(args, "sss", &key, &val, &ext_key);
    setCustomContext(self->ctx, context_dupkey(self, key), val, ext_key);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject * Context_toDict(Context *self, PyObject *ignored) {
    PyObject *dict, *key, *val;
    Data *data;
    char *c_key;
    int ok;

    dict = PyDict_New();
    for(size_t i = 0; i < self->n_keys; i++) {
        c_key = self->all_keys[i];
        key = Py_BuildValue("s", c_key);
        ok = hashmap_get((map_t)self->ctx, c_key, (void **)&data);
        if (ok == MAP_OK) {
            switch (data->rawType)
            {
                case RawDataBool:
                case RawDataInt:
                    val = Py_BuildValue("i", data->toInt(data));
                    break;
                case RawDataDouble:
                    val = Py_BuildValue("d", data->toDouble(data));
                    break;
                case RawDataString:
                case RawDataCustom:
                    val = Py_BuildValue("s", data->getCStr(data));
                    break;
                default:
                    val = Py_BuildValue(""); /* None */
                    break;
            }
        }
        PyDict_SetItem(dict, key, val);
    }
    return dict;
}

static PyMethodDef Context_methods[] = {
    {"addBool", (PyCFunction) Context_addBool, METH_VARARGS, "Add a boolean context"},
    {"addInt", (PyCFunction) Context_addInt, METH_VARARGS, "Add an integer context"},
    {"addDouble", (PyCFunction) Context_addDouble, METH_VARARGS, "Add a double context"},
    {"addString", (PyCFunction) Context_addString, METH_VARARGS, "Add a stirng context"},
    {"addCustom", (PyCFunction) Context_addCustom, METH_VARARGS, "Add a customized context (extended through lure.c plugins). "},
    {"toDict", (PyCFunction) Context_toDict, METH_VARARGS, "Map context entries to a python dict. "},
    {NULL}  /* Sentinel */
};

static PyTypeObject ContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylure.Context",
    .tp_doc = "Context objects",
    .tp_basicsize = sizeof(Context),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor) Context_dealloc,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Context_init,
    .tp_methods = Context_methods,
};

void exprDestructor(PyObject *capsule) {
    Node *opaque_ptr = (Node *)PyCapsule_GetPointer(capsule, EXPR_PY_CAPSULE_NAME);
    free_node_deep(opaque_ptr);
}

static PyObject * reCompile(PyObject *self, PyObject *args) {
    char * s = NULL;
    void * exprPtr = NULL;
    if (!PyArg_ParseTuple(args, "s", &s))
        goto compile_none;

    exprPtr = (void *)lure_compile(s);
    if (exprPtr == NULL)
        goto compile_none;

    return PyCapsule_New(exprPtr, EXPR_PY_CAPSULE_NAME, exprDestructor);
compile_none:
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *reEval(PyObject *self, PyObject *args) {
    Node *c_node = NULL;
    Context *c_ctx = NULL;
    PyObject *exprPtr = NULL;
    PyObject *ctxPtr = NULL;
    if (!PyArg_ParseTuple(args, "OO", &exprPtr, &ctxPtr))
        goto eval_none;

    c_node = (Node *)PyCapsule_GetPointer(exprPtr, EXPR_PY_CAPSULE_NAME);
    if (c_node == NULL)
        goto eval_none;

    c_ctx = (Context *) ctxPtr;
    bool result = lure_compile_eval(c_node, c_ctx->ctx);
    return Py_BuildValue("O", result ? Py_True : Py_False);
eval_none:
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject * version(PyObject *self) {
    return Py_BuildValue("s", "Version 1.0");
}

static PyMethodDef lureMethods[] = {
    {"compile", reCompile, METH_VARARGS, "Compile an rule, expr = compile('CITY_ID == 1') "},
    {"eval", reEval, METH_VARARGS, "Evaluate expression like eval(expr, ctx) "},
    {"version", (PyCFunction)version, METH_NOARGS, "Return the version string"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef lureModule = {
    PyModuleDef_HEAD_INIT,
    "pylure",               /* m_name */
    "Lu's Rule Engine",     /* m_doc */
    -1,                     /* m_size */
    lureMethods,            /* m_methods */
};

PyMODINIT_FUNC PyInit_pylure(void) {
    PyObject *m;
    if (PyType_Ready(&ContextType) < 0)
        return NULL;
    m = PyModule_Create(&lureModule);
    if (m == NULL)
        return NULL;
    Py_INCREF(&ContextType);
    PyModule_AddObject(m, "Context", (PyObject *) &ContextType);
    return m;
};
#else
PyMODINIT_FUNC initpylure() {
    PyObject *m;
    if (PyType_Ready(&ContextType) < 0)
        return;        
    m = Py_InitModule3("pylure", lureMethods, "Lu's Rule Engine");
    if (m == NULL)
        return;
    Py_INCREF(&ContextType);
    PyModule_AddObject(m, "Context", (PyObject *) &ContextType);
}
#endif