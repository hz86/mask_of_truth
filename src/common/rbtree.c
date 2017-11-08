#include "rbtree.h"
static int rb_cmp(RBTREE_ENTRY *a, RBTREE_ENTRY *b);
RB_PROTOTYPE_INTERNAL(RBTREE, _RBTREE_ENTRY, entry, rb_cmp, static)
RB_GENERATE_INTERNAL(RBTREE, _RBTREE_ENTRY, entry, rb_cmp, static)

//比较函数
static int rb_cmp(RBTREE_ENTRY *a, RBTREE_ENTRY *b)
{
	return a->head->compare(a, b);
}

//初始化
void rbtree_init(RBTREE_HEADER *head, RBCOMPARE compare)
{
	head->depth = 0;
	head->compare = compare;
	RB_INIT(&head->tree);
}

//插入
RBTREE_ENTRY * rbtree_insert(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	entry->head = head;
	RBTREE_ENTRY * old = RB_INSERT(RBTREE, &head->tree, entry);
	if(NULL == old) head->depth++;
	return old;
}

//移除
RBTREE_ENTRY * rbtree_remove(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	RBTREE_ENTRY * old = RB_REMOVE(RBTREE, &head->tree, entry);
	if(NULL != old) head->depth--;
	return old;
}

//根
RBTREE_ENTRY * rbtree_root(RBTREE_HEADER *head)
{
	return RB_ROOT(&head->tree);
}

//查找
RBTREE_ENTRY * rbtree_find(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	entry->head = head;
	return RB_FIND(RBTREE, &head->tree, entry);
}

//查找
RBTREE_ENTRY * rbtree_nfind(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	entry->head = head;
	return RB_NFIND(RBTREE, &head->tree, entry);
}

//上
RBTREE_ENTRY * rbtree_prev(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	return RB_PREV(RBTREE, &head->tree, entry);
}

//下
RBTREE_ENTRY * rbtree_next(RBTREE_HEADER *head, RBTREE_ENTRY *entry)
{
	return RB_NEXT(RBTREE, &head->tree, entry);
}

//小
RBTREE_ENTRY * rbtree_min(RBTREE_HEADER *head)
{
	return RB_MIN(RBTREE, &head->tree);
}

//大
RBTREE_ENTRY * rbtree_max(RBTREE_HEADER *head)
{
	return RB_MAX(RBTREE, &head->tree);
}

//总数
unsigned int rbtree_count(RBTREE_HEADER *head)
{
	return head->depth;
}
