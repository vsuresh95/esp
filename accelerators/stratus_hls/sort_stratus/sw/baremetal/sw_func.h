#include <stdio.h>

static void insertion_sort(float *value, int len)
{
	int i;

	for (i = 1; i < len; i++) {
		double current;
		int empty;

		current = value[i];
		empty = i;

		while (empty > 0 && current < value[empty-1]) {
			value[empty] = value[empty-1];
			empty--;
		}

		value[empty] = current;
	}
}

static int partition(float * array, int low, int high)
{
	int left, right, mid;
	int pivot;
	float cur;

	mid = (low + high) / 2;
	left = low;
	right = high;

	/* choose pivot as median of 3: low, high, and mid */
	if ((array[low] - array[mid]) * (array[high] - array[low]) >= 0)
		pivot = low;
	else if ((array[mid] - array[low]) * (array[high] - array[mid]) >= 0)
		pivot = mid;
	else
		pivot = high;

	/* store value,index at the pivot */
	cur = array[pivot];

	/* swap pivot with the first entry in the list */
	array[pivot] = array[low];
	array[low] = cur;

	/* the quicksort itself */
	while (left < right)
	{
		while (array[left] <= cur && left < high)
			left++;
		while (array[right] > cur)
			right--;
		if (left < right)
		{
			float tmp_val;

			tmp_val = array[right];
			array[right] = array[left];
			array[left] = tmp_val;

		}
	}

	/* pivot was in low, but now moves into position at right */
	array[low] = array[right];
	array[right] = cur;

	return right;
}


/* This defines the length at which we switch to insertion sort */
#define MAX_THRESH 10

int quicksort_inner(float *array, int low, int high)
{
	int pivot;
	int length = high - low + 1;

	if (high > low) {
		if (length > MAX_THRESH) {
			pivot = partition (array, low, high);
			quicksort_inner (array, low, pivot-1);
			quicksort_inner (array, pivot+1, high);
		}
	}

	return 0;
}

int quicksort(float *array, int len)
{
	quicksort_inner(array, 0, len-1);
	insertion_sort(array, len);
	return 0;
}

// static void sort_comp(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);
// 	int i;

// 	for (i = 0; i < t->n_batches; i++)
// 		quicksort(&t->sbuf[i * t->n_elems], t->n_elems);
// }
