#include "createmethod.h"

int GetTypeCreate( PFILE_OBJECT FileObject )
{
	if( !FileObject->FileName.Length )
	{
		return NONETYPE;
	}

	if( !memcmp( L"\\STRING", 
				FileObject->FileName.Buffer, 
				FileObject->FileName.Length ) )
	{
		return STRINGTYPE;
	}

	if( !memcmp( L"\\MEMORY", 
				FileObject->FileName.Buffer, 
				FileObject->FileName.Length ) )
	{
		return MEMORYTYPE;
	}

	if( !memcmp( L"\\IOPORT", 
				FileObject->FileName.Buffer, 
				FileObject->FileName.Length ) )
	{
		return IOPORTTYPE;
	}

	return NONETYPE;
}

PVOID AllocateStructForFileContext( PFILE_OBJECT FileObject )
{
	int Type;

	Type = GetTypeCreate( FileObject );
	if( Type == NONETYPE )
		return NULL;

	switch( Type )
	{
	case STRINGTYPE:
		{
			PCONTEXT_FOR_STRING pContextForString;
			FileObject->FsContext = ExAllocatePool( 
				NonPagedPool, 
				sizeof(CONTEXT_FOR_STRING) );
			pContextForString = (PCONTEXT_FOR_STRING)FileObject->FsContext;
			pContextForString->ContextType = STRINGTYPE;
			pContextForString->StringBuffer = (char *)ExAllocatePool( NonPagedPool, MAXSTRINGBUFFERCOUNT + 1 );
			pContextForString->CurrentRemainBufferSize = 0;
			return FileObject->FsContext;
		}
	case MEMORYTYPE:
		{
			PCONTEXT_FOR_MEMORY pContextForMemory;

			FileObject->FsContext = ExAllocatePool( 
				NonPagedPool, 
				sizeof(CONTEXT_FOR_MEMORY) );

			pContextForMemory = (PCONTEXT_FOR_MEMORY)FileObject->FsContext;
			pContextForMemory->ContextType = MEMORYTYPE;
			
			pContextForMemory->KernelLevelVirtualAddress = 0;
			pContextForMemory->Mdl = 0;
			pContextForMemory->UserLevelVirtualAddress = 0;
			pContextForMemory->VirtualAddressMappedSize = 0;
			return FileObject->FsContext;
		}
	case IOPORTTYPE:
		{
			PCONTEXT_FOR_IOPORT pContextForIoPort;

			FileObject->FsContext = ExAllocatePool( 
				NonPagedPool, 
				sizeof(CONTEXT_FOR_IOPORT) );

			pContextForIoPort = (PCONTEXT_FOR_IOPORT)FileObject->FsContext;
			pContextForIoPort->ContextType = IOPORTTYPE;
			pContextForIoPort->pUserEventObject = 0;
			pContextForIoPort->UserEventHandle = 0;
			pContextForIoPort->ProcessId = 0;

			return FileObject->FsContext;
		}
	default:
		return NULL;
	}
}

/* ����ϴ� User Mapped�� ����޸𸮸� �����ϴ� ������ ������ �����ϴ�.
1. UserMapped �����ּҸ� �����մϴ�
2. ����� Mdl�� �����մϴ�
3. Mdl�� Source�� ���� �޸������� ���� Ŀ�ΰ����ּҸ� �����մϴ�
*/
void _memorytype_flush_context( PCONTEXT_FOR_MEMORY pContextForMemory )
{
	if( pContextForMemory->VirtualAddressMappedSize )
	{
		MmUnmapLockedPages( pContextForMemory->UserLevelVirtualAddress,
			pContextForMemory->Mdl );
		IoFreeMdl( pContextForMemory->Mdl );
		MmUnmapIoSpace( pContextForMemory->KernelLevelVirtualAddress,
			pContextForMemory->VirtualAddressMappedSize );
	}
	pContextForMemory->VirtualAddressMappedSize = 0;
}

/* ���ο� ����޸𸮸��������� ������ �����ϴ�.
1. ����ڰ� ������ �����޸𸮸� �����ϴ� Ŀ�ΰ����ּҸ� �غ��մϴ�
2. MDL�� �����մϴ�. �̶�, MDL�� Ŀ�ΰ����ּҸ� ����Ű���� �ʱ�ȭ �մϴ�
3. �غ�� MDL�� ����ؼ�, ��������α׷��� ���� ������ �����ּҸ� �����մϴ�
*/
void _memorytype_mapping_context( PCONTEXT_FOR_MEMORY pContextForMemory, PWRITE_PARAMETER pWriteParameter )
{
	PHYSICAL_ADDRESS PhysicalAddress;
	PhysicalAddress.u.HighPart = 0;
	PhysicalAddress.u.LowPart = (ULONG)pWriteParameter->pPhysicalAddress;
	
	// 1.
	pContextForMemory->KernelLevelVirtualAddress = (unsigned char *)MmMapIoSpace( PhysicalAddress, pWriteParameter->dwMemorySize, MmNonCached );

	// 2.
	pContextForMemory->Mdl = IoAllocateMdl( pContextForMemory->KernelLevelVirtualAddress, pWriteParameter->dwMemorySize, FALSE, FALSE, NULL );
	MmBuildMdlForNonPagedPool( pContextForMemory->Mdl );

	// 3.
	pContextForMemory->UserLevelVirtualAddress = (unsigned char *)MmMapLockedPages( pContextForMemory->Mdl, UserMode );
	pContextForMemory->VirtualAddressMappedSize = pWriteParameter->dwMemorySize;
}

void FreeStructForFileContext( PFILE_OBJECT FileObject )
{
	PCONTEXT_FOR_COMMON pContext;

	pContext = (PCONTEXT_FOR_COMMON)FileObject->FsContext;

	if( !pContext )
		return;

	switch( pContext->ContextType )
	{
	case STRINGTYPE:
		{
			PCONTEXT_FOR_STRING pContextForString;
			pContextForString = (PCONTEXT_FOR_STRING)pContext;
			if( pContextForString->StringBuffer )
			{
				ExFreePool(pContextForString->StringBuffer);
			}
		}
		break;
	case MEMORYTYPE:
		{
			PCONTEXT_FOR_MEMORY pContextForMemory;
			pContextForMemory = (PCONTEXT_FOR_MEMORY)pContext;
			_memorytype_flush_context( pContextForMemory );
		}
		break;
	case IOPORTTYPE:
		{
			PCONTEXT_FOR_IOPORT pContextForIoPort;
			pContextForIoPort = (PCONTEXT_FOR_IOPORT)pContext;
			if( pContextForIoPort->pUserEventObject )
			{
				KeSetEvent( pContextForIoPort->pUserEventObject, 0, FALSE );
				ObDereferenceObject( pContextForIoPort->pUserEventObject );
				// EventObject�� ����ȸ���� ���ҽ�ŵ�ϴ�
			}
		}
		break;
	default:
		return;
	}
	ExFreePool(pContext);
}