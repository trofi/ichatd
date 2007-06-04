#include "clist.h"

/*
  list of functions that support
  access to the connection list
*/

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
    //    CLIENT* temp;
    
    // actually, this clean up calls do nothing, but POSIX...
    pthread_cond_destroy (&clist->empty_cond);
    pthread_mutex_destroy (&clist->mutex);
    
    // no memory cleaning...
    /*    // oki, clean the memory
      while( (temp = clist->head) )
      {
      clist->head = temp->next;

      // maybe we need to move memore freeing routines to the dedicated function
      // done it
      ulist_cleanupuser( temp );
      }
      // ulist_head is NULL here*/
}

// check for emptyness of lust :)))
void
clist_waitempty (CLIST* clist)
{
    // lock access
    pthread_mutex_lock (&clist->mutex);
    
    // if we are not empty
    if (clist->head) //while ??
        // wait until we became empty... 
        // !!! we don't use CPU time here, considering docs
        pthread_cond_wait (&clist->empty_cond, &clist->mutex);
    
    // oki, allow others...
    pthread_mutex_unlock (&clist->mutex);
}

// remove the client that is specified
void
clist_remove (CLIST * clist, void * item)
{
    if (!clist || !item)
        return;

    CLISTITEM * client = (CLISTITEM *) item;

    // lock access to the list
    pthread_mutex_lock (&clist->mutex);
    
    // find element to be destroyed    
    // but first check for non NULL head
    if (clist->head)
    {
        // check for head item... it is special case
        if (clist->head == client)
        {
            // shift head forward
            clist->head = clist->head->next;
        
            // may be, we need to cleanup user here...
        }
        else
        {
            CLISTITEM * temp = clist->head;
    
            // loop while we have next element and it's data 
            while (temp->next
                   && temp->next != client)
                temp = temp->next;

            if (temp->next)
            {
                // we can get here only if we have founded user entry
                // so temp->next == user
                // we need to link other users
                // i.e. make next user whom was the next for rempved item
                temp->next = temp->next->next;
            }
        }
    }

    // check for emptyness after removing...
    // if we are empty, then send broadcast about it
    // thus we allow threads wating for waitempty to wakeup
    if (clist->head == NULL)
        pthread_cond_broadcast (&clist->empty_cond);

    // allow other threads to get access to the list    
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

    // lock access
    pthread_mutex_lock (&clist->mutex);
    
    for (temp = clist->head; temp; temp = temp->next)
    {
        if (!function (temp, cookie))
        {
            result = 1;
            break;
        }
    }
    
    // free access (look comment above for lock)
    pthread_mutex_unlock (&clist->mutex);
    
    //
    return result;
}
