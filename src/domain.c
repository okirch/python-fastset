/*
fastsets - domain objects

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

static PyObject *	Fastset_newDomain(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		Fastset_initDomain(fastset_Domain *self, PyObject *args, PyObject *kwds);
static void		Fastset_deallocDomain(fastset_Domain *self);
static PyObject *	Fastset_getDomainName(fastset_Domain *self, void *closure);

static PyMemberDef	domain_TypeMembers[] = {
	{ "set", T_OBJECT_EX, offsetof(fastset_Domain, set_class), READONLY, },
	{ "member", T_OBJECT_EX, offsetof(fastset_Domain, member_class), READONLY, },
	{ NULL, }
};

static PyGetSetDef	domain_Getters[] = {
	{ "name", (getter) Fastset_getDomainName, },
	{ NULL, }
};

static PyTypeObject *
fastset_DSTAlloc(fastset_Domain *domain, const PyTypeObject *typeTemplate, const char *typeName)
{
	fastset_DomainSpecificType *dst;
	const char *domainName = domain->name;
	PyTypeObject *newType;

	dst = calloc(1, sizeof(*dst));
	dst->magic = FASTSET_DST_MAGIC;
	dst->domain = domain;
	/* Note we do not INCREF domain here, else we would create a circular ref */

	newType = &dst->base;
	*newType = *typeTemplate;

	asprintf((char **) &newType->tp_name, "%s.%s", domainName, typeName);
	asprintf((char **) &newType->tp_doc, "%s class for fastset domain %s", typeName, domainName);

	if (PyType_Ready(newType) < 0) {
		fprintf(stderr, "Unable to initialize new type %s", newType->tp_name);
		abort();
	}

	return newType;
}

PyTypeObject	fastset_DomainType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "domain",
	.tp_basicsize	= sizeof(fastset_Domain),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= NULL,

//	.tp_methods	= fastset_memberMethods,
	.tp_init	= (initproc) Fastset_initDomain,
	.tp_new		= Fastset_newDomain,
	.tp_dealloc	= (destructor) Fastset_deallocDomain,
	.tp_members	= domain_TypeMembers,
	.tp_getset	= domain_Getters,
};


PyTypeObject *
fastset_CreateMemberClass(fastset_Domain *domain)
{
	return fastset_DSTAlloc(domain, &fastset_MemberTypeTemplate, "member");
}

static const fastset_DomainSpecificType *
Fastset_DSTGetType(PyObject *obj)
{
	PyTypeObject *type;

	for (type = obj->ob_type; type; type = type->tp_base) {
		fastset_DomainSpecificType *dst = (fastset_DomainSpecificType *) type;

		if (dst->magic == FASTSET_DST_MAGIC)
			return dst;
	}

	return NULL;
}

fastset_Domain *
Fastset_DSTGetDomain(PyObject *obj)
{
	const fastset_DomainSpecificType *dst;

	if ((dst = Fastset_DSTGetType(obj)) != NULL) {
		// printf("%s: object belongs to domain \"%s\"\n", __func__, dst->domain->name);
		return dst->domain;
	}

	// printf("%s: object type is %s\n", __func__, obj->ob_type->tp_name);
	PyErr_SetString(PyExc_RuntimeError, "unable to locate fastset domain for this object");
	return NULL;
}

PyTypeObject *
fastset_CreateSetClass(fastset_Domain *domain)
{
	return fastset_DSTAlloc(domain, &fastset_SetTypeTemplate, "set");
}

static PyObject *
Fastset_newDomain(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	fastset_Domain *self;

	self = (fastset_Domain *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->member_class = NULL;
	self->set_class = NULL;

	self->size = 0;
	self->domain_objects = NULL;

	return (PyObject *) self;
}

static int
Fastset_initDomain(fastset_Domain *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"name",
		NULL
	};
	char *domain_name = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &domain_name))
		return -1;

	self->name = strdup(domain_name);
	self->member_class = fastset_DSTAlloc(self, &fastset_MemberTypeTemplate, "member");
	self->set_class = fastset_DSTAlloc(self, &fastset_SetTypeTemplate, "set");

	return 0;
}

static void
Fastset_deallocDomain(fastset_Domain *self)
{
	if (self->name) {
		free(self->name);
		self->name = NULL;
	}

	Py_CLEAR(self->member_class);
	Py_CLEAR(self->set_class);

	/* Can this really happen? */
	if (self->domain_objects) {
		unsigned int i;

		for (i = 0; i < self->size; ++i) {
			Py_CLEAR(self->domain_objects[i]);
		}
		free(self->domain_objects);
		self->domain_objects = NULL;
		self->count = 0;
		self->size = 0;
	}
}

void
FastsetDomain_register(fastset_Domain *self, fastset_Member *member)
{
	int slot = -1;

	if (self->count < self->size) {
		unsigned int i;

		for (i = 0; i < self->size; ++i) {
			if (self->domain_objects[i] == NULL) {
				slot = i;
				break;
			}
		}
	}

	if (slot < 0) {
		static const unsigned int chunk_size = 16;

		/* allocate array space in chunks */
		if ((self->size % chunk_size) == 0) {
			unsigned int nalloc = self->size + chunk_size;
			PyObject **new_array;

			new_array = realloc(self->domain_objects, nalloc * sizeof(new_array[0]));
			if (new_array == NULL)
				abort();
			self->domain_objects = new_array;
		}

		slot = self->size++;
	}

	self->domain_objects[slot] = (PyObject *) member;
	Py_INCREF(member);

	self->count += 1;

	member->index = slot;
}

void
FastsetDomain_unregister(fastset_Domain *self, fastset_Member *member)
{
	if (member->index < 0)
		return;

	assert(self->count);
	assert(member->index < self->size);
	assert(self->domain_objects[member->index] == (PyObject *) member);

	self->domain_objects[member->index] = NULL;
	member->index = -1;

	self->count -= 1;
}

int
FastsetDomain_Check(PyObject *ob)
{
	return PyType_IsSubtype(Py_TYPE(ob), &fastset_DomainType);
}

bool
FastsetDomain_IsMember(fastset_Domain *self, PyObject *object)
{
	const fastset_DomainSpecificType *dst;

	return ((dst = Fastset_DSTGetType(object)) != NULL && &dst->base == self->member_class);
}

bool
FastsetDomain_IsSet(fastset_Domain *self, PyObject *object)
{
	const fastset_DomainSpecificType *dst;

	return ((dst = Fastset_DSTGetType(object)) != NULL && &dst->base == self->set_class);
}

PyObject *
FastsetDomain_GetMember(fastset_Domain *self, unsigned int index)
{
	/* printf("%s %s count=%u\n", __func__, self->name, self->count); */
	if (index >= self->size)
		return NULL;

	return self->domain_objects[index];
}

PyObject *
Fastset_getDomainName(fastset_Domain *self, void *closure)
{
	return PyUnicode_FromString(self->name);
}
