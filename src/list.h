#ifndef _LIST_H
#define _LIST_H

#include <stdio.h>

#include "types.h"

extern u64 nvm_addr2off(void *addr);
extern void *nvm_off2addr(u64 offset);

struct list_head {
  u64 next;
  u64 prev;
};

#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)

static inline void INIT_LIST_HEAD(struct list_head *list) {
  list->next = nvm_addr2off(list);
  list->prev = nvm_addr2off(list);
}

// container_of - cast a member of a structure out to the containing structure
// @ptr:        the pointer to the member.
// @type:       the type of the container struct this is embedded in.
// @member:     the name of the member within the struct.
#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

// list_entry - get the struct for this entry
// @ptr:    the &struct list_head pointer.
// @type:   the type of the struct this is embedded in. * @member: the name of
// the list_head within the struct.
#define list_entry(ptr, type, member) container_of(ptr, type, member)

// Insert a new entry between two known consecutive entries.
//
// This is only for internal list manipulation where we know
// the prev/next entries already!
static inline void __list_add(struct list_head *new_node,
                              struct list_head *prev, struct list_head *next) {
  next->prev = nvm_addr2off(new_node);
  new_node->next = nvm_addr2off(next);
  new_node->prev = nvm_addr2off(prev);
  prev->next = nvm_addr2off(new_node);
}

// list_add - add a new entry
// @new: new entry to be added
// @head: list head to add it after
//
// Insert a new entry after the specified head.
// This is good for implementing stacks.
static inline void list_add(struct list_head *new_node,
                            struct list_head *head) {
  __list_add(new_node, head, nvm_off2addr(head->next));
}

// Delete a list entry by making the prev/next entries
// point to each other.
//
// This is only for internal list manipulation where we know
// the prev/next entries already!
static inline void __list_del(struct list_head *prev, struct list_head *next) {
  next->prev = nvm_addr2off(prev);
  prev->next = nvm_addr2off(next);
}

// list_move - delete from one list and add as another's head
// @list: the entry to move
// @head: the head that will precede our entry
static inline void list_move(struct list_head *list, struct list_head *head) {
  __list_del(nvm_off2addr(list->prev), nvm_off2addr(list->next));
  list_add(list, head);
}

// list_empty - tests whether a list is empty
// @head: the list to test.
static inline int list_empty(const struct list_head *head) {
  return head->next == nvm_addr2off(head);
}

#endif /* _LIST_H */

