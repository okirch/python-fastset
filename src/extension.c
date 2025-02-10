/*
fastsets

Copyright (C) 2023 SUSE

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <fcntl.h>
#include <sys/wait.h>

#include "fastsets.h"

/*
 * Methods belonging to the module itself.
 * None so far
 */
static PyMethodDef fastset_methods[] = {
      {	NULL }
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
# define PyMODINIT_FUNC void
#endif

static void
fastset_registerType(PyObject *m, const char *name, PyTypeObject *type)
{
	if (PyType_Ready(type) < 0)
		return;

	Py_INCREF(type);
	PyModule_AddObject(m, name, (PyObject *) type);
}

PyObject *
fastset_importType(const char *moduleName, const char *typeName)
{
	PyObject *module, *type;

	module = PyImport_ImportModule(moduleName);
	if (module == NULL)
		return NULL;

	type = PyDict_GetItemString(PyModule_GetDict(module), typeName);
	if (type == NULL) {
		PyErr_Format(PyExc_TypeError, "%s.%s: not defined", moduleName, typeName);
		return NULL;
	}

	if (!PyType_Check(type)) {
		PyErr_Format(PyExc_TypeError, "%s.%s: not a type object", moduleName, typeName);
		return NULL;
	}
	return type;
}

PyObject *
fastset_callType(PyTypeObject *typeObject, PyObject *args, PyObject *kwds)
{
	PyObject *obj;

	if (args == NULL) {
		args = PyTuple_New(0);
		obj = PyObject_Call((PyObject *) typeObject, args, NULL);
		Py_DECREF(args);
	} else {
		obj = PyObject_Call((PyObject *) typeObject, args, kwds);
	}

	return obj;
}

static struct PyModuleDef fastset_module_def = {
	PyModuleDef_HEAD_INIT,
	"fastset",		/* m_name */
	"Provide very fast operations for sets over a defined domain",
				/* m_doc */
	-1,			/* m_size */
	fastset_methods,	/* m_methods */
	NULL,			/* m_reload */
	NULL,			/* m_traverse */
	NULL,			/* m_clear */
	NULL,			/* m_free */
};



PyMODINIT_FUNC
PyInit_fastsets(void)
{
	PyObject* m;

	m = PyModule_Create(&fastset_module_def);

	fastset_registerType(m, "Domain", &fastset_DomainType);
	fastset_registerType(m, "Transform", &fastset_TransformType);
	fastset_registerType(m, "iterator", &fastset_SetIteratorType);
	return m;
}
