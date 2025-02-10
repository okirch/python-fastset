/*
fastsets - member objects

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
#include "fastsets.h"

static PyObject *	Fastset_newMember(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		Fastset_initMember(fastset_Member *self, PyObject *args, PyObject *kwds);
static void		Fastset_deallocMember(fastset_Member *self);

static PyMethodDef fastset_memberMethods[] = {
	{ NULL }
};

PyTypeObject	fastset_MemberTypeTemplate = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= NULL,
	.tp_basicsize	= sizeof(fastset_Member),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= NULL,

	.tp_methods	= fastset_memberMethods,
	.tp_init	= (initproc) Fastset_initMember,
	.tp_new		= Fastset_newMember,
	.tp_dealloc	= (destructor) Fastset_deallocMember,
};


static PyObject *
Fastset_newMember(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	fastset_Member *self;

	self = (fastset_Member *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->domain = NULL;
	self->index = -1;

	return (PyObject *) self;
}

static int
Fastset_initMember(fastset_Member *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		NULL
	};
	fastset_Domain *domain;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
		return -1;

	if (!(domain = Fastset_DSTGetDomain((PyObject *) self)))
		return -1;

	Py_INCREF(domain);
	self->domain = domain;

	FastsetDomain_register(domain, self);
	return 0;
}

static void
Fastset_deallocMember(fastset_Member *self)
{
	if (self->index >= 0)
		FastsetDomain_unregister(self->domain, self);
	Py_CLEAR(self->domain);
}
