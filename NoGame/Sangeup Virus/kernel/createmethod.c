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

/* 사용하던 User Mapped된 가상메모리를 해제하는 순서는 다음과 같습니다.
1. UserMapped 가상주소를 해제합니다
2. 사용한 Mdl을 해제합니다
3. Mdl의 Source로 사용된 메모리접근을 위한 커널가상주소를 해제합니다
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

/* 새로운 가상메모리매핑절차는 다음과 같습니다.
1. 사용자가 보내준 물리메모리를 접근하는 커널가상주소를 준비합니다
2. MDL을 생성합니다. 이때, MDL은 커널가상주소를 가리키도록 초기화 합니다
3. 준비된 MDL을 사용해서, 사용자프로그램이 접근 가능한 가상주소를 매핑합니다
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
				// EventObject의 참조회수를 감소시킵니다
			}
		}
		break;
	default:
		return;
	}
	ExFreePool(pContext);
}