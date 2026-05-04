#pragma once
#include <windows.h>







void InitializeListHead(PLIST_ENTRY ListHead);

int IsListEmpty(PLIST_ENTRY ListHead);

void InsertHeadList(PLIST_ENTRY Head, PLIST_ENTRY Entry);

void InsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry);

void InsertAfter(PLIST_ENTRY Pos, PLIST_ENTRY Entry);

void InsertBefore(PLIST_ENTRY Pos, PLIST_ENTRY Entry);

void RemoveEntryList(PLIST_ENTRY Entry);

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY Head);

PLIST_ENTRY RemoveTailList(PLIST_ENTRY Head);
