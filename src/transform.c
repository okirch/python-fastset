/*
fastsets - transform objects

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

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "fastsets.h"
#include <structmember.h>

static PyObject *	Fastset_newTransform(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		Fastset_initTransform(fastset_Transform *self, PyObject *args, PyObject *kwds);
static void		Fastset_deallocTransform(fastset_Transform *self);
static PyObject *	FastsetTransform_Call(fastset_Transform *callable, PyObject *args, PyObject *kwargs);
static PyObject *	FastsetTransform_Update(fastset_Transform *callable, PyObject *args, PyObject *kwargs);

static PyMethodDef fastset_transformMethods[] = {
      { "update", (PyCFunction) FastsetTransform_Update, METH_VARARGS | METH_KEYWORDS,
        "update the transform for one input value"
      },
      { NULL }
};


PyTypeObject	fastset_TransformType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "transform",
	.tp_basicsize	= sizeof(fastset_Transform),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= NULL,

	.tp_methods	= fastset_transformMethods,
	.tp_init	= (initproc) Fastset_initTransform,
	.tp_new		= Fastset_newTransform,
	.tp_dealloc	= (destructor) Fastset_deallocTransform,
	.tp_call	= (ternaryfunc) FastsetTransform_Call,
};


static PyObject *
Fastset_newTransform(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	fastset_Transform *self;

	self = (fastset_Transform *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->domain = NULL;
	self->bittrans = NULL;

	return (PyObject *) self;
}

static int
Fastset_initTransform(fastset_Transform *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"domain",
		"function",
		NULL
	};
	PyObject *domainObject = NULL;
	PyObject *functionObject = NULL;
	PyObject *callArgs = NULL;
	unsigned int i;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &domainObject, &functionObject))
		return -1;

	if (!FastsetDomain_Check(domainObject)) {
		PyErr_SetString(PyExc_ValueError, "first argument must be a fastset domain instance");
		return -1;
	}

	self->domain = (fastset_Domain *) domainObject;
	Py_INCREF(domainObject);

	self->bittrans = fastset_bitvec_transform_new(self->domain->count);

	/* Initialize undefined transform that can be set up using transform.update() */
	if (functionObject == NULL)
		return 0;

	callArgs = PyTuple_New(1);
	for (i = 0; i < self->domain->count; ++i) {
		fastset_Member *arg_member = (fastset_Member *) self->domain->domain_objects[i];
		fastset_Member *res_member;
		PyObject *result;

		if (arg_member == NULL)
			continue;

		assert(arg_member->index == i);

		PyTuple_SetItem(callArgs, 0, (PyObject *) arg_member);
		Py_INCREF(arg_member);

		result = PyObject_Call(functionObject, callArgs, NULL);
		if (result == NULL)
			goto failed;

		if (!FastsetDomain_IsMember(self->domain, result)) {
			PyErr_SetString(PyExc_RuntimeError, "return value of mapping function is not compatible with domain");
			goto failed;
		}

		res_member = (fastset_Member *) result;
		if (res_member->index < 0) {
			PyErr_SetString(PyExc_RuntimeError, "return value of mapping function is an uninitialized domain member");
			goto failed;
		}

		fastset_bitvec_transform_add(self->bittrans, i, res_member->index);
	}

	Py_CLEAR(callArgs);
	return 0;

failed:
	Py_CLEAR(self->domain);
	Py_CLEAR(callArgs);
	return -1;
}

static void
Fastset_deallocTransform(fastset_Transform *self)
{
	Py_CLEAR(self->domain);

	if (self->bittrans) {
		fastset_bitvec_transform_free(self->bittrans);
		self->bittrans = NULL;
	}
}

PyObject *
FastsetTransform_Call(fastset_Transform *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"argument",
		NULL
	};
	PyObject *argObject = NULL;
	fastset_Domain *domain;
	PyObject *result = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &argObject))
		return NULL;

	if (!(domain = Fastset_DSTGetDomain(argObject)))
		return NULL;

	if (self->domain != domain) {
		PyErr_SetString(PyExc_ValueError, "argument is from a different domain");
		return NULL;
	}

	if (FastsetDomain_IsSet(self->domain, argObject)) {
		result = FastsetSet_TransformBitvec((fastset_Set *) argObject, self->bittrans);
	} else {
		PyErr_SetString(PyExc_ValueError, "unsupported argument type");
		return NULL;
	}

	return result;
}

static inline int
fastset_objectToMemberIndex(fastset_Transform *self, PyObject *obj, const char *name)
{
	fastset_Member *member;

	if (!FastsetDomain_IsMember(self->domain, obj)) {
		PyErr_Format(PyExc_ValueError, "%s is from a different domain", name);
		return -1;
	}

	member = (fastset_Member *) obj;
	if (member->index < 0) {
		PyErr_Format(PyExc_RuntimeError, "%s is an uninitialized domain member", name);
		return -1;
	}

	return member->index;
}

PyObject *
FastsetTransform_Update(fastset_Transform *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"argument",
		"result",
		NULL
	};
	PyObject *argObject = NULL, *resObject = NULL;
	int arg_index, res_index;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &argObject, &resObject))
		return NULL;

	if ((arg_index = fastset_objectToMemberIndex(self, argObject, "argument")) < 0)
		return NULL;

	if ((res_index = fastset_objectToMemberIndex(self, resObject, "result")) < 0)
		return NULL;

	fastset_bitvec_transform_add(self->bittrans, arg_index, res_index);

	Py_INCREF(Py_None);
	return Py_None;
}
