/*
 * fastsets - implement sets of things as subsets of a domain.
 *
 * This allows us to represent a set as a bit vector, and set operations
 * become bit operations.
 */

#ifndef FASTSETS_H
#define FASTSETS_H

#include <stdbool.h>
#include <Python.h>


typedef uint64_t	fastset_bitvec_word_t;

typedef struct fastset_bitvec {
	unsigned int	refcount;

	unsigned int	max_index;
	unsigned int	nwords;

	unsigned int	nalloc;
	fastset_bitvec_word_t *words;
} fastset_bitvec_t;

typedef struct fastset_bitvec_transform {
	unsigned int	max_index;
	int *		mapping;
} fastset_bitvec_transform_t;

extern PyTypeObject	fastset_DomainType;
extern PyTypeObject	fastset_SetIteratorType;
extern PyTypeObject	fastset_SetTypeTemplate;
extern PyTypeObject	fastset_MemberTypeTemplate;
extern PyTypeObject	fastset_TransformType;

typedef struct {
	PyObject_HEAD

	char *		name;

	PyTypeObject *	member_class;
	PyTypeObject *	set_class;

	unsigned int	size;
	unsigned int	count;
	PyObject **	domain_objects;
} fastset_Domain;

typedef struct {
	PyObject_HEAD

	fastset_Domain *domain;
	int		index;
} fastset_Member;

typedef struct {
	PyObject_HEAD

	fastset_Domain *domain;
	fastset_bitvec_t *bitvec;
} fastset_Set;

typedef struct {
	PyObject_HEAD

	fastset_Domain *domain;
	fastset_bitvec_t *vector;
	unsigned int	index;
} fastset_SetIterator;

typedef struct {
	PyTypeObject	base;

	uint64_t	magic;
	fastset_Domain *domain;
} fastset_DomainSpecificType;

typedef struct {
	PyObject_HEAD

	char *		name;

	fastset_Domain *domain;
	fastset_bitvec_transform_t *bittrans;
} fastset_Transform;

#define FASTSET_DST_MAGIC	0xfaded0ddbeefcafe

extern fastset_bitvec_t *fastset_bitvec_new(unsigned int size);
extern fastset_bitvec_t *fastset_bitvec_unshare(fastset_bitvec_t *);
extern fastset_bitvec_t *fastset_bitvec_hold(fastset_bitvec_t *);
extern fastset_bitvec_t *fastset_bitvec_copy(const fastset_bitvec_t *);
extern void		fastset_bitvec_resize(fastset_bitvec_t *, unsigned int max_index);
extern void		fastset_bitvec_release(fastset_bitvec_t *);
extern bool		fastset_bitvec_set(fastset_bitvec_t *, unsigned int i);
extern bool		fastset_bitvec_clear(fastset_bitvec_t *, unsigned int i);
extern void		fastset_bitvec_update_union(fastset_bitvec_t *, const fastset_bitvec_t *);
extern void		fastset_bitvec_update_intersection(fastset_bitvec_t *, const fastset_bitvec_t *);
extern void		fastset_bitvec_update_difference(fastset_bitvec_t *, const fastset_bitvec_t *);
extern void		fastset_bitvec_update_symmetric_difference(fastset_bitvec_t *, const fastset_bitvec_t *);
extern int		fastset_bitvec_compare(const fastset_bitvec_t *, const fastset_bitvec_t *);
extern int		fastset_bitvec_find_next_bit(const fastset_bitvec_t *, unsigned int);
extern unsigned int	fastset_bitvec_count_ones(const fastset_bitvec_t *);

extern bool		fastset_bitvec_test_subset(const fastset_bitvec_t *subset, const fastset_bitvec_t *superset);
extern bool		fastset_bitvec_test_disjoint(const fastset_bitvec_t *subset, const fastset_bitvec_t *superset);
extern bool		fastset_bitvec_test_empty(const fastset_bitvec_t *);
extern bool		fastset_bitvec_test_bit(const fastset_bitvec_t *, unsigned int);

extern fastset_bitvec_t *fastset_bitvec_union(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2);
extern fastset_bitvec_t *fastset_bitvec_intersection(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2);
extern fastset_bitvec_t *fastset_bitvec_difference(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2);
extern fastset_bitvec_t *fastset_bitvec_symmetric_difference(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2);
extern fastset_bitvec_t *fastset_bitvec_transform(const fastset_bitvec_t *arg, const fastset_bitvec_transform_t *);

extern fastset_bitvec_transform_t *fastset_bitvec_transform_new(unsigned int);
extern void		fastset_bitvec_transform_add(fastset_bitvec_transform_t *, unsigned int arg_index, int res_index);
extern void		fastset_bitvec_transform_free(fastset_bitvec_transform_t *);

enum {
	FASTSET_REL_EQUAL		= 0,
	FASTSET_REL_GREATER_THAN	= 1,
	FASTSET_REL_LESS_THAN		= 2,
	FASTSET_REL_NOT_EQUAL		= 3,
};

static inline void
fastset_bitvec_drop(fastset_bitvec_t **var)
{
	fastset_bitvec_t *vec;

	if ((vec = *var) != NULL) {
		fastset_bitvec_release(vec);
		*var = NULL;
	}
}

extern PyObject *	fastset_callType(PyTypeObject *typeObject, PyObject *args, PyObject *kwds);

extern int		FastsetDomain_Check(PyObject *self);
extern bool		FastsetDomain_IsMember(fastset_Domain *self, PyObject *object);
extern bool		FastsetDomain_IsSet(fastset_Domain *self, PyObject *object);
extern void		FastsetDomain_register(fastset_Domain *self, fastset_Member *member);
extern void		FastsetDomain_unregister(fastset_Domain *self, fastset_Member *member);
extern PyObject *	FastsetDomain_GetMember(fastset_Domain *self, unsigned int index);

extern PyObject *	FastsetSet_TransformBitvec(fastset_Set *self, const fastset_bitvec_transform_t *);

extern fastset_Domain *	Fastset_DSTGetDomain(PyObject *obj);

#endif /* FASTSETS_H */
