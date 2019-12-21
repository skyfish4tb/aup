#include <stdio.h>

#include "value.h"
#include "memory.h"

void aupVa_init(aupVa *array)
{
	array->count = 0;
	array->capacity = 0;
	array->values = NULL;
}

int aupVa_write(aupVa *array, aupV value)
{
	if (array->capacity < array->count + 1) {
		int oldCapacity = array->capacity;
		array->capacity = AUP_GROW_CAP(oldCapacity);
		array->values = AUP_GROW_ARR(aupV, array->values, oldCapacity, array->capacity);
	}

	array->values[array->count] = value;
	return array->count++;
}

void aupVa_free(aupVa *array)
{
	AUP_FREE_ARR(aupV, array->values, array->capacity);
	aupVa_init(array);
}