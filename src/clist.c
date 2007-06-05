#include "clist.h"

// initialize system vars etc...
// should not be called for already initialized data,
// we need to check for it... but lazyness
void
clist_init (CLIST * clist)
{
    if (!clist)
        return;
    // by default, list is empty
    clist->head = NULL;
    
    // initialize mutex
    pthread_mutex_init (& clist->mutex, NULL);
    
    // ... and conditinal wait...
    pthread_cond_init (&clist->empty_cond, NULL);
}

// no syncs here, as we should never call this function 
// from non-main thread
void
clist_clean (CLIST * clist)
{
    if (!clist)
        return;

    pthread_cond_destroy (&clist->empty_cond);
    pthread_mutex_destroy (&clist->mutex);
}

// check for emptyness of list
void
clist_waitempty (CLIST* clist)
{
    pthread_mutex_lock (&clist->mutex);
    
    // if we are not empty
    while (clist->head)
        pthread_cond_wait (&clist->empty_cond, &clist->mutex);
    
    pthread_mutex_unlock (&clist->mutex);
}

// remove specified client
void
clist_remove (CLIST * clist, void * item)
{
    if (!clist || !item)
        return;

    CLISTITEM * client = (CLISTITEM *) item;

    pthread_mutex_lock (&clist->mutex);
    
    // find element to be destroyed    
    // but first check for non NULL head

    //i think there's some redundancy
    if (clist->head)
    {
        if (clist->head == client)
            clist->head = clist->head->next;
        else
        {
            CLISTITEM * temp = clist->head;
            
            // loop while we have next element and it's data 
            while (temp->next
                   && temp->next != client)
                temp = temp->next;

            if (temp->next)
                temp->next = temp->next->next;
        }
    }
    if (clist->head == NULL)
        pthread_cond_broadcast (&clist->empty_cond);

    pthread_mutex_unlock (&clist->mutex);
}

// adds user data to the list
void
clist_add (CLIST * clist, void * item)
{
    if (!clist || !item)
        return;

    CLISTITEM * client = (CLISTITEM *) item;

    pthread_mutex_lock (&clist->mutex);

    client->next = clist->head;
    clist->head = client;

    pthread_mutex_unlock (&clist->mutex);
}

int
clist_firstthat (CLIST * clist, CLIST_ITERATOR function, void * cookie)
{
    CLISTITEM * temp;
    int result = 0;

    pthread_mutex_lock (&clist->mutex);
    
    for (temp = clist->head; temp; temp = temp->next)
        if (!function (temp, cookie))
        {
            result = 1;
            break;
        }
    
    pthread_mutex_unlock (&clist->mutex);

    return result;
}
