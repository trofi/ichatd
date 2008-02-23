#ifndef __LIST_H__
#define __LIST_H__

#define list_for_each(entry, head)  \
    for (entry = head;              \
         entry != 0;                \
         entry = entry->next)

#define list_prepend(entry, head)   \
    {                               \
        entry->next = head;         \
        head = entry;               \
    }

#endif // __LIST_H__
