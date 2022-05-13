#include <ddk/ntddk.h>

#ifndef CREATE_METHOD_H
#define CREATE_METHOD_H

enum
{
	STRINGTYPE = 0,
	MEMORYTYPE,
	IOPORTTYPE,
	NONETYPE
};

// Common
typedef struct _CONTEXT_FOR_COMMON
{
	int ContextType; // 어떤 Context인지를 구분하기 위해서 정의합니다
}CONTEXT_FOR_COMMON, *PCONTEXT_FOR_COMMON;

// for STRING
typedef struct _CONTEXT_FOR_STRING
{
	int ContextType; // 어떤 Context인지를 구분하기 위해서 정의합니다
#define MAXSTRINGBUFFERCOUNT 100
	char *StringBuffer;
	int CurrentRemainBufferSize;
}CONTEXT_FOR_STRING, *PCONTEXT_FOR_STRING;

// for MEMORY
typedef struct _CONTEXT_FOR_MEMORY
{
	int ContextType; // 어떤 Context인지를 구분하기 위해서 정의합니다
	PVOID KernelLevelVirtualAddress; 
	PVOID UserLevelVirtualAddress; 
	int VirtualAddressMappedSize; 
	PMDL Mdl; 
}CONTEXT_FOR_MEMORY, *PCONTEXT_FOR_MEMORY;

typedef struct
{
	unsigned char * pPhysicalAddress;
	unsigned long dwMemorySize;
}WRITE_PARAMETER, *PWRITE_PARAMETER;

// for IOPORT
typedef struct _CONTEXT_FOR_IOPORT
{
	int ContextType; // 어떤 Context인지를 구분하기 위해서 정의합니다
	PKEVENT pUserEventObject;
	HANDLE UserEventHandle;
	HANDLE ProcessId;
}CONTEXT_FOR_IOPORT, *PCONTEXT_FOR_IOPORT;

int GetTypeCreate( PFILE_OBJECT FileObject );
PVOID AllocateStructForFileContext( PFILE_OBJECT FileObject );
void FreeStructForFileContext( PFILE_OBJECT FileObject );
void _memorytype_flush_context( PCONTEXT_FOR_MEMORY pContextForMemory );
void _memorytype_mapping_context( PCONTEXT_FOR_MEMORY pContextForMemory, PWRITE_PARAMETER pWriteParameter );

#endif //CREATE_METHOD_H