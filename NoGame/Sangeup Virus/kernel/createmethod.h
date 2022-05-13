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
	int ContextType; // � Context������ �����ϱ� ���ؼ� �����մϴ�
}CONTEXT_FOR_COMMON, *PCONTEXT_FOR_COMMON;

// for STRING
typedef struct _CONTEXT_FOR_STRING
{
	int ContextType; // � Context������ �����ϱ� ���ؼ� �����մϴ�
#define MAXSTRINGBUFFERCOUNT 100
	char *StringBuffer;
	int CurrentRemainBufferSize;
}CONTEXT_FOR_STRING, *PCONTEXT_FOR_STRING;

// for MEMORY
typedef struct _CONTEXT_FOR_MEMORY
{
	int ContextType; // � Context������ �����ϱ� ���ؼ� �����մϴ�
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
	int ContextType; // � Context������ �����ϱ� ���ؼ� �����մϴ�
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