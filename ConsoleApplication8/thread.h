#pragma once

#include <windows.h>

enum  status
{
	Running,
	Ready,
	Wait,
	Create,
	TERMINATED,
};

typedef struct _KTHREAD
{
	VOID* InitialStack;          //线程内核栈起始位置，高地址。                
	VOID* StackLimit;            //线程内核栈低地址               
	VOID* KernelStack;           //线程内核栈，eps0   
	VOID* lpThreadParameter;	//线程参数
	VOID* Func;	//线程参数
	UCHAR State;
	CHAR Priority;
	CHAR BasePriority;
	CHAR PriorityDecrement;
	CHAR Quantum;
	VOID* ServiceTable;
	UCHAR Preempted;
	struct _LIST_ENTRY WaitListEntry;
	ULONG SleepTick;
	struct _KTRAP_FRAME* TrapFrame;
	CHAR PreviousMode;
	struct _LIST_ENTRY ThreadListEntry;
	ULONG ThreadId;
	VOID* StartRoutine;			//初始运行函数的地址
}KTHREAD, *PKTHREAD;

//0x8c
typedef struct _KTRAP_FRAME
{
	ULONG DbgEbp;                                                           //0x0
	ULONG DbgEip;                                                           //0x4
	ULONG DbgArgMark;                                                       //0x8
	ULONG DbgArgPointer;                                                    //0xc
	ULONG TempSegCs;                                                        //0x10
	ULONG TempEsp;                                                          //0x14
	ULONG Dr0;                                                              //0x18
	ULONG Dr1;                                                              //0x1c
	ULONG Dr2;                                                              //0x20
	ULONG Dr3;                                                              //0x24
	ULONG Dr6;                                                              //0x28
	ULONG Dr7;                                                              //0x2c
	ULONG SegGs;                                                            //0x30
	ULONG SegEs;                                                            //0x34
	ULONG SegDs;                                                            //0x38
	ULONG Edx;                                                              //0x3c
	ULONG Ecx;                                                              //0x40
	ULONG Eax;                                                              //0x44
	ULONG PreviousPreviousMode;                                             //0x48
	struct _EXCEPTION_REGISTRATION_RECORD* ExceptionList;                   //0x4c
	ULONG SegFs;                                                            //0x50
	ULONG Edi;                                                              //0x54
	ULONG Esi;                                                              //0x58
	ULONG Ebx;                                                              //0x5c
	ULONG Ebp;                                                              //0x60
	ULONG ErrCode;                                                          //0x64
	ULONG Eip;                                                              //0x68
	ULONG SegCs;                                                            //0x6c
	ULONG EFlags;                                                           //0x70
	ULONG HardwareEsp;                                                      //0x74
	ULONG HardwareSegSs;                                                    //0x78
	ULONG V86Es;                                                            //0x7c
	ULONG V86Ds;                                                            //0x80
	ULONG V86Fs;                                                            //0x84
	ULONG V86Gs;                                                            //0x88
}KTRAP_FRAME, *PKTRAP_FRAME;


typedef struct _HANDLE_TABLE
{
	ULONG GlobalTable;

	ULONG NextHandleNeedingPool;
	LONG HandleCount;

}HANDLE_TABLE, *PHANDLE_TABLE;

typedef struct _HANDLE_TABLE_ENTRY
{
	PVOID Object;      // ⭐ 指向对象（带 flag）

	ULONG GrantedAccess;

} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

//32个优先级的就绪链表
extern LIST_ENTRY DispatchThreadListEntry[32];

extern HANDLE_TABLE GlobalTable;

PKTHREAD GetCurrentThreadL();

VOID ScheduleThread();

ULONG SetTableHandle(PVOID object);

VOID SwapContext(PKTHREAD OldThread, PKTHREAD NewThread);