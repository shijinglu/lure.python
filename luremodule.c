#include <Python.h>
#include "lure.h"

#define EXPR_PY_CAPSULE_NAME "lure_expr"

bool evalWrap(void *ptr, void *ctx) {
    if (!ptr || !ctx) return false;
    return eval((map_t)ctx, (Expr *)ptr);
}

void exprDestructor(PyObject *capsule) {
    Expr *opaque_ptr = (Expr *)PyCapsule_GetPointer(capsule, EXPR_PY_CAPSULE_NAME);
    freeExpr(opaque_ptr);
}

static PyObject * reCompile(PyObject *self, PyObject *args) {
    char * s = NULL;
    void * exprPtr = NULL;
    if (!PyArg_ParseTuple(args, "s", &s))
        return NULL;
    exprPtr = (void *)compile(s);
    if (exprPtr == NULL)
        return NULL;
    return PyCapsule_New(exprPtr, EXPR_PY_CAPSULE_NAME, exprDestructor);
}

static PyObject *reEval(PyObject *self, PyObject *args) {
    void *cPtr = NULL;
    PyObject *pyPtr = NULL;
    if (!PyArg_ParseTuple(args, "O", &pyPtr))
        return NULL;
     cPtr = PyCapsule_GetPointer(pyPtr, EXPR_PY_CAPSULE_NAME);
     if (cPtr == NULL)
        return NULL;
    bool result = eval(NULL, (Expr *)cPtr);
    return Py_BuildValue("O", result ? Py_True : Py_False);
}

static PyObject * version(PyObject *self) {
    return Py_BuildValue("s", "Version 1.0");
}

static PyMethodDef myMethods[] = {
    {"compile", reCompile, METH_VARARGS, "Compile an rule, expr = compile('CITY_ID == 1') "},
    {"eval", reEval, METH_VARARGS, "Evaluate expression like eval(expr, ctx) "},
    {"version", (PyCFunction)version, METH_NOARGS, "return the version"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef myModule = {
    PyModuleDef_HEAD_INIT,
    "lure", 
    "Lu's Rule Engine",
    -1,
    myMethods,
};

PyMODINIT_FUNC PyInit_myModule(void) {
    return PyModule_Create(&myModule);
};
#else
PyMODINIT_FUNC inithello() {
    Py_InitModule3("hello", myMethods, "mod doc");
}
#endif