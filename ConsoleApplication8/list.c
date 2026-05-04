#include <windows.h>
#include "list.h"



//初始化
void InitializeListHead(PLIST_ENTRY ListHead)
{
	ListHead->Flink = ListHead;
	ListHead->Blink = ListHead;
}

//判断是否为空
int IsListEmpty(PLIST_ENTRY ListHead)
{
	return (ListHead->Flink == ListHead);
}

//头插
void InsertHeadList(PLIST_ENTRY Head, PLIST_ENTRY Entry)
{
	PLIST_ENTRY First = Head->Flink;

	Entry->Flink = First;
	Entry->Blink = Head;

	First->Blink = Entry;
	Head->Flink = Entry;

}

//尾插
void InsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry)
{
	PLIST_ENTRY Last = Head->Blink;

	Entry->Flink = Head;
	Entry->Blink = Last;

	Last->Flink = Entry;
	Head->Blink = Entry;
}

//插入到指定节点之后
void InsertAfter(PLIST_ENTRY Pos, PLIST_ENTRY Entry)
{
	PLIST_ENTRY Next = Pos->Flink;

	Entry->Flink = Next;
	Entry->Blink = Pos;

	Pos->Flink = Entry;
	Next->Blink = Entry;
}

//插入到指定节点之前
void InsertBefore(PLIST_ENTRY Pos, PLIST_ENTRY Entry)
{
	PLIST_ENTRY Prev = Pos->Blink;

	Entry->Flink = Pos;
	Entry->Blink = Prev;

	Prev->Flink = Entry;
	Pos->Blink = Entry;
}

//删除任意节点
void RemoveEntryList(PLIST_ENTRY Entry)
{
	PLIST_ENTRY Prev = Entry->Blink;
	PLIST_ENTRY Next = Entry->Flink;

	Prev->Flink = Next;
	Next->Blink = Prev;

	// 可选：清空，防止野指针
	Entry->Flink = Entry->Blink = NULL;
}

//弹出头节点
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY Head)
{
	PLIST_ENTRY First = Head->Flink;
	RemoveEntryList(First);
	return First;
}

//弹出尾节点
PLIST_ENTRY RemoveTailList(PLIST_ENTRY Head)
{
	PLIST_ENTRY Last = Head->Blink;
	RemoveEntryList(Last);
	return Last;
}