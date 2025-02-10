/*
 * fastsets - set objects
 * 
 * Copyright (C) 2023 SUSE
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdbool.h>
#include "fastsets.h"

static PyObject *	Fastset_newSet(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		Fastset_initSet(fastset_Set *self, PyObject *args, PyObject *kwds);
static void		Fastset_deallocSet(fastset_Set *self);

static PyObject *	Fastset_getiter(fastset_Set *self);
static PyObject *	Fastset_add(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_remove(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_discard(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_pop(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_copy(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_union(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_intersection(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_difference(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_symmetric_difference(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_update(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_intersection_update(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_difference_update(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_symmetric_difference_update(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_issubset(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_issuperset(fastset_Set *self, PyObject *args, PyObject *kwds);
static PyObject *	Fastset_isdisjoint(fastset_Set *self, PyObject *args, PyObject *kwds);
static Py_ssize_t	Fastset_length(fastset_Set *);
static PyObject *	Fastset_str(fastset_Set *);
static PyObject *	Fastset_richcompare(fastset_Set *self, PyObject *other, int op);
static int		Fastset_contains(fastset_Set *self, PyObject *member);
static int		Fastset_nonempty(fastset_Set *);

static PyMethodDef fastset_setMethods[] = {
      { "copy", (PyCFunction) Fastset_copy, METH_VARARGS | METH_KEYWORDS,
        "create a copy of a set"
      },
      { "add", (PyCFunction) Fastset_add, METH_VARARGS | METH_KEYWORDS,
        "add an object to the set"
      },
      { "remove", (PyCFunction) Fastset_remove, METH_VARARGS | METH_KEYWORDS,
        "remove an object from the set"
      },
      { "discard", (PyCFunction) Fastset_discard, METH_VARARGS | METH_KEYWORDS,
        "discard an object from the set"
      },
      { "pop", (PyCFunction) Fastset_pop, METH_VARARGS | METH_KEYWORDS,
        "pop an object from the set"
      },
      { "union", (PyCFunction) Fastset_union, METH_VARARGS | METH_KEYWORDS,
        "compute the union of this set with another set"
      },
      { "intersection", (PyCFunction) Fastset_intersection, METH_VARARGS | METH_KEYWORDS,
        "compute the intersection of this set with another set"
      },
      { "difference", (PyCFunction) Fastset_difference, METH_VARARGS | METH_KEYWORDS,
        "compute the difference of this set with another set"
      },
      { "symmetric_difference", (PyCFunction) Fastset_symmetric_difference, METH_VARARGS | METH_KEYWORDS,
        "compute the symmetric difference of this set with another set"
      },
      { "update", (PyCFunction) Fastset_update, METH_VARARGS | METH_KEYWORDS,
        "update the set with the union of this set with another set"
      },
      { "intersection_update", (PyCFunction) Fastset_intersection_update, METH_VARARGS | METH_KEYWORDS,
        "update the set with the intersection of this set with another set"
      },
      { "difference_update", (PyCFunction) Fastset_difference_update, METH_VARARGS | METH_KEYWORDS,
        "update the set with the difference of this set with another set"
      },
      { "symmetric_difference_update", (PyCFunction) Fastset_symmetric_difference_update, METH_VARARGS | METH_KEYWORDS,
        "update the set with the symmetric difference of this set with another set"
      },
      { "issubset", (PyCFunction) Fastset_issubset, METH_VARARGS | METH_KEYWORDS,
        "test whether the set is a subset of another set"
      },
      { "issuperset", (PyCFunction) Fastset_issuperset, METH_VARARGS | METH_KEYWORDS,
        "test whether the set is a superset of another set"
      },
      { "isdisjoint", (PyCFunction) Fastset_isdisjoint, METH_VARARGS | METH_KEYWORDS,
        "test whether the set is disjoint wrt another set"
      },
      { NULL, }
};

static PySequenceMethods fastset_sequenceMethods = {
	.sq_length	= (lenfunc) Fastset_length,
	.sq_contains	= (objobjproc) Fastset_contains,
};

static PyNumberMethods fastset_numberMethods = {
	.nb_bool	= (inquiry) Fastset_nonempty,
};

PyTypeObject	fastset_SetTypeTemplate = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= NULL,
	.tp_basicsize	= sizeof(fastset_Set),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= NULL,

	.tp_methods	= fastset_setMethods,
	.tp_init	= (initproc) Fastset_initSet,
	.tp_new		= Fastset_newSet,
	.tp_dealloc	= (destructor) Fastset_deallocSet,
	.tp_iter	= (getiterfunc) Fastset_getiter,
	.tp_as_sequence	= &fastset_sequenceMethods,
	.tp_as_number	= &fastset_numberMethods,
	.tp_richcompare = (richcmpfunc) Fastset_richcompare,
	.tp_str		= (reprfunc) Fastset_str,
};

PyObject *
Fastset_newSet(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	fastset_Set *self;

	self = (fastset_Set *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->domain = NULL;
	self->bitvec = fastset_bitvec_new(0);

	return (PyObject *) self;
}

int
Fastset_initSet(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"values",
		NULL
	};
	fastset_Domain *domain;
	PyObject *values = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &values))
		return -1;

	if (!(domain = Fastset_DSTGetDomain((PyObject *) self)))
		return -1;

	Py_INCREF(domain);
	self->domain = domain;

	if (values != NULL && values != Py_None) {
		PyObject *iter, *member_object = NULL;

		if (!(iter = PyObject_GetIter(values)))
			return -1;

		while ((member_object = PyIter_Next(iter)) != NULL) {
			fastset_Member *member;

			if (!FastsetDomain_IsMember(self->domain, member_object)) {
				PyErr_SetString(PyExc_RuntimeError, "argument is not compatible with domain");
				break;
			}

			member = (fastset_Member *) member_object;

			if (member->index < 0) {
				PyErr_SetString(PyExc_RuntimeError, "fastset member has invalid index");
				break;
			}

			fastset_bitvec_set(self->bitvec, member->index);
			Py_CLEAR(member_object);
		}

		Py_CLEAR(member_object);
		Py_CLEAR(iter);
		if (PyErr_Occurred())
			return -1;
	}

	return 0;
}

void
Fastset_deallocSet(fastset_Set *self)
{
	if (self->domain)
		Py_CLEAR(self->domain);

	fastset_bitvec_drop(&self->bitvec);
}

Py_ssize_t
Fastset_length(fastset_Set *self)
{
	if (self->bitvec == NULL)
		return 0;
	return fastset_bitvec_count_ones(self->bitvec);
}

int
Fastset_contains(fastset_Set *self, PyObject *member)
{
	if (!FastsetDomain_IsMember(self->domain, member))
		return 0;

	return fastset_bitvec_test_bit(self->bitvec, ((fastset_Member *) member)->index);
}

int
Fastset_nonempty(fastset_Set *self)
{
	return !fastset_bitvec_test_empty(self->bitvec);
}

PyObject *
Fastset_getiter(fastset_Set *self)
{
	PyObject *tuple, *result;

	tuple = PyTuple_New(1);

	PyTuple_SetItem(tuple, 0, (PyObject *) self);
	Py_INCREF(self);

	result = fastset_callType(&fastset_SetIteratorType, tuple, NULL);

	Py_DECREF(tuple);
	return result;
}

static bool
Fastset_getDomainAndBitvector(PyObject *obj, fastset_Domain **domain_p, fastset_bitvec_t **vec_p)
{
	PyTypeObject *type;

	for (type = obj->ob_type; type; type = type->tp_base) {
		if (type->tp_init == (initproc) Fastset_initSet) {
			fastset_Set *set = (fastset_Set *) obj;

			*domain_p = set->domain;
			*vec_p = set->bitvec;
			return true;
		}
	}

	return false;
}

static bool
Fastset_argsVoid(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { NULL };

	return !!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist);
}

static fastset_Member *
Fastset_argsToMember(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"member",
		NULL
	};
	PyObject *member_object = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &member_object))
		return NULL;

	if (!FastsetDomain_IsMember(self->domain, member_object)) {
		PyErr_SetString(PyExc_RuntimeError, "argument is not compatible with domain");
		return NULL;
	}

	return (fastset_Member *) member_object;
}

static fastset_Set *
Fastset_castToSet(fastset_Set *self, PyObject *other_object)
{
	if (self->ob_base.ob_type != other_object->ob_type) {
		PyErr_SetString(PyExc_RuntimeError, "argument is not compatible with set domain");
		return NULL;
	}

	return (fastset_Set *) other_object;
}

static fastset_Set *
Fastset_argsToSet(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"other",
		NULL
	};
	PyObject *other_object = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &other_object))
		return NULL;

	return Fastset_castToSet(self, other_object);
}

static PyObject *
boolObject(bool rv)
{
	PyObject *res = rv? Py_True : Py_False;

	Py_INCREF(res);
	return res;
}

PyObject *
Fastset_add(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Member *member;

	if (!(member = Fastset_argsToMember(self, args, kwds)))
		return NULL;

	if (member->index < 0) {
		PyErr_SetString(PyExc_RuntimeError, "fastset member has invalid index");
		return NULL;
	}

	return boolObject(fastset_bitvec_set(self->bitvec, member->index));
}

PyObject *
Fastset_remove(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Member *member;

	if (!(member = Fastset_argsToMember(self, args, kwds)))
		return NULL;

	if (member->index < 0 || !fastset_bitvec_clear(self->bitvec, member->index)) {
		_PyErr_SetKeyError((PyObject *) member);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *
Fastset_discard(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Member *member;
	PyObject *ret;

	if (!(member = Fastset_argsToMember(self, args, kwds)))
		return NULL;

	ret = Py_True;
	if (member->index < 0 || !fastset_bitvec_clear(self->bitvec, member->index))
		ret = Py_False;

	Py_INCREF(ret);
	return ret;
}

PyObject *
Fastset_pop(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	PyObject *member;
	int next_bit = 0;

	if (!Fastset_argsVoid(self, args, kwds))
		return NULL;

	do {
		next_bit = fastset_bitvec_find_next_bit(self->bitvec, next_bit);
		if (next_bit < 0) {
			Py_INCREF(Py_None);
			return Py_None;
		}

		member = FastsetDomain_GetMember(self->domain, next_bit);
		fastset_bitvec_clear(self->bitvec, next_bit);
	} while (member == NULL);

	Py_INCREF(member);
	return member;
}

static PyObject *
Fastset_buildResult(PyTypeObject *set_type, fastset_bitvec_t *vec)
{
	PyObject *result;

	result = fastset_callType(set_type, NULL, NULL);
	if (result != NULL) {
		((fastset_Set *) result)->bitvec = vec;
	} else {
		fastset_bitvec_release(vec);
	}

	return result;
}

PyObject *
Fastset_copy(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_bitvec_t *vec = NULL;

	if (!Fastset_argsVoid(self, args, kwds))
		return NULL;

	vec = fastset_bitvec_copy(self->bitvec);

	return Fastset_buildResult(self->ob_base.ob_type, vec);
}

PyObject *
Fastset_union(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_bitvec_t *vec = NULL;
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	vec = fastset_bitvec_union(self->bitvec, other->bitvec);
	return Fastset_buildResult(self->ob_base.ob_type, vec);
}

static PyObject *
Fastset_intersection(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_bitvec_t *vec = NULL;
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	vec = fastset_bitvec_intersection(self->bitvec, other->bitvec);
	return Fastset_buildResult(self->ob_base.ob_type, vec);
}

static PyObject *
Fastset_difference(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_bitvec_t *vec = NULL;
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	vec = fastset_bitvec_difference(self->bitvec, other->bitvec);
	return Fastset_buildResult(self->ob_base.ob_type, vec);
}

static PyObject *
Fastset_symmetric_difference(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_bitvec_t *vec = NULL;
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	vec = fastset_bitvec_symmetric_difference(self->bitvec, other->bitvec);
	return Fastset_buildResult(self->ob_base.ob_type, vec);
}

PyObject *
Fastset_update(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	fastset_bitvec_update_union(self->bitvec, other->bitvec);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *
Fastset_intersection_update(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	fastset_bitvec_update_intersection(self->bitvec, other->bitvec);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *
Fastset_difference_update(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	fastset_bitvec_update_difference(self->bitvec, other->bitvec);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *
Fastset_symmetric_difference_update(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	fastset_bitvec_update_symmetric_difference(self->bitvec, other->bitvec);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *
Fastset_issubset(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	return boolObject(fastset_bitvec_test_subset(self->bitvec, other->bitvec));
}

PyObject *
Fastset_issuperset(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	return boolObject(fastset_bitvec_test_subset(other->bitvec, self->bitvec));
}

PyObject *
Fastset_isdisjoint(fastset_Set *self, PyObject *args, PyObject *kwds)
{
	fastset_Set *other;

	if (!(other = Fastset_argsToSet(self, args, kwds)))
		return NULL;

	return boolObject(fastset_bitvec_test_disjoint(self->bitvec, other->bitvec));
}

PyObject *
Fastset_richcompare(fastset_Set *self, PyObject *other_object, int op)
{
	fastset_Set *other;
	int relation;
	bool rv;

	if (!(other = Fastset_castToSet(self, other_object)))
		return NULL;

	relation = fastset_bitvec_compare(self->bitvec, other->bitvec);
	switch (op) {
	case Py_LT:
		// printf("%s(Py_LT): relation %d\n", __func__, relation);
		rv = relation == FASTSET_REL_LESS_THAN;
		break;
	case Py_LE:
		// printf("%s(Py_LE): relation %d\n", __func__, relation);
		rv = relation == FASTSET_REL_LESS_THAN || relation == FASTSET_REL_EQUAL;
		break;
	case Py_GT:
		// printf("%s(Py_GT): relation %d\n", __func__, relation);
		rv = relation == FASTSET_REL_GREATER_THAN;
		break;
	case Py_GE:
		// printf("%s(Py_GE): relation %d\n", __func__, relation);
		rv = relation == FASTSET_REL_GREATER_THAN || relation == FASTSET_REL_EQUAL;
		break;
	case Py_EQ:
		// printf("%s(Py_EQ): relation %d\n", __func__, relation);
		rv = relation == FASTSET_REL_EQUAL;
		break;
	case Py_NE:
		// printf("%s(Py_NE): relation %d\n", __func__, relation);
		rv = relation != FASTSET_REL_EQUAL;
		break;
	default:
		/* FIXME set exception? */
		return NULL;
	}

	return boolObject(rv);
}

/*
 * Represent set as string
 */
PyObject *
Fastset_str(fastset_Set *self)
{
	const fastset_bitvec_t *bv = self->bitvec;
	unsigned int index = 0;
	PyObject *seq = NULL, *sepa = NULL, *result = NULL;

	seq = PyList_New(0);
	while (true) {
		int next_bit;
		PyObject *member, *item_str;

		next_bit = fastset_bitvec_find_next_bit(bv, index);
		if (next_bit < 0)
			break;

		member = FastsetDomain_GetMember(self->domain, next_bit);

		item_str = PyObject_Str(member);
		PyList_Append(seq, item_str);
		Py_CLEAR(item_str);

		index = next_bit + 1;
	}

	sepa = PyUnicode_FromString(" ");
	result = PyUnicode_Join(sepa, seq);

	Py_CLEAR(sepa);
	Py_CLEAR(seq);

	return result;
}

/*
 * fast transforms of sets
 */
PyObject *
FastsetSet_TransformBitvec(fastset_Set *self, const fastset_bitvec_transform_t *trans)
{
	fastset_bitvec_t *res;

	res = fastset_bitvec_transform(self->bitvec, trans);
	if (res == NULL) {
		PyErr_SetString(PyExc_ValueError, "transformation failed");
		return NULL;
	}

	return Fastset_buildResult(self->ob_base.ob_type, res);
}


/*
 * Iteration
 */
static PyObject *	FastsetIterator_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		FastsetIterator_init(fastset_SetIterator *self, PyObject *args, PyObject *kwds);
static void		FastsetIterator_dealloc(fastset_SetIterator *self);
static PyObject *	FastsetIterator_iternext(fastset_SetIterator *self);

PyTypeObject fastset_SetIteratorType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "fastset.setiter",
	.tp_basicsize	= sizeof(fastset_SetIterator),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Object representing an iterator over a fastset",

	.tp_init	= (initproc) FastsetIterator_init,
	.tp_new		= FastsetIterator_new,
	.tp_dealloc	= (destructor) FastsetIterator_dealloc,
	.tp_iter	= (getiterfunc) PyObject_SelfIter,
	.tp_iternext	= (iternextfunc) FastsetIterator_iternext,

	/* need to add sequence methods for __len__ */
};

/*
 * Iterator implementation
 */
static PyObject *
FastsetIterator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	fastset_SetIterator *self;

	self = (fastset_SetIterator *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->domain = NULL;
	self->vector = NULL;
	self->index = 0;

	return (PyObject *)self;
}

/*
 * Initialize the iterator object
 */
static int
FastsetIterator_init(fastset_SetIterator *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"set",
		NULL
	};
	PyObject *set_object = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &set_object))
		return -1;

	if (set_object) {
		fastset_Domain *domain;
		fastset_bitvec_t *vec;

		if (!Fastset_getDomainAndBitvector(set_object, &domain, &vec)) {
			PyErr_SetString(PyExc_RuntimeError, "cannot create fastset iterator for this object");
			return -1;
		}

		self->domain = domain;
		Py_INCREF(self->domain);

		self->vector = vec;
		fastset_bitvec_hold(vec);
	}

	return 0;
}

static void
FastsetIterator_dealloc(fastset_SetIterator *self)
{
	Py_CLEAR(self->domain);

	if (self->vector) {
		fastset_bitvec_release(self->vector);
		self->vector = NULL;
	}
}

PyObject *
FastsetIterator_iter(fastset_SetIterator *self)
{
	Py_INCREF(self);
	return (PyObject *) self;
}

PyObject *
FastsetIterator_iternext(fastset_SetIterator *self)
{
	PyObject *member = NULL;
	int next_bit;

	/* We may have to do this loop serveral times in case a member has been removed
	 * from the domain */
	do {
		next_bit = fastset_bitvec_find_next_bit(self->vector, self->index);
		if (next_bit < 0) {
			/* raise StopIteration exception? */
			return NULL;
		}

		member = FastsetDomain_GetMember(self->domain, next_bit);

		self->index = next_bit + 1;
	} while (member == NULL);

	Py_INCREF(member);
	return member;
}
