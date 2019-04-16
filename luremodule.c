#include <Python.h>
#include "clib/lure.h"
#include "clib/logger.h"
#include "clib/node.h"

#define EXPR_PY_CAPSULE_NAME "capsule_name_lure_expr"
#define CONTEXT_PY_CAPSULE_NAME "capsule_name_lure_context"


void exprDestructor(PyObject *capsule) {
    Node *opaque_ptr = (Node *)PyCapsule_GetPointer(capsule, EXPR_PY_CAPSULE_NAME);
    free_node_deep(opaque_ptr);
}

static PyObject * reCompile(PyObject *self, PyObject *args) {
    char * s = NULL;
    void * exprPtr = NULL;
    if (!PyArg_ParseTuple(args, "s", &s))
        return NULL;
    
    exprPtr = (void *)lure_compile(s);
    if (exprPtr == NULL)
        return NULL;
    return PyCapsule_New(exprPtr, EXPR_PY_CAPSULE_NAME, exprDestructor);
}

static PyObject *reEval(PyObject *self, PyObject *args) {
    void *exprPtrUnsafe = NULL;
    void *ctxPtrUnsafe = NULL;
    PyObject *exprPtr = NULL;
    PyObject *ctxPtr = NULL;

    if (!PyArg_ParseTuple(args, "OO", &exprPtr, &ctxPtr))
        return NULL;
    exprPtrUnsafe = PyCapsule_GetPointer(exprPtr, EXPR_PY_CAPSULE_NAME);
    if (exprPtrUnsafe == NULL)
        return NULL;
    ctxPtrUnsafe = PyCapsule_GetPointer(ctxPtr, CONTEXT_PY_CAPSULE_NAME);
    if (ctxPtrUnsafe == NULL) {
        return NULL;
    }
    
    bool result = lure_compile_eval((Node *)exprPtrUnsafe, (ContextPtr)ctxPtrUnsafe);
    return Py_BuildValue("O", result ? Py_True : Py_False);
}

static PyObject * version(PyObject *self) {
    return Py_BuildValue("s", "Version 1.0");
}

static PyMethodDef lureMethods[] = {
    {"compile", reCompile, METH_VARARGS, "Compile an rule, expr = compile('CITY_ID == 1') "},
    {"eval", reEval, METH_VARARGS, "Evaluate expression like eval(expr, ctx) "},
    {"version", (PyCFunction)version, METH_NOARGS, "return the version"},
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
    return PyModule_Create(&lureModule);
};
#else
PyMODINIT_FUNC PyInit_pylure() {
    Py_InitModule3("pylure", lureMethods, "mod doc");
}
#endif