#ifndef _RBTREE_H
#define _RBTREE_H

#ifndef NULL
#define NULL 0
#endif

#include "sys-tree.h"

typedef struct _RBTREE_ENTRY RBTREE_ENTRY;
typedef struct _RBTREE_HEADER RBTREE_HEADER;
typedef int(*RBCOMPARE)(RBTREE_ENTRY *a, RBTREE_ENTRY *b);
RB_HEAD(RBTREE, _RBTREE_ENTRY) RBTREE;

typedef struct _RBTREE_HEADER {
	struct RBTREE tree;
	unsigned int depth;
	RBCOMPARE compare;
} RBTREE_HEADER;

typedef struct _RBTREE_ENTRY {
	RB_ENTRY(_RBTREE_ENTRY) entry;
	RBTREE_HEADER *head;
} RBTREE_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif

	//��ʼ��
	void rbtree_init(RBTREE_HEADER *head, RBCOMPARE compare);

	//���� (�ɹ����� NULL���� �� entry)
	RBTREE_ENTRY * rbtree_insert(RBTREE_HEADER *head, RBTREE_ENTRY *entry);
	
	//�Ƴ� (�ɹ����� entry)
	RBTREE_ENTRY * rbtree_remove(RBTREE_HEADER *head, RBTREE_ENTRY *entry);

	//�� (���� entry)
	RBTREE_ENTRY * rbtree_root(RBTREE_HEADER *head);

	//���� (���� entry)
	RBTREE_ENTRY * rbtree_find(RBTREE_HEADER *head, RBTREE_ENTRY *entry);
	RBTREE_ENTRY * rbtree_nfind(RBTREE_HEADER *head, RBTREE_ENTRY *entry);

	//���� (���� entry)
	RBTREE_ENTRY * rbtree_prev(RBTREE_HEADER *head, RBTREE_ENTRY *entry);
	RBTREE_ENTRY * rbtree_next(RBTREE_HEADER *head, RBTREE_ENTRY *entry);

	//С�� (���� entry)
	RBTREE_ENTRY * rbtree_min(RBTREE_HEADER *head);
	RBTREE_ENTRY * rbtree_max(RBTREE_HEADER *head);

	//���� (���� int)
	unsigned int rbtree_count(RBTREE_HEADER *head);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* End of rbtree.h */
