#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "glist.h"

#ifndef max
#define max(a,b) (a>b?a:b)
#endif

glist* glist_last (glist *list)
{
	if (list){
		while (list->next)
			list = list->next;
	}
	return list;
}

glist* glist_first (glist *list)
{
	if (list){
		while (list->prev)
			list = list->prev;
	}

	return list;
}

void glist_free_node (glist *list)
{
	free (list);
}

void glist_free (glist *list)
{
	glist *next;
	while (list){
		next = list->next;
		free (list);
		list = next;
	}
}

void glist_free_full (glist *list, GFunc free_func)
{
	glist_foreach (list, free_func, NULL);
	glist_free (list);
}

glist* glist_append (glist *list, void *data)
{
	glist *new_list;
	glist *last;

	new_list = (glist *)malloc(sizeof (glist));
	if(!new_list)
		return list;
	
	memset(new_list, 0, sizeof(glist));
	new_list->data = data;
	if (list){
		last = glist_last (list);
		last->next = new_list;
		new_list->prev = last;

		return list;
	}
	return new_list;
}

glist* glist_prepend (glist *list, void *data)
{
	glist *new_list;

	new_list = (glist *)malloc(sizeof (glist));
	if(!new_list)
		return list;

	new_list->data = data;
	new_list->next = list;

	if (list){
		new_list->prev = list->prev;
		if (list->prev)
			list->prev->next = new_list;
		list->prev = new_list;
	}
	else
		new_list->prev = NULL;

	return new_list;
}

uint32_t glist_length (glist *list)
{
	uint32_t length = 0;

	while (list){
		length++;
		list = list->next;
	}

	return length;
}

int glist_data_position (glist *list, void *data)
{
	int i = 0;
	while (list){
		if (list->data == data)
			return i;
		i++;
		list = list->next;
	}

	return -1;
}

int glist_link_position (glist *list, glist *llink)
{
	int i= 0;
	while (list){
		if (list == llink)
			return i;
		i++;
		list = list->next;
	}

	return -1;
}

glist* glist_find (glist *list,void *data)
{
	while (list){
		if (list->data == data)
			break;
		list = list->next;
	}

	return list;
}

glist* glist_find_custom (glist *list, void *data, GCompareDataFunc func)
{
	if (!func)
		return list;

	while (list){
		if (! func (list->data, data, NULL))
			return list;
		list = list->next;
	}

	return NULL;
}

glist* glist_find_custom_from_the_back (glist *list, void *data, 
	GCompareDataFunc func)
{
	if (!func)
		return list;

	list = glist_last (list);
	while (list){
		if (! func (list->data, data, NULL))
			return list;
		list = list->prev;
	}

	return NULL;
}

void glist_foreach (glist *list, GFunc func, void *user_data)
{
	while (list){
		glist *next = list->next;
		(*func) (list->data, user_data);
		list = next;
	}
}

glist* glist_remove (glist *list, void *data)
{
	glist *tmp;

	tmp = list;
	while (tmp){
		if (tmp->data != data)
			tmp = tmp->next;
		else {
			if (tmp->prev)
				tmp->prev->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = tmp->prev;

			if (list == tmp)
				list = list->next;

			glist_free (tmp);

			break;
		}
	}
	return list;
}

glist* glist_remove_all (glist	*list, void *data)
{
	glist *tmp = list;

	while (tmp){
		if (tmp->data != data)
			tmp = tmp->next;
		else {
			glist *next = tmp->next;

			if (tmp->prev)
				tmp->prev->next = next;
			else
				list = next;
			if (next)
				next->prev = tmp->prev;

			glist_free (tmp);
			tmp = next;
		}
	}
	return list;
}

glist* glist_remove_link (glist *list, glist *link)
{
	if (link){
		if (link->prev)
			link->prev->next = link->next;
		if (link->next)
			link->next->prev = link->prev;

		if (link == list)
			list = list->next;

		link->next = NULL;
		link->prev = NULL;
	}

	return list;
}

glist* glist_delete_link (glist *list, glist *link)
{
  list = glist_remove_link (list, link);
  glist_free_node (link);

  return list;
}

/*only copy list, do not copy data*/
glist* glist_copy (glist *list)
{
	glist *new_list = NULL;

	if (list){
		glist *last;

		new_list = (glist *)malloc(sizeof (glist));
		if(!new_list)
			return list;

		memset(new_list, 0, sizeof(glist));
		new_list->data = list->data;
		last = new_list;
		list = list->next;
		while (list){
			last->next = (glist *)malloc(sizeof (glist));
			if(!last->next){
				glist_free(new_list);
				return NULL;
			}
			last->next->prev = last;
			last = last->next;
			last->data = list->data;
			list = list->next;
		}
	}

	return new_list;
}

/**
 * glist_reverse:
 * reverse "1st element->2nd element->.....->nth element" to "nth element->.....->2nd element->1st element"
 *
 * Returns: new list
 */
glist* glist_reverse (glist *list)
{
	glist *last = NULL;

	while (list){
		last = list;
		list = last->next;
		last->next = last->prev;
		last->prev = list;
	}

	return last;
}

int glist_index (glist *list,void *data)
{
	int i;
  
	i = 0;
	while (list)
	{
		if (list->data == data)
			return i;
		i++;
		list = list->next;
	}
  
	return -1;
}

int glist_position (glist *list, glist *llink)
{
  int i;

  i = 0;
  while (list)
	{
	  if (list == llink)
	return i;
	  i++;
	  list = list->next;
	}

  return -1;
}


glist* glist_nth (glist *list, uint32_t n)
{
	while ((n-- > 0) && list)
		list = list->next;

	return list;
}

void * glist_nth_data (glist *list, unsigned n)
{
	while ((n-- > 0) && list)
		list = list->next;
	
	return list ? list->data : NULL;
}

glist* glist_nth_prev (glist* list, uint32_t n)
{
	while ((n-- > 0) && list)
		list = list->prev;

	return list;
}

/**
 * glist_insert:
 * insert @data to the @position_th of @list 
 *
 * Returns: new list
 */
glist* glist_insert (glist *list, void *data, int position)
{
	glist *new_list;
	glist *tmp_list;

	if (position < 0)
		return glist_append (list, data);
	else if (position == 0)
		return glist_prepend (list, data);

	tmp_list = glist_nth (list, position);
	if (!tmp_list)
		return glist_append (list, data);

	new_list = (glist *)malloc(sizeof (glist));
	if(!new_list)
		return list;

	new_list->data = data;
	new_list->prev = tmp_list->prev;
	if (tmp_list->prev)
		tmp_list->prev->next = new_list;
	new_list->next = tmp_list;
	tmp_list->prev = new_list;

	if (tmp_list == list)
		return new_list;
	else
		return list;
}

static glist * glist_sort_merge (glist *l1, glist *l2, GCompareDataFunc compare_func,
	void *user_data)
{
	glist list, *l, *lprev;
	int cmp;

	l = &list; 
	lprev = NULL;

	while (l1 && l2){
		cmp = (compare_func) (l1->data, l2->data, user_data);

		if (cmp <= 0){
			l->next = l1;
			l1 = l1->next;
		} 
		else {
			l->next = l2;
			l2 = l2->next;
		}
		l = l->next;
		l->prev = lprev; 
		lprev = l;
	}
	l->next = l1 ? l1 : l2;
	l->next->prev = l;

	return list.next;
}

/**
 * glist_sort:
 * sort @list's elments by @compare_func(return 0/1), we can pass a @user_data if needed.
 *
 * Returns: new list
 */
glist* glist_sort (glist *list, GCompareDataFunc compare_func, void *user_data)
{
	glist *l1, *l2;

	if (!list) 
		return NULL;
	if (!list->next) 
		return list;

	//below is the insertion method. 1. get the last list, 2. insert the last list to the previous sorted list.
	/***************************************/
	//below equals to  "l2 = glist_last (list); list = glist_remove_link (list, l2);"
	l1 = list; 
	l2 = list->next;

	while ((l2 = l2->next) != NULL){
		if ((l2 = l2->next) == NULL) 
			break;
		l1 = l1->next;
	}
	l2 = l1->next; 
	l1->next = NULL; 
	/***************************************/
	return glist_sort_merge (glist_sort (list, compare_func, user_data),
		glist_sort (l2, compare_func, user_data),
		compare_func, user_data);
}

/**
 * glist_merge:
 * append list2 to list1's end.
 *
 * Returns: list1
 */
glist* glist_merge (glist *list1, glist *list2)
{
	glist *tmp_list;

	if (list2){
		tmp_list = glist_last (list1);
		if (tmp_list)
			tmp_list->next = list2;
		else
			list1 = list2;
		list2->prev = tmp_list;
	}

	return list1;
}

void gqueue_init (gqueue *queue)
{
	if(!queue)
		return;
  
	queue->head = queue->tail = NULL;
	queue->length = 0;
}

void gqueue_clear (gqueue *queue)
{
	if(!queue)
		return;
	
	glist_free (queue->head);
	gqueue_init (queue);
}

void gqueue_free (gqueue *queue)
{
	if(!queue)
		return;
	glist_free (queue->head);
	free(queue);
}


void gqueue_push_tail (gqueue  *queue, void * data)
{
	if(!queue)
		return;

	queue->tail = glist_append (queue->tail, data);
	if (queue->tail->next)
		queue->tail = queue->tail->next;
	else
		queue->head = queue->tail;
	queue->length++;
}

glist* gqueue_peek_head_link (gqueue *queue)
{
	if(!queue)
		return NULL;

	return queue->head;
}


void gqueue_foreach (gqueue *queue, GFunc func, void *user_data)
{
  glist *list;

  if(!queue)
	  return;
  if(!func)
	  return;
  
  list = queue->head;
  while (list)
	{
	  glist *next = list->next;
	  func (list->data, user_data);
	  list = next;
	}
}

int atomic_int_exchange_and_add (pthread_mutex_t *atomic_mutex, 
	volatile int *atomic, int val)
{
	int result;

	if(!atomic_mutex)
		return *atomic + val;

	pthread_mutex_lock(atomic_mutex);
	result = *atomic;
	*atomic += val;
	pthread_mutex_unlock(atomic_mutex);

	return result;
}

PtrArray* ptr_array_new (void)
{
	RealPtrArray *array = malloc(sizeof(RealPtrArray));

	if(!array){
		return NULL;
	}

	pthread_mutex_init(&array->atomic_mutex, NULL);
#ifndef __linux__
	if(!array->atomic_mutex){
		free(array);
		return NULL;
	}
#endif
	array->pdata = NULL;
	array->len = 0;
	array->alloc = 0;
	array->ref_count = 1;
	array->element_free_func = NULL;

	return (PtrArray*) array;  

}

static unsigned nearest_pow (int num)
{
  unsigned n = 1;

  while (n < num && n > 0)
	n <<= 1;

  return n ? n : num;
}

static void ptr_array_maybe_expand (RealPtrArray *array, int len)
{
	if((array->len + len) > array->alloc)
	{
		unsigned old_alloc = array->alloc;
		array->alloc = nearest_pow (array->len + len);
		array->alloc = max(array->alloc, 16);
		if(old_alloc == array->alloc){
			printf("array size is too big");
			return;
		}
		array->pdata = realloc(array->pdata, sizeof(void *) * array->alloc);
		//free(array->pdata);
		//array->pdata = malloc(sizeof(void *) * array->alloc);
	}
}


void ptr_array_add (PtrArray *farray, void *data)
{
	RealPtrArray* array = (RealPtrArray*) farray;

	if(!array)
		return;

	ptr_array_maybe_expand (array, 1);

	array->pdata[array->len++] = data;
}


static void ptr_array_foreach (PtrArray *array,
					 GFunc func,
					 void *user_data)
{
	int i;
	
	if(!array)
		return;
	
	
	for (i = 0; i < array->len; i++)
		(*func)(array->pdata[i], user_data);
}

void **ptr_array_free (PtrArray *farray, int8_t free_segment)
{
	RealPtrArray *array = (RealPtrArray*) farray;
	void **segment;
	int8_t preserve_wrapper;

	if(!array)
		return NULL;

	/* if others are holding a reference, preserve the wrapper but do free/return the data */
	preserve_wrapper = 0;
	if (array->ref_count > 1)
		preserve_wrapper = 1;

	if (free_segment){
		if (array->element_free_func != NULL)
			ptr_array_foreach (farray, (GFunc) array->element_free_func, NULL);
		if(array->pdata)
			free(array->pdata);
		segment = NULL;
	}
	else
		segment = array->pdata;

	if (preserve_wrapper){
		array->pdata = NULL;
		array->len = 0;
		array->alloc = 0;
	}
	else {
		pthread_mutex_destroy(&array->atomic_mutex);
		free(array);
	}

	return segment;
}


PtrArray *ptr_array_ref (PtrArray *array)
{
	RealPtrArray *rarray = (RealPtrArray*) array;

	if(!array || rarray->ref_count <= 0)
		return array;

	atomic_int_exchange_and_add(&rarray->atomic_mutex, &rarray->ref_count, 1);
	return array;
}

void ptr_array_unref (PtrArray *array)
{
  RealPtrArray *rarray = (RealPtrArray*) array;

  if(!array || rarray->ref_count <= 0)
	return;
  if (atomic_int_exchange_and_add(&rarray->atomic_mutex, &rarray->ref_count, -1))
	ptr_array_free (array, 1);
}


void ptr_array_set_free_func (PtrArray *array, GDes element_free_func)
{
	RealPtrArray* rarray = (RealPtrArray*) array;

	if (!rarray)
		return;
	rarray->element_free_func = element_free_func;
}


