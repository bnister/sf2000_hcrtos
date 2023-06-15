#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
//#ifndef uint32_t
//#define uint32_t unsigned long int
//#endif

typedef struct _glist glist;
struct _glist
{
	void *data;
	glist *next;
	glist *prev;
};

typedef struct _gqueue
{
	glist *head;
	glist *tail;
	unsigned  length;
}gqueue;

#define glist_prev(list)			((list) ? (((glist *)(list))->prev) : NULL)
#define glist_next(list)			((list) ? (((glist *)(list))->next) : NULL)

typedef void (*GDes) (void *data);
typedef void (*GFunc) (void *data, void *user_data);
typedef int (*GCompareDataFunc) (void *a, void *b, void *user_data);

glist* glist_last (glist *list);

glist* glist_first (glist *list);

void glist_free_node (glist *list);

void glist_free (glist *list);

void glist_free_full (glist *list, GFunc free_func);

glist* glist_append (glist *list, void *data);

glist* glist_prepend (glist *list, void *data);

uint32_t glist_length (glist *list);

int glist_data_position (glist *list, void *data);

int glist_link_position (glist *list, glist *llink);

glist* glist_find (glist *list,void *data);

glist* glist_find_custom (glist *list, void *data, 
	GCompareDataFunc func);

glist* glist_find_custom_from_the_back (glist *list, void *data, 
	GCompareDataFunc func);

void glist_foreach (glist *list, GFunc func, void *user_data);

glist* glist_remove (glist *list, void *data);

glist* glist_remove_all (glist *list, void *data);

glist* glist_delete_link (glist *list, glist *link);

glist* glist_copy (glist *list);

glist* glist_reverse (glist *list);

int glist_index (glist *list,void *data);

int glist_position (glist *list, glist *llink);

glist* glist_nth (glist *list, uint32_t n);

void * glist_nth_data (glist *list, unsigned n);

glist* glist_nth_prev (glist* list, uint32_t n);

glist* glist_insert (glist *list, void *data, int position);

glist* glist_sort (glist *list, GCompareDataFunc compare_func,
	void *user_data);

glist* glist_merge (glist *list1, glist *list2);

glist* glist_remove_link (glist *list, glist *link);

void gqueue_init (gqueue *queue);

void gqueue_clear (gqueue *queue);

void gqueue_free (gqueue *queue);

void gqueue_push_tail (gqueue  *queue, void * data);

glist* gqueue_peek_head_link (gqueue *queue);

void gqueue_foreach (gqueue *queue, GFunc func, void *user_data);

int atomic_int_exchange_and_add (pthread_mutex_t *atomic_mutex, 
	volatile int *atomic, int val);


#define ptr_array_index(array,index) (((array)->pdata)[index])

typedef struct _RealPtrArray
{
	void	 **pdata;
	unsigned		 len;
	unsigned		 alloc;
	volatile int ref_count;
	pthread_mutex_t atomic_mutex;
	GDes element_free_func;
}RealPtrArray;

typedef struct _PtrArray
{
	void	 **pdata;
	unsigned		len;
}PtrArray;

PtrArray* ptr_array_new (void);

void **ptr_array_free (PtrArray *farray, int8_t free_segment);

void ptr_array_add (PtrArray *farray, void *data);

PtrArray *ptr_array_ref (PtrArray *array);

void ptr_array_unref (PtrArray *array);

void ptr_array_set_free_func (PtrArray *array, GDes element_free_func);

