#ifndef _ADV_LIST_H
#define _ADV_LIST_H

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *head)
{
        head->next = head;
        head->prev = head;
}

static inline int list_empty(const struct list_head *head)
{
        return head->next == head;
}

static void __list_add(struct list_head *new_lst,
                              struct list_head *prev,
                              struct list_head *next)
{
        next->prev = new_lst;
        new_lst->next = next;
        new_lst->prev = prev;
        prev->next = new_lst;
}

static inline void list_add(struct list_head *new_lst, 
              struct list_head *head)
{
	__list_add(new_lst, head, head->next);
}


static inline void list_add_tail(struct list_head *new_lst, struct list_head *head)
{
        __list_add(new_lst, head->prev, head);
}

static  void __list_del(struct list_head * prev, 
                              struct list_head * next  )
{
        next->prev = prev;
        prev->next = next;
}

#define LIST_POISON1 0
#define LIST_POISON2 0

static inline void list_del(struct list_head * entry)
{
        __list_del(entry->prev,entry->next);
        entry->next = LIST_POISON1;
        entry->prev = LIST_POISON2;
}

#if __linux__   //  or #if __GNUC__
    #if __LP64__
        #define ENVIRONMENT64
	#pragma message("Linux 64bit")
    #else
        #define ENVIRONMENT32
	#pragma message("Linux 32bit")
    #endif
#else
    #if _WIN32
        #define ENVIRONMENT32
	#pragma message("Windows 32bit")
    #else
        #define ENVIRONMENT64
	#pragma message("Windows 64bit")
    #endif
#endif // __linux__

#ifdef ENVIRONMENT64
    #define PTR_OFFSET (unsigned long long int)
#else
    #define PTR_OFFSET (unsigned long int)
#endif // ENVIRONMENT64

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define adv_offsetof(TYPE, MEMBER) (PTR_OFFSET &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
               (type *)( (char *)__mptr - adv_offsetof(type,member) );})

#define list_entry(ptr,type,member)     \
    container_of(ptr, type, member)

#endif
