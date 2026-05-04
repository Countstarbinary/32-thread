#define  _CRT_SECURE_NO_WARNINGS
#include "thread.h"
#include <windows.h>
#include <stdio.h>
#include "list.h"

CRITICAL_SECTION g_csGlobalVariable = { 0 };

PKTHREAD IdleThread = NULL;
PKTHREAD MainThread = NULL;
//倒序，优先级高的在低位。优先级低的在高位。方便查找
ULONG KiReadySummary = 0;

LIST_ENTRY DispatchThreadListEntry[32] = { 0 };
LIST_ENTRY WaitListHead = { 0 };
HANDLE_TABLE GlobalHandleTable = { 0 };


PKTHREAD GlobalCurrentThread = NULL;

VOID ThreadInit()
{
	//创建全局句柄表
	PUCHAR Table = (PUCHAR)malloc(0x1000);
	memset(Table, 0, 0x1000);

	GlobalHandleTable.GlobalTable = (ULONG)Table;

	//初始化就绪链表
	for (int i = 0; i < 32; i++)
	{
		PLIST_ENTRY lits = &DispatchThreadListEntry[i];
		InitializeListHead(lits);
	};

	InitializeListHead(&WaitListHead);
};

typedef ULONG(*ThreadStartRoutine)(void *lpParameter);

//typedef void(*func)(void *lpParameter);

void ThreadStartup(PKTHREAD thread)
{
	((ThreadStartRoutine)thread->Func)(thread->lpThreadParameter);

	return;
}

//借鉴滴水
void PushStack(unsigned int **Stackpp, unsigned int v)
{
	*Stackpp -= 1;
	**Stackpp = v;

	return;
}



PKTHREAD CreateThreadL(ThreadStartRoutine lpStartAddress, PVOID lpParameter, PULONG lpThreadId)
{
	PKTHREAD Thread = NULL;
	PUCHAR StackBase = NULL;
	do
	{
		//分配线程结构体内存，1个页。方便扩展
		Thread = (PKTHREAD)malloc(0x1000);
		if (Thread == NULL) break;


		memset((PUCHAR)Thread, 0, 0x1000);
		//分配线程堆栈
		StackBase = (PUCHAR)malloc(0x1000);
		if (Thread == NULL)break;

		memset((PUCHAR)StackBase, 0, 0x1000);

		//初始化赋值
		Thread->InitialStack = StackBase + 0x1000;

		Thread->StackLimit = StackBase;

		//Thread->KernelStack =(VOID*)((ULONG)Thread->InitialStack - sizeof(KTRAP_FRAME) - 0x44);

		Thread->StartRoutine = ThreadStartup;

		Thread->BasePriority = 0;

		Thread->Priority = 0;

		Thread->Quantum = 6;

		Thread->PriorityDecrement = 1;

		Thread->State = 0;

		Thread->ThreadId = SetTableHandle(Thread);

		if (Thread->ThreadId == -1)break;

		if (lpThreadId)
		{
			*lpThreadId = Thread->ThreadId;
		};
		

		/*
		push ds
		push es
		push fs
		push gs
		pushad
		pushf
		共0xd
		*/



		//入栈
		PULONG StackDWORDParam = (VOID*)((ULONG)Thread->InitialStack - sizeof(KTRAP_FRAME) - 0x44);
		//PushStack(&StackDWORDParam, (unsigned int)Thread);		//函数有两个参数
		//PushStack(&StackDWORDParam, (unsigned int)Thread);		//通过这个指针来找到：线程函数、函数参数
		//PushStack(&StackDWORDParam, (unsigned int)9);				//平衡堆栈
		//PushStack(&StackDWORDParam, (unsigned int)ThreadStartup);	//线程入口函数 这个函数负责调用线程函数
		//PushStack(&StackDWORDParam, 5);								//push ebp
		//PushStack(&StackDWORDParam, 0x202);								//push eflags
		//PushStack(&StackDWORDParam, 0);								//pushad
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//
		//PushStack(&StackDWORDParam, 0);								//

		//PushStack(&StackDWORDParam, 0);								//push ds
		//PushStack(&StackDWORDParam, 0);								//push es
		////PushStack(&StackDWORDParam, 0);								//push fs
		//PushStack(&StackDWORDParam, 0);							//push gs
		//Thread->KernelStack = StackDWORDParam;

		PushStack(&StackDWORDParam, (unsigned int)Thread);		//通过这个指针来找到：线程函数、函数参数
		PushStack(&StackDWORDParam, (unsigned int)9);				//平衡堆栈
		PushStack(&StackDWORDParam, (unsigned int)ThreadStartup);	//线程入口函数 这个函数负责调用线程函数
		PushStack(&StackDWORDParam, 5);								//push ebp
		PushStack(&StackDWORDParam, 7);								//push edi
		PushStack(&StackDWORDParam, 6);								//push esi
		PushStack(&StackDWORDParam, 3);								//push ebx
		PushStack(&StackDWORDParam, 2);								//push ecx
		PushStack(&StackDWORDParam, 1);								//push edx
		PushStack(&StackDWORDParam, 0);								//push eax
		Thread->KernelStack = StackDWORDParam;


		Thread->Func = lpStartAddress;
		Thread->lpThreadParameter = lpParameter;


		InitializeListHead(&Thread->ThreadListEntry);

		//插入对应的优先级链表
		InsertHeadList(&DispatchThreadListEntry[Thread->Priority], &Thread->ThreadListEntry);


		KiReadySummary |= 1 << (31 - Thread->Priority);

		return Thread;
	} while (0);
	if (Thread) free((PUCHAR)Thread);
	if (StackBase) free((PUCHAR)Thread);
	return NULL;
};


VOID SetThreadPriorityL(PKTHREAD Thread, UCHAR Priority)
{
	//后面需要进行同步操作
	Thread->Priority = Priority;
};

//查找下一个被调度的线程
PKTHREAD FindReadyThread()
{
	ULONG Priority = -1;
	ULONG TempKiReadySummary = 0;
	TempKiReadySummary = KiReadySummary;
	for (int i = 0; i < 32; i++)
	{
		if ((TempKiReadySummary & 1) == 1)
		{
			Priority = i;
			break;
		};
		TempKiReadySummary = TempKiReadySummary >> 1;
	};

	if (Priority == -1)return NULL;
	Priority = 0x1f - Priority;

	BOOL isEmpty = IsListEmpty(&DispatchThreadListEntry[Priority]);
	if (isEmpty)return NULL;

	PLIST_ENTRY list = RemoveHeadList(&DispatchThreadListEntry[Priority]);

	PKTHREAD thread = CONTAINING_RECORD(list, KTHREAD, ThreadListEntry);

	return thread;
};

//设置句柄表，返回对象id
ULONG SetTableHandle(PVOID object)
{
	for (int i = 0; i < 0x200; i++)
	{
		if (((PULONG64)GlobalHandleTable.GlobalTable)[i] == 0)
		{
			((PULONG64)GlobalHandleTable.GlobalTable)[i] = (ULONG)object;
			return i;
		};
	};

	return -1;
};




VOID KiSwapThread(PKTHREAD OldThread, PKTHREAD NewThread)
{
	//关中断
	//EnterCriticalSection(&g_csGlobalVariable);


	//切换线程上下文
	SwapContext(OldThread, NewThread);

	//恢复线程状态
	//LeaveCriticalSection(&g_csGlobalVariable);
};


__declspec(naked) void Swap(PKTHREAD OldThread, PKTHREAD NewThread)
{
	__asm
	{
		push ebp
		mov ebp, esp

		mov eax, [ebp + 8]   // eax = OldThread
		mov edx, [ebp + 0xc]   // edx = NewThread

		//sub esp,__LOCAL_SIZE
		push edi
		push esi
		push ebx
		push ecx
		push edx
		push eax

		mov esi, eax
		mov edi, edx

		mov[esi + 8], esp
		//---------------经典堆栈切换 另一个线程复活----------------------------------
		mov esp, [edi + 8]


		pop eax
		pop edx
		pop ecx
		pop ebx
		pop esi
		pop edi
		//add esp,__LOCAL_SIZE
		pop ebp
		ret
	}
}


//void __declspec(naked) Swap(PKTHREAD OldThread, PKTHREAD NewThread)
//{
//	__asm
//	{
//		push ebp
//		mov ebp,esp
//
//		mov eax, [ebp + 8]   // eax = OldThread
//		mov edx, [ebp + 0xc]   // edx = NewThread
//
//		pushfd
//		pushad
//		push ds
//		push es
//		//push fs
//		push gs
//		
//		
//		mov [eax + 8], esp	//KernelStack
//
//		mov esp, [edx + 8]
//
//		pop gs
//		//pop fs	//用户态不能改这个
//		pop es
//		pop ds
//		
//		popad
//		popfd
//		pop ebp
//
//		ret
//	};
//};

VOID SwapContext(PKTHREAD OldThread, PKTHREAD NewThread)
{
	ULONG NewEsp = (ULONG)NewThread->KernelStack;
	ULONG OldEsp = 0;

	GlobalCurrentThread = NewThread;

	Swap(OldThread, NewThread);
	//SwapNew(OldThread, NewThread);



};

PKTHREAD GetCurrentThreadL()
{
	return GlobalCurrentThread;
};

VOID ScheduleThread()
{
	PKTHREAD NewThread = FindReadyThread();
	PKTHREAD CurrentThread = GetCurrentThreadL();

	if (NewThread == NULL || CurrentThread == NULL)return;

	if (NewThread == CurrentThread)
	{
		if (NewThread == IdleThread)
		{
			//PUCHAR str = "IsIdleThread==================\n";
			//putstr(str);
			//InsertHeadList(&DispatchThreadListEntry[CurrentThread->Priority], &CurrentThread->ThreadListEntry);
			//KiReadySummary |= 1 << (31 - CurrentThread->Priority);
			//while (1);
			//return;
		}
		else if (NewThread == MainThread)
		{
			PUCHAR str = "MainThread\n";
			printf(str);
			//RemoveEntryList(&CurrentThread->ThreadListEntry);
			//if (IsListEmpty(&DispatchThreadListEntry[CurrentThread->Priority]))
			//	KiReadySummary &= ~(1 << (31 - CurrentThread->Priority));

			while (1);

			//return;
		}
		else
		{
			PUCHAR str = "current thread wrong\n";
			printf(str);
			while (1);
		};;

	};

	//旧插入对应的优先级链表
	if (CurrentThread->State != Wait)
	{
		//旧插入对应的优先级链表
		InsertTailList(&DispatchThreadListEntry[CurrentThread->Priority], &CurrentThread->ThreadListEntry);
		KiReadySummary |= 1 << (31 - CurrentThread->Priority);
	};

	KiSwapThread(CurrentThread, NewThread);


};

BOOL InterruptTimer(UCHAR vector)
{
	PKTHREAD Thread = GetCurrentThreadL();
	if (Thread == NULL) return 1;

	PLIST_ENTRY entry = WaitListHead.Flink;

	//处理睡眠线程
	while (entry != &WaitListHead)
	{

		PLIST_ENTRY next = entry->Flink;          // ★ 提前保存下一个节点
		PKTHREAD thread = CONTAINING_RECORD(entry, KTHREAD, WaitListEntry);

		if (thread->SleepTick > 0) {
			thread->SleepTick--;
		}
		else {
			printf("WAKE UP\n");
			RemoveEntryList(entry);
			InsertTailList(&DispatchThreadListEntry[thread->Priority], &thread->ThreadListEntry);
			thread->State = Ready;
			// 维护就绪位图（重要！）
			KiReadySummary |= 1 << (31 - thread->Priority);
		}
		entry = next;   // 使用提前保存的下一个节点
	}


	if (Thread->Quantum == 0)
	{
		if (Thread->Priority > 1)
		{
			Thread->Priority--;
		};
		Thread->Quantum = 6;
		ScheduleThread();
	}
	else
	{
		Thread->Quantum--;
	};

};



//DWORD WINAPI thread0(LPVOID lpThreadParameter)
//{
//	while (1)
//	{
//		printf("000000000000000000000");
//	};
//
//	return 0;
//};

VOID SleepLT(ULONG time)
{
	PKTHREAD Thread = GetCurrentThreadL();


	//时钟中断频率100Hz/s->1 tick = 10ms
	Thread->SleepTick = time / 10;


	Thread->State = Wait;


	InsertTailList(&WaitListHead, &Thread->WaitListEntry);

	ScheduleThread();
};

ULONG thread0()
{
	while (1)
	{
		//vmmprint(_SELF, __LINE__, "Thread0 \n");
		//OutputDebugStringA("Thread0 \n");
		printf("Thread0......................\n");
		//SleepLT(100);
		ScheduleThread();
	};
	return 0;
};

ULONG thread1()
{
	while (1)
	{
		//vmmprint(_SELF, __LINE__, "Thread1 \n");
		//OutputDebugStringA("Thread1 \n");
		printf("Thread1......................\n");
		//SleepLT(100);
		ScheduleThread();
		
	};
	return 0;
};
ULONG thread2()
{
	while (1)
	{
		//vmmprint(_SELF, __LINE__, "Thread2 \n");
		//OutputDebugStringA("Thread2 \n");
		printf("Thread2......................\n");
		//SleepLT(100);
		ScheduleThread();
		
	};
	return 0;
};
ULONG thread3()
{
	while (1)
	{
		//vmmprint(_SELF, __LINE__, "Thread3 \n");
		//OutputDebugStringA("Thread3 \n");
		printf("Thread3......................\n");
		//SleepLT(100);
		ScheduleThread();
		
	};
	return 0;
};

//空闲线程
ULONG IdleThreadFunc(void *lpParameter)
{
	while (1)
	{
		printf("Idle......................\n");

		ScheduleThread();   // 主动让权

	};

	return 0;
};

__declspec(naked) ULONG GetEip()
{
	__asm 
	{
		mov eax, [esp]
		ret;
	}
}

__declspec(naked) ULONG GetEsp()
{
	__asm
	{
		mov eax,esp
		add eax,4
		ret
	};
	
}

void CreateSystemMainThread(PVOID kmain)
{
	MainThread = CreateThreadL(kmain, NULL, NULL);
	RemoveEntryList(&MainThread->ThreadListEntry);
	MainThread->State = Running;
	MainThread->StartRoutine = kmain;
	MainThread->KernelStack = 0;
	
};





int main()
{
	ULONG Tid0 = 0;
	ULONG Tid1 = 0;
	ULONG Tid2 = 0;
	ULONG Tid3 = 0;

	ThreadInit();
	CreateSystemMainThread(main);
	PKTHREAD thread = CreateThreadL(thread0, NULL, &Tid1);
	
	GlobalCurrentThread = MainThread;

	IdleThread = CreateThreadL(IdleThreadFunc, NULL, NULL);
	//
	//thread = CreateThreadL(thread1, NULL, &Tid1);
	//
	//
	//thread = CreateThreadL(thread2, NULL, &Tid2);
	//
	//thread = CreateThreadL(thread3, NULL, &Tid3);
	InitializeCriticalSection(&g_csGlobalVariable);


	while (1)
	{
		InterruptTimer(0x20);
		//SleepLT(100);
		printf("main......................\n");
	};
};