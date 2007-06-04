#ifndef __CLIST_H__
#define __CLIST_H__

#include <pthread.h>

struct tagCLISTITEM
{
    struct tagCLISTITEM * next;
};

typedef struct tagCLISTITEM CLISTITEM;

struct tagCLIST
{
    pthread_mutex_t       mutex;
    pthread_cond_t        empty_cond;
    struct tagCLISTITEM * head;
};

typedef struct tagCLIST CLIST;

// should return 1 to continue traversing, 0 - to break...
// be sure, this function will be called fromin the global list lock
typedef int (*CLIST_ITERATOR)(void * item, void * cookie);

// 1 - was breaked, 0 - all calls succeded
int clist_firstthat (CLIST * clist, CLIST_ITERATOR function, void * cookie);
void clist_init (CLIST * clist);
void clist_add (CLIST * clist, void * item);
void clist_remove (CLIST * clist, void * item);
void clist_clean (CLIST * clist);
void clist_waitempty (CLIST* clist);

#endif // __CLIST_H__
