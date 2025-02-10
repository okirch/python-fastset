/*
fastsets - bitvec implementation

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

static const unsigned int	FASTVEC_WORD_SIZE = 8 * sizeof(((fastset_bitvec_t *) 0)->words[0]);

static const unsigned int	bits_per_octet[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

static inline unsigned int
fastset_bitvec_bits_to_size(unsigned int size)
{
	return (size / FASTVEC_WORD_SIZE) + 1;
	return ((size + FASTVEC_WORD_SIZE - 1) / FASTVEC_WORD_SIZE);
}

static inline unsigned int
fastset_bitvec_bit_index_to_word_index(unsigned int size)
{
	return size / FASTVEC_WORD_SIZE;
}

static inline unsigned int
MAX(unsigned int a, unsigned int b)
{
	return (a > b)? a : b;
}

static inline unsigned int
MIN(unsigned int a, unsigned int b)
{
	return (a < b)? a : b;
}

static inline void
fastset_bitvec_swap(fastset_bitvec_t **p1, fastset_bitvec_t **p2)
{
	fastset_bitvec_t *tmp = *p1;

	*p1 = *p2;
	*p2 = tmp;
}

static inline void
fastset_bitvec_bit_to_index_unchecked(const fastset_bitvec_t *vec, unsigned int i, unsigned int *word_index, fastset_bitvec_word_t *mask)
{
	*word_index = i / FASTVEC_WORD_SIZE;
	*mask = (1ULL << (i % FASTVEC_WORD_SIZE));
}

static inline bool
fastset_bitvec_bit_to_index(const fastset_bitvec_t *vec, unsigned int i, unsigned int *word_index, fastset_bitvec_word_t *mask)
{
	if (i >= vec->max_index)
		return false;

	*word_index = i / FASTVEC_WORD_SIZE;
	*mask = (1ULL << (i % FASTVEC_WORD_SIZE));
	return true;
}

fastset_bitvec_t *
fastset_bitvec_new(unsigned int initial_size)
{
	fastset_bitvec_t *vec;

	vec = calloc(1, sizeof(*vec));
	if (initial_size)
		fastset_bitvec_resize(vec, initial_size);
	vec->refcount = 1;
	return vec;
}

static void
fastset_bitvec_free(fastset_bitvec_t *vec)
{
	fastset_bitvec_resize(vec, 0);
	free(vec);
}

fastset_bitvec_t *
fastset_bitvec_hold(fastset_bitvec_t *vec)
{
	if (vec != NULL) {
		assert(vec->refcount);
		vec->refcount ++;
	}

	return vec;
}

void
fastset_bitvec_release(fastset_bitvec_t *vec)
{
	assert(vec->refcount);
	if (--(vec->refcount) == 0)
		fastset_bitvec_free(vec);
}

void
fastset_bitvec_resize(fastset_bitvec_t *vec, unsigned int max_index)
{
	unsigned int old_word_index, new_word_index;
	fastset_bitvec_word_t old_mask, new_mask;

	if (max_index == vec->max_index)
		return;

	if (max_index == 0) {
		if (vec->words) {
			free(vec->words);
			vec->words = NULL;
		}
		vec->nalloc = vec->nwords = vec->max_index = 0;
		return;
	} else
	if (max_index <= vec->max_index) {
		/* we never bother shrinking the vec memory for now,
		 * but we need to clear bits within the word that the
		 * new max_index belongs to. We skip clearing any
		 * subsequent words - but we need to do that whenever
		 * someone decides to grow the vector again.
		 */
	} else {
		unsigned int new_nwords = fastset_bitvec_bits_to_size(max_index);

		if (new_nwords > vec->nalloc) {
			vec->words = realloc(vec->words, new_nwords * sizeof(vec->words[0]));
			vec->nalloc = new_nwords;
		}
		while (vec->nwords < new_nwords)
			vec->words[vec->nwords++] = 0;
	}

	fastset_bitvec_bit_to_index_unchecked(vec, vec->max_index, &old_word_index, &old_mask);
	fastset_bitvec_bit_to_index_unchecked(vec, max_index, &new_word_index, &new_mask);
	assert(new_word_index < vec->nwords);

	vec->words[old_word_index++] &= old_mask - 1;
	while (old_word_index < new_word_index)
		vec->words[old_word_index++] = 0;

	/* Clear any trailing bits in the highest word - needed when shrinking a bitvec */
	vec->words[new_word_index] &= new_mask - 1;

	vec->max_index = max_index;
	vec->nwords = new_word_index + 1;

	assert(fastset_bitvec_bit_index_to_word_index(vec->max_index) < vec->nwords);
}

bool
fastset_bitvec_set(fastset_bitvec_t *vec, unsigned int i)
{
	unsigned int word_index;
	fastset_bitvec_word_t mask;
	bool rv;

	if (i >= vec->max_index)
		fastset_bitvec_resize(vec, i + 1);

	fastset_bitvec_bit_to_index_unchecked(vec, i, &word_index, &mask);
	rv = !!(vec->words[word_index] & mask);
//	printf("%s: index %u -> word %u mask 0x%Lx\n", __func__, i, word_index, (unsigned long long) mask);
	vec->words[word_index] |= mask;
	return rv;
}

bool
fastset_bitvec_clear(fastset_bitvec_t *vec, unsigned int i)
{
	unsigned int word_index;
	fastset_bitvec_word_t mask;
	bool rv;

	if (i >= vec->max_index)
		return false;

	fastset_bitvec_bit_to_index(vec, i, &word_index, &mask);
	rv = !!(vec->words[word_index] & mask);
	vec->words[word_index] &= ~mask;
	return rv;
}

bool
fastset_bitvec_test(const fastset_bitvec_t *vec, unsigned int i)
{
	unsigned int word_index;
	fastset_bitvec_word_t mask;

	if (i >= vec->max_index)
		return false;

	fastset_bitvec_bit_to_index(vec, i, &word_index, &mask);
	return !!(vec->words[word_index] & mask);
}

static inline int
__find_next_bit_in_word(unsigned int word_index, fastset_bitvec_word_t word)
{
	unsigned int bit_index;

//	printf("%s: word_index=%u word=0x%llx\n", __func__, word_index, (unsigned long long) word);
	if (!word)
		return -1;

	bit_index = word_index * FASTVEC_WORD_SIZE;
	while (!(word & 1)) {
		bit_index += 1;
		word >>= 1;
	}
	return bit_index;
}

int
fastset_bitvec_find_next_bit(const fastset_bitvec_t *vec, unsigned int from_index)
{
	unsigned int word_index;
	fastset_bitvec_word_t mask, word;

	if (from_index >= vec->max_index)
		return -1;

	fastset_bitvec_bit_to_index(vec, from_index, &word_index, &mask);

	mask = ~(mask - 1);
	word = vec->words[word_index] & mask;
	if (word != 0)
		return __find_next_bit_in_word(word_index, word);

	while (++word_index < vec->nwords) {
		word = vec->words[word_index];
		if (word != 0)
			return __find_next_bit_in_word(word_index, word);
	}

	return -1;
}

unsigned int
fastset_bitvec_count_ones(const fastset_bitvec_t *vec)
{
	unsigned int result = 0;
	unsigned int word_index;
	fastset_bitvec_word_t mask, word;

	if (vec->max_index == 0)
		return 0;

	if (!fastset_bitvec_bit_to_index(vec, vec->max_index - 1, &word_index, &mask))
		return 0;

	mask = (mask - 1) | mask;

//	printf("word %u = 0x%Lx mask 0x%Lx\n", word_index, (unsigned long long) vec->words[word_index],  (unsigned long long) mask);
	word = vec->words[word_index] & mask;
	while (1) {
		while (word) {
			result += bits_per_octet[word & 0xFF];
			result += bits_per_octet[(word >> 8) & 0xFF];
			result += bits_per_octet[(word >> 16) & 0xFF];
			result += bits_per_octet[(word >> 24) & 0xFF];
			word >>= 32;
		}

		if (word_index == 0)
			break;
		word = vec->words[--word_index];
	}

	return result;
}

static inline void
__fastset_bitvec_union(fastset_bitvec_t *res, const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2, unsigned int nwords)
{
	unsigned int n;

	assert(nwords <= res->nwords);
	for (n = 0; n < nwords; ++n)
		res->words[n] = arg1->words[n] | arg2->words[n];
}

static inline void
__fastset_bitvec_copy(fastset_bitvec_t *res, const fastset_bitvec_t *arg, unsigned int from_word, unsigned int upto_word)
{
	unsigned int n;

	assert(upto_word <= arg->nwords);
	assert(upto_word <= res->nwords);
	for (n = from_word; n < upto_word; ++n)
		res->words[n] = arg->words[n];
}

fastset_bitvec_t *
fastset_bitvec_copy(const fastset_bitvec_t *arg)
{
	fastset_bitvec_t *res;

	res = fastset_bitvec_new(arg->max_index);
	__fastset_bitvec_copy(res, arg, 0, res->nwords);
	return res;
}

void
fastset_bitvec_update_union(fastset_bitvec_t *res, const fastset_bitvec_t *arg)
{
	unsigned int max_index;

	max_index = MAX(res->max_index, arg->max_index);
	fastset_bitvec_resize(res, max_index);

	__fastset_bitvec_union(res, res, arg, arg->nwords);
}

fastset_bitvec_t *
fastset_bitvec_union(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2)
{
	unsigned int max_index;
	fastset_bitvec_t *res;

	max_index = MAX(arg1->max_index, arg2->max_index);
	res = fastset_bitvec_new(max_index);

	if (arg1->nwords < arg2->nwords) {
		__fastset_bitvec_union(res, arg1, arg2, arg1->nwords);
		__fastset_bitvec_copy(res, arg2, arg1->nwords, arg2->nwords);
	} else {
		__fastset_bitvec_union(res, arg1, arg2, arg2->nwords);
		__fastset_bitvec_copy(res, arg1, arg2->nwords, arg1->nwords);
	}

	return res;
}

static inline void
__fastset_bitvec_intersection(fastset_bitvec_t *res, const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2)
{
	unsigned int n, nbits;

	nbits = MIN(arg1->max_index, arg2->max_index);
//	printf("%s: resize from %u, %u -> %u\n", __func__, arg1->max_index, arg2->max_index, nbits);
	fastset_bitvec_resize(res, nbits);
//	printf("  max_index=%u nwords=%u\n", res->max_index, res->nwords);

	for (n = 0; n < res->nwords; ++n)
		res->words[n] = arg1->words[n] & arg2->words[n];
}

void
fastset_bitvec_update_intersection(fastset_bitvec_t *res, const fastset_bitvec_t *arg)
{
	__fastset_bitvec_intersection(res, res, arg);
}

fastset_bitvec_t *
fastset_bitvec_intersection(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2)
{
	fastset_bitvec_t *res;

	res = fastset_bitvec_new(0);

	__fastset_bitvec_intersection(res, arg1, arg2);

	return res;
}

fastset_bitvec_t *
fastset_bitvec_difference(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2)
{
	fastset_bitvec_t *res;

	res = fastset_bitvec_copy(arg1);
	fastset_bitvec_update_difference(res, arg2);
	return res;
}

void
fastset_bitvec_update_difference(fastset_bitvec_t *res, const fastset_bitvec_t *arg)
{
	unsigned int n, nbits, count;

	nbits = MIN(res->max_index, arg->max_index);
	if (nbits > 0) {
		count = fastset_bitvec_bits_to_size(nbits - 1);
		for (n = 0; n < count; ++n)
			res->words[n] &= ~(arg->words[n]);
	}
}

fastset_bitvec_t *
fastset_bitvec_symmetric_difference(const fastset_bitvec_t *arg1, const fastset_bitvec_t *arg2)
{
	fastset_bitvec_t *res;

	res = fastset_bitvec_copy(arg1);
	fastset_bitvec_update_symmetric_difference(res, arg2);

	return res;
}

void
fastset_bitvec_update_symmetric_difference(fastset_bitvec_t *res, const fastset_bitvec_t *arg)
{
	unsigned int n, max_index;

	max_index = MAX(res->max_index, arg->max_index);
	fastset_bitvec_resize(res, max_index);

	for (n = 0; n < arg->nwords; ++n)
		res->words[n] ^= arg->words[n];
}

bool
fastset_bitvec_test_subset(const fastset_bitvec_t *subset, const fastset_bitvec_t *superset)
{
	unsigned int n, nwords;
	fastset_bitvec_word_t test = 0;

	nwords = MIN(subset->nwords, superset->nwords);

	for (n = 0; n < nwords; ++n)
		test |= (subset->words[n] & ~(superset->words[n]));

	for (; n < subset->nwords; ++n)
		test |= subset->words[n];

	return test == 0;
}

bool
fastset_bitvec_test_disjoint(const fastset_bitvec_t *subset, const fastset_bitvec_t *superset)
{
	unsigned int n, nwords;
	fastset_bitvec_word_t test = 0;

	nwords = MIN(subset->nwords, superset->nwords);

	for (n = 0; n < nwords; ++n)
		test |= (subset->words[n] & superset->words[n]);

	return test == 0;
}

bool
fastset_bitvec_test_empty(const fastset_bitvec_t *vec)
{
	unsigned int i;

	for (i = 0; i < vec->nwords; ++i) {
		if (vec->words[i])
			return false;
	}

	return true;
}

bool
fastset_bitvec_test_bit(const fastset_bitvec_t *vec, unsigned int index)
{
	unsigned int word_index;
	fastset_bitvec_word_t mask;

	if (!fastset_bitvec_bit_to_index(vec, index, &word_index, &mask))
		return false;
	return !!(vec->words[word_index] & mask);
}

/*
 * precomputed transforms
 */
fastset_bitvec_transform_t *
fastset_bitvec_transform_new(unsigned int max_index)
{
	fastset_bitvec_transform_t *trans;
	unsigned int i;

	trans = calloc(1, sizeof(*trans));
	trans->max_index = max_index;
	trans->mapping = calloc(max_index, sizeof(int));

	for (i = 0; i < max_index; ++i)
		trans->mapping[i] = -1;

	return trans;
}

void
fastset_bitvec_transform_add(fastset_bitvec_transform_t *trans, unsigned int arg_index, int res_index)
{
	assert(arg_index < trans->max_index);

	trans->mapping[arg_index] = res_index;
}

void
fastset_bitvec_transform_free(fastset_bitvec_transform_t *trans)
{
	free(trans->mapping);
	trans->mapping = NULL;
	trans->max_index = 0;

	free(trans);
}

fastset_bitvec_t *
fastset_bitvec_transform(const fastset_bitvec_t *vec, const fastset_bitvec_transform_t *trans)
{
	unsigned int max_index = MIN(vec->max_index, trans->max_index);
	fastset_bitvec_t *res;
	unsigned int from_index = 0;

	res = fastset_bitvec_new(max_index);
	while (true) {
		int arg_bit, res_bit;

		/* Find the next bit in the argument bitvec */
		arg_bit = fastset_bitvec_find_next_bit(vec, from_index);
		if (arg_bit < 0)
			break;

		from_index = arg_bit + 1;

		assert((unsigned int) arg_bit < max_index);

		/* transform using the given mapping. a negative value means
		 * the mapping is not defined for this bit.
		 * We choose to ignore this. Alternatively, we could error out.
		 */
		res_bit = trans->mapping[arg_bit];
		if (res_bit >= 0)
			fastset_bitvec_set(res, res_bit);
	}

	return res;
}

int
fastset_bitvec_compare(const fastset_bitvec_t *vec1, const fastset_bitvec_t *vec2)
{
	unsigned int min_word_index;
	unsigned int pos, state = 0;

	min_word_index = MIN(vec1->nwords, vec2->nwords);
	for (pos = 0; pos < min_word_index; ++pos) {
		fastset_bitvec_word_t word1, word2;

		word1 = vec1->words[pos];
		word2 = vec2->words[pos];
		if (word1 & ~word2)
			state |= FASTSET_REL_GREATER_THAN; /* vec1 is NOT a subset of vec2 */
		if (word2 & ~word1)
			state |= FASTSET_REL_LESS_THAN; /* vec1 is NOT a superset of vec2 */
	}

	while (pos < vec1->nwords) {
		if (vec1->words[pos++] != 0)
			state |= FASTSET_REL_GREATER_THAN; /* vec1 is NOT a subset of vec2 */
	}

	while (pos < vec2->nwords) {
		if (vec2->words[pos++] != 0)
			state |= FASTSET_REL_LESS_THAN; /* vec1 is NOT a superset of vec2 */
	}

	return state;
}
