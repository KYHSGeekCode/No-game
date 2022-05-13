/*
	SAMPLE5드라이버의 동작을 이해하려면, 다음 시나리오를 먼저 이해해야 합니다

	응용프로그램 -> DeviceIoControl() -> SAMPLE5_DeviceIoControl() -> System's Process WorkItem Routine ->
	System's Process System Thread Routine

	이후, System's Process System Thread Routine은 스스로의 문맥을 응용프로그램문맥으로 바꿔치기 합니다
	그런다음, 응용프로그램이 보내준 이벤트핸들을 이벤트Object로 변환합니다.
	이후, 원래대로 System's Process System Thread Routine으로 문맥을 환원합니다

	이와 같은 부분을 반드시 이해해야 소스를 파악할 수 있습니다
*/

// SAMPLE5드라이버는 드라이버측에서 사용자프로그램측으로 사건을 알려주는 방법과 응용프로그램측에서 자유롭게 IOPORT를 접근하는 방법을 알려줍니다

#include <ddk/ntddk.h>        /* required for WDM driver development */
#include <stdarg.h>     /* standard io include files */
#include <stdio.h>

//#include <devioctl.h>
#include <initguid.h>

DEFINE_GUID(GUID_SAMPLE, 
0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
// 현재 우리가 사용될 DeviceStack에 대한 Interface GUID를 정의합니다

//// 지금 소스는 ntddk.h를 사용하고 있기 때문에 추가정의하지 않으면 사용할 수 없다
// NTIFS.H 에서 정의되어있는 함수원형과 자료구조들
void Ke386SetIoAccessMap( int, PVOID );
void Ke386IoSetAccessProcess( PEPROCESS, int );
NTSTATUS PsLookupProcessByProcessId( HANDLE, PEPROCESS * );
VOID KeStackAttachProcess( PEPROCESS, PVOID );
VOID KeUnstackDetachProcess( PVOID );
typedef struct
{
	char Reserved[32];
}KAPC_STATE, *PKAPC_STATE;
////

////
// 응용프로그램과 주고받을 ControlCode를 정의합니다
#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct
{
	HANDLE UserEventHandle;
	HANDLE ProcessId;
}EVENT_INFORMATION, *PEVENT_INFORMATION;
// 응용프로그램측에서 전달된 데이터구조체

#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)
////

typedef struct
{
	ULONG dwIoControlCode;
	PEPROCESS Process;
	PVOID Context;
}THREAD_CONTEXT, *PTHREAD_CONTEXT;
// System Thread루틴측으로 보낼 Context

typedef struct
{
	ULONG dwIoControlCode;
	PIO_WORKITEM pIoWorkItem;
	PVOID Context;
}WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;
// WorkItem루틴측으로 보낼 Context

#pragma optimize("", off)
#include "SAMPLE5.h"    
#include "createmethod.h"

char IOPM[0x2000];

void IrpStringOut( PIRP Irp ) // IRP를 디버그프린트합니다
{
	KIRQL Irql;
	PIO_STACK_LOCATION pStack;
	
	pStack = IoGetCurrentIrpStackLocation( Irp );

	switch( pStack->MajorFunction )
	{
	case IRP_MJ_CREATE:
		DbgPrint("[IRP_MJ_CREATE]");
		break;
	case IRP_MJ_READ:
		DbgPrint("[IRP_MJ_READ]");
		break;
	case IRP_MJ_WRITE:
		DbgPrint("[IRP_MJ_WRITE]");
		break;
	case IRP_MJ_CLEANUP:
		DbgPrint("[IRP_MJ_CLEANUP]");
		break;
	case IRP_MJ_CLOSE:
		DbgPrint("[IRP_MJ_CLOSE]");
		break;
	case IRP_MJ_DEVICE_CONTROL:
		DbgPrint("[IRP_MJ_DEVICE_CONTROL]");
		DbgPrint("ControlCode = %8X", pStack->Parameters.DeviceIoControl.IoControlCode);
		break;
	case IRP_MJ_PNP:
		DbgPrint("[IRP_MJ_PNP]");
		switch( pStack->MinorFunction )
		{
			case IRP_MN_START_DEVICE:
				DbgPrint("IRP_MN_START_DEVICE");
				break;
			case IRP_MN_REMOVE_DEVICE:
				DbgPrint("IRP_MN_REMOVE_DEVICE");
				break;
			case IRP_MN_SURPRISE_REMOVAL:
				DbgPrint("IRP_MN_SURPRISE_REMOVAL");
				break;
			default:
				DbgPrint("ANY MN FUNCTION");
				break;
		}
		break;
	case IRP_MJ_POWER:
		DbgPrint("[IRP_MJ_POWER]");
		switch( pStack->MinorFunction )
		{
			case IRP_MN_SET_POWER:
				DbgPrint("IRP_MN_SET_POWER");
				break;
			case IRP_MN_QUERY_POWER:
				DbgPrint("IRP_MN_QUERY_POWER");
				break;
			default:
				DbgPrint("ANY MN FUNCTION");
				break;
		}
		break;
	default:
		DbgPrint("[ANY IRP] = 0x%8X\n", Irp );
		return;
	}
	DbgPrint("\n");
}

NTSTATUS                                                            
DriverEntry                                                         
	(                                                               
	IN PDRIVER_OBJECT DriverObject,             /* pointer to the device object */
	IN PUNICODE_STRING RegistryPath             /* pointer to a unicode string representing the path */
												/*    to the drivers specific key in the registry    */
	)// 드라이버가 메모리에 처음상주하는 경우에만 한번 호출됩니다
{
	NTSTATUS returnStatus = STATUS_SUCCESS;     /* stores status to be returned */

	DbgPrint("DriverEntry ++ \n" );

	DriverObject->DriverUnload = SAMPLE5_Unload;
	DriverObject->DriverExtension->AddDevice = SAMPLE5_AddDevice;
	DriverObject->MajorFunction[IRP_MJ_PNP] = SAMPLE5_PnpDispatch;
	DriverObject->MajorFunction[IRP_MJ_POWER] = SAMPLE5_PowerDispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = SAMPLE5_Create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SAMPLE5_Close;
	DriverObject->MajorFunction[IRP_MJ_READ] = SAMPLE5_Read;
	// Win32 API ReadFile();
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SAMPLE5_Write;
	// Win32 API WriteFile();
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SAMPLE5_Cleanup;
	// Win32 API CloseHandle();
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SAMPLE5_DeviceIoControl;
	// Win32 API DeviceIoControl();

	DbgPrint("DriverEntry -- \n" );
	return returnStatus;
}

NTSTATUS
SAMPLE5_AddDevice
	(
	IN PDRIVER_OBJECT DriverObject,         
	IN PDEVICE_OBJECT PhysicalDeviceObject  
	)// PNPManager는 이곳을 통해서 IRP DeviceStack을 구성하라는 요청을 합니다
{
	/* IRP DeviceStack을 구성하기 위해서는 몇가지 단계를 거치게 됩니다 
	1. DeviceObject생성작업
	2. 현존하는 DeviceStack위로 DeviceObject를 올리기
	3. DeviceObject의 Flag값 변경하기
	*/
	PDEVICE_EXTENSION deviceExtension;              
	NTSTATUS returnStatus = STATUS_SUCCESS;         
	PDEVICE_OBJECT DeviceObject = NULL;             
	ULONG dwRet;

	DbgPrint("SAMPLE5_AddDevice ++ \n" );

	returnStatus = IoCreateDevice                                       
					(                                               
						DriverObject,                               
						sizeof ( DEVICE_EXTENSION ),                
						NULL,                   
						FILE_DEVICE_UNKNOWN,
						FILE_AUTOGENERATED_DEVICE_NAME,                                          
						FALSE,                                      
						&DeviceObject       
					);// DeviceObject를 생성합니다

	if( !NT_SUCCESS( returnStatus ) )
	{
		DbgPrint("IoCreateDevice() Error, Status = %8X\n", returnStatus );
		goto SAMPLE5_AddDevice_Exit;
	}

	deviceExtension = DeviceObject->DeviceExtension;

	RtlZeroMemory( deviceExtension, sizeof(DEVICE_EXTENSION));

	deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
	
	deviceExtension->DeviceObject = DeviceObject;

	deviceExtension->NextLayerDeviceObject =
	IoAttachDeviceToDeviceStack (
							DeviceObject,
							PhysicalDeviceObject	
	);// 현존하는 DeviceStack위로 DeviceObject를 올립니다

	DeviceObject->Flags |= deviceExtension->NextLayerDeviceObject->Flags & 
							( DO_POWER_PAGABLE  | DO_POWER_INRUSH); // DeviceObject의 Flag를 변경합니다

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING; // DeviceObject의 Flag를 변경합니다

	DeviceObject->Flags |= DO_BUFFERED_IO; // IRP의 SystemBuffer를 사용해서 응용프로그램버퍼와 연결될것임을 암시

	IoRegisterDeviceInterface( PhysicalDeviceObject, &GUID_SAMPLE, NULL, &deviceExtension->UnicodeString );
	// 현재 우리가 속한 DeviceStack에 대해 InterfaceGuid를 등록합니다
	// 리턴되는 de->UnicodeString는 사용자가 접근하는 이름입니다


SAMPLE5_AddDevice_Exit:
	DbgPrint("SAMPLE5_AddDevice -- \n" );
	return returnStatus;
}

VOID                                                                
SAMPLE5_Unload                                                       
	(                                                               
	IN PDRIVER_OBJECT DriverObject  /* pointer to device object */
	)
{// 더이상 드라이버가 관여할 DeviceObject가 없으면 이곳이 호출된후, 드라이버는 메모리에서 제거됩니다

	DbgPrint("SAMPLE5_Unload ++ \n" );

	// Do job..

	DbgPrint("SAMPLE5_Unload -- \n" );
}

NTSTATUS
SAMPLE5_DeferIrpCompletion
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to a device object */
	IN PIRP Irp,                        /* pointer to a I/O request packet */
	IN PVOID Context                    /* driver defined context */
	)// 명령어 IRP가 NextLayer 드라이버에서 IoCompleteRequest()함수에 의해서 종결요청되는 경우에 호출됩니다
{
	PKEVENT event = Context;    /* kernel event passed in Context */

	DbgPrint("SAMPLE5_DeferIrpCompletion ++ \n" );

	KeSetEvent(event,
			   1,
			   FALSE);

	
	DbgPrint("SAMPLE5_DeferIrpCompletion -- \n" );

	return STATUS_MORE_PROCESSING_REQUIRED; // IoCompleteRequest()함수로 하여금, 현재 IRP의 종결과정을 중단하라는 요청을 합니다
}

NTSTATUS
SAMPLE5_PnpDispatch
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	)
{// PNPManager가 보내는 IRP를 처리하는 처리기
	PIO_STACK_LOCATION pStack;                /* pointer to IRP stack */
	PDEVICE_EXTENSION deviceExtension;          /* pointer to device extention */
	NTSTATUS returnStatus = STATUS_SUCCESS;     /* stores status to be returned */
	PDEVICE_OBJECT NextLayerDeviceObject;           /* pointer to the stack device object */
	
	DbgPrint("SAMPLE5_PnpDispatch ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation ( Irp );
	deviceExtension = DeviceObject->DeviceExtension;
	NextLayerDeviceObject = deviceExtension->NextLayerDeviceObject;
			
	switch ( pStack->MinorFunction )
	{
		case IRP_MN_START_DEVICE :
		{// DeviceStack을 구동하라는 허가를 의미합니다
			KEVENT event;   /* event used to wait for an operation to complete */
			KeInitializeEvent(&event, NotificationEvent, FALSE);
			IoCopyCurrentIrpStackLocationToNext(Irp);  
			IoSetCompletionRoutine(Irp,
								   SAMPLE5_DeferIrpCompletion,
								   &event,
								   TRUE,
								   TRUE,
								   TRUE); // SAMPLE5_DeferIrpCompletion()함수가 현재 IRP에 대해 Next Layer Driver가 호출하는
										  // IoCompleteRequest()함수에 의해서 호출되도록 유도합니다
			
			returnStatus = IoCallDriver(NextLayerDeviceObject,Irp); // NextLayer Driver에게 이 명령을 전달한뒤, 결과를 확인합니다
			if (returnStatus == STATUS_PENDING) 
			{
				KeWaitForSingleObject(
					&event,
					Executive,
					KernelMode,
					FALSE,
					NULL);
				returnStatus = Irp->IoStatus.Status;
			}
			if( !NT_SUCCESS( returnStatus ) )
			{
				DbgPrint("Start Device Fail\n" );
				IoCompleteRequest( Irp, IO_NO_INCREMENT );
				goto SAMPLE5_PnpDispatch_Exit;
			}
			// NextLayer Driver가 이 명령을 성공적으로 다루었으면, 우리도 무엇인가를 해도 좋다는 의미입니다

			// Do Job...
			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, TRUE ); 
			// 사용자가 우리의 DeviceStack에 접근하는 것을 허용

			IoCompleteRequest( Irp, IO_NO_INCREMENT ); // 현재 IRP를 종결요청합니다
			// SAMPLE5_DeferIrpCompletion()함수에서 우리는 IRP에 대한 Next Layer Driver가 호출한 IoCompleteRequest()에 대해
		}	// 무시하도록 "STATUS_MORE_PROCESSING_REQUIRED"리턴을 시켰기때문에, 우리가 종결요청을 해야합니다
		break;

		case IRP_MN_SURPRISE_REMOVAL :
		{ // 갑자기 장치가 PC에서 제거되었다는 의미에서 전달됩니다

			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, FALSE );
			// 사용자가 우리의 DeviceStack에 접근하는 것을 금지

			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation( Irp ); // 현재 IRP는 이후 종결될때까지 더이상 간섭을 하지 않겠다는 의미입니다
			returnStatus = IoCallDriver(NextLayerDeviceObject, Irp);
		}
		break;

		case IRP_MN_REMOVE_DEVICE :
		{ // IRP DeviceStack을 해체하라는 요청의 의미로 전달됩니다
			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, FALSE );
			// 사용자가 우리의 DeviceStack에 접근하는 것을 금지

			RtlFreeUnicodeString( &deviceExtension->UnicodeString );
			// 사용자가 접근하는 이름을 메모리에서 해제한다

			IoDetachDevice ( NextLayerDeviceObject );// Next Layer Driver와의 연결고리를 우선 끊습니다
													 // SAMPLE5_AddDevice()에서 IoAttachDeviceToDeviceStack()함수의 반대의미
			IoDeleteDevice ( DeviceObject );		 // 우리의 DeviceObject를 제거합니다
													 // SAMPLE5_AddDevice()에서 IoCreateDevice()함수의 반대의미
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation( Irp ); // 현재 IRP는 이후 종결될때까지 더이상 간섭을 하지 않겠다는 의미입니다
			returnStatus = IoCallDriver(NextLayerDeviceObject, Irp);
		}
		break;

		default:
		{
			IoSkipCurrentIrpStackLocation( Irp ); // 현재 IRP는 이후 종결될때까지 더이상 간섭을 하지 않겠다는 의미입니다
			returnStatus = IoCallDriver(NextLayerDeviceObject, Irp);
		}
		break;
	} 

SAMPLE5_PnpDispatch_Exit:
	DbgPrint("SAMPLE5_PnpDispatch -- \n" );

	return returnStatus;
}

NTSTATUS                                                            
SAMPLE5_PowerDispatch
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// POWERManager가 보내는 IRP를 처리하는 처리기
	PDEVICE_EXTENSION		deviceExtension;
	NTSTATUS returnStatus;
	PDEVICE_OBJECT NextLayerDeviceObject;           /* pointer to the stack device object */
	
	DbgPrint("SAMPLE5_PowerDispatch ++ \n" );

	IrpStringOut( Irp );

	deviceExtension = DeviceObject->DeviceExtension;
	NextLayerDeviceObject = deviceExtension->NextLayerDeviceObject;

	PoStartNextPowerIrp( Irp ); // 또 다른 Power IRP를 받을 수 있다고 PowerManager에게 알립니다
	IoSkipCurrentIrpStackLocation( Irp ); // 현재 IRP는 이후 종결될때까지 더이상 간섭을 하지 않겠다는 의미입니다
	returnStatus = PoCallDriver(NextLayerDeviceObject, Irp); // Power IRP는 다른 IRP와 달리, 반드시 PoCallDriver()함수를 사용해야 합니다
	
	DbgPrint("SAMPLE5_PowerDispatch -- \n" );
	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Create
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application가 보내는 CreateFile()함수에 대응되는 처리기
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;

	DbgPrint("SAMPLE5_Create ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	if( !AllocateStructForFileContext( pStack->FileObject ) ) 
		returnStatus = STATUS_INVALID_PARAMETER;
	
	DbgPrint("SAMPLE5_Create -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Close
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application가 보내는 CloseHandle()함수에 대응되는 처리기
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;

	DbgPrint("SAMPLE5_Close ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	FreeStructForFileContext( pStack->FileObject ); // 사용된 FsContext를 제거합니다
	
	DbgPrint("SAMPLE5_Close -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Cleanup
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application가 보내는 CloseHandle()함수에 대응되는 처리기
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;

	DbgPrint("SAMPLE5_Cleanup ++ \n" );

	IrpStringOut( Irp );

	DbgPrint("SAMPLE5_Cleanup -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Read
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application가 보내는 ReadFile()함수에 대응되는 처리기
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;
	ULONG dwRequestSize;
	char * pClientBuffer;

	DbgPrint("SAMPLE5_Read ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	dwRequestSize = pStack->Parameters.Read.Length;
	// 수용가능한 크기를 확인한다... 
	pClientBuffer = Irp->AssociatedIrp.SystemBuffer; // DO_BUFFERED_IO

	switch( GetTypeCreate(pStack->FileObject) )
	{
		case STRINGTYPE: 
			{
				PCONTEXT_FOR_STRING	pContextForString;
				pContextForString = (PCONTEXT_FOR_STRING)pStack->FileObject->FsContext;
				dwRequestSize = ((int)dwRequestSize >= pContextForString->CurrentRemainBufferSize)?
					pContextForString->CurrentRemainBufferSize:dwRequestSize;

				if( !dwRequestSize )
				{
					returnStatus = STATUS_INVALID_PARAMETER;
				}
				else
				{
					memcpy( pClientBuffer, pContextForString->StringBuffer, dwRequestSize );
				}
			} 
			break;

		case MEMORYTYPE: 
			{
				PCONTEXT_FOR_MEMORY	pContextForMemory;
				PUCHAR * ppSystemBuffer;
				
				if( dwRequestSize != sizeof(PVOID) )
				{
					returnStatus = STATUS_INVALID_PARAMETER;
				}
				ppSystemBuffer = (PUCHAR * )Irp->AssociatedIrp.SystemBuffer;
				pContextForMemory = (PCONTEXT_FOR_MEMORY)pStack->FileObject->FsContext;
				*ppSystemBuffer = (PUCHAR)pContextForMemory->UserLevelVirtualAddress;
			}
			break;

		default:
			break;
	}
	Irp->IoStatus.Information = dwRequestSize; // 실제수행된 크기를 적는다..
	
	DbgPrint("SAMPLE5_Read -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Write
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application가 보내는 WriteFile()함수에 대응되는 처리기
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;
	ULONG dwRequestSize;
	char * pClientBuffer;

	DbgPrint("SAMPLE5_Write ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	dwRequestSize = pStack->Parameters.Write.Length;
	// 수용가능한 크기를 확인한다...
	pClientBuffer = Irp->AssociatedIrp.SystemBuffer; // DO_BUFFERED_IO

	switch( GetTypeCreate(pStack->FileObject) )
	{
		case STRINGTYPE:
		{
			PCONTEXT_FOR_STRING	pContextForString;
			pContextForString = (PCONTEXT_FOR_STRING)pStack->FileObject->FsContext;
			dwRequestSize = ((int)dwRequestSize > MAXSTRINGBUFFERCOUNT)?
				0:dwRequestSize;

			if( !dwRequestSize )
			{
				returnStatus = STATUS_INVALID_PARAMETER;
			}
			else
			{
				memcpy( pContextForString->StringBuffer, pClientBuffer, dwRequestSize );
				pContextForString->CurrentRemainBufferSize = dwRequestSize;
			}
		}
		break;

		case MEMORYTYPE:
		{
			PCONTEXT_FOR_MEMORY	pContextForMemory;
			PWRITE_PARAMETER pWriteParameter;

			pContextForMemory = (PCONTEXT_FOR_MEMORY)pStack->FileObject->FsContext;
			pWriteParameter = (PWRITE_PARAMETER)pClientBuffer;

			// 사용자가 보내준 물리메모리와 크기정보를 가지고
			// 사용자가 접근가능한 새로운 가상주소를 매핑합니다

			// 우선, 이전에 사용중이던 매핑정보가 남아있다면, 이것을 먼저 해제합니다
			_memorytype_flush_context( pContextForMemory );

			// 새로운 가상메모리를 매핑합니다
			_memorytype_mapping_context( pContextForMemory, pWriteParameter );
		}
		break;

		default:
			break;
	}
	Irp->IoStatus.Information = dwRequestSize; // 실제수행된 크기를 적는다..
	
	DbgPrint("SAMPLE5_Write -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

////////////////////////////////////////////////////////////////
//
// 추가된 함수들입니다
//

VOID
MyThreadRoutine(
    IN PVOID Context
    )
{ // MyThreadRoutine은 System프로세스의 문맥아래에서 실행됩니다
	PTHREAD_CONTEXT pThreadContext;
	PCONTEXT_FOR_IOPORT pContextForIoPort;
	KAPC_STATE ApcState;
	NTSTATUS status=STATUS_SUCCESS;

	pThreadContext = (PTHREAD_CONTEXT)Context;
	pContextForIoPort = (PCONTEXT_FOR_IOPORT)pThreadContext->Context;

	KeStackAttachProcess( pThreadContext->Process, &ApcState );

	switch( pThreadContext->dwIoControlCode )
	{
		case IOCTL_SAMPLE_SET_EVENT:
			// EventHandle를 사용해서 EventObject를 구합니다
			status = ObReferenceObjectByHandle(
				pContextForIoPort->UserEventHandle,
				GENERIC_READ|GENERIC_WRITE,
				NULL,
				KernelMode,
				&pContextForIoPort->pUserEventObject,
				NULL
			);
			break;
		case IOCTL_SAMPLE_IO_ENABLE:
			RtlZeroMemory( IOPM, 0x2000 );
			Ke386IoSetAccessProcess( PsGetCurrentProcess(), 1 );
			Ke386SetIoAccessMap( 1, IOPM );
			// APP프로세스가 IOPORT를 접근하는 것을 허용시킵니다
			break;
		case IOCTL_SAMPLE_IO_DISABLE:
			Ke386IoSetAccessProcess( PsGetCurrentProcess(), 0 ); 
			// APP프로세스가 IOPORT를 접근하는 것을 막습니다
			break;
	}
	if( NT_SUCCESS( status ) )
		KeSetEvent( pContextForIoPort->pUserEventObject, 0, FALSE );

	KeUnstackDetachProcess( &ApcState );
	PsTerminateSystemThread( status );
}

VOID
MyWorkItemRoutine(
	PDEVICE_OBJECT DeviceObject,
	PVOID Context 
	)
{ // MyWorkItemRoutine은 System프로세스의 문맥아래에서 실행됩니다
	HANDLE ThreadHandle;
	PWORKITEM_CONTEXT pIoWorkItemContext;
	PCONTEXT_FOR_IOPORT pContextForIoPort;
	PTHREAD_CONTEXT pThreadContext;

	pIoWorkItemContext = (PWORKITEM_CONTEXT)Context;
	pContextForIoPort = (PCONTEXT_FOR_IOPORT)pIoWorkItemContext->Context;

	pThreadContext = (PTHREAD_CONTEXT)ExAllocatePool( NonPagedPool, sizeof(THREAD_CONTEXT) );
	pThreadContext->dwIoControlCode = pIoWorkItemContext->dwIoControlCode;
	pThreadContext->Context = (PVOID)pContextForIoPort;
	PsLookupProcessByProcessId( pContextForIoPort->ProcessId, (PEPROCESS *)&pThreadContext->Process );

	PsCreateSystemThread( &ThreadHandle, THREAD_ALL_ACCESS, NULL, 
		NULL, NULL, MyThreadRoutine, pThreadContext );

	ExFreePool( pIoWorkItemContext->pIoWorkItem );
	ExFreePool( pIoWorkItemContext );
}

NTSTATUS
SAMPLE5_DeviceIoControl
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	)
{ // 응용프로그램측에서 호출하는 Win32 API DeviceIoControl()함수와 연결되는 함수
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;
	ULONG dwRequestSize=0;
	PEVENT_INFORMATION pEvent_Information;
	PCONTEXT_FOR_IOPORT	pContextForIoPort;
	PWORKITEM_CONTEXT pIoWorkItemContext;

	DbgPrint("SAMPLE5_DeviceIoControl ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	switch( GetTypeCreate(pStack->FileObject) )
	{
		case IOPORTTYPE:
			switch( pStack->Parameters.DeviceIoControl.IoControlCode )
			// 응용프로그램이 전달하는 ControlCode를 확인합니다
			{
				case IOCTL_SAMPLE_SET_EVENT:
					pContextForIoPort = (PCONTEXT_FOR_IOPORT)pStack->FileObject->FsContext;

					pEvent_Information = (PEVENT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
					// METHOD_BUFFERED
					
					if( pContextForIoPort->pUserEventObject )
					{
						KeSetEvent( pContextForIoPort->pUserEventObject, 0, FALSE );
						ObDereferenceObject( pContextForIoPort->pUserEventObject );
						pContextForIoPort->pUserEventObject = 0;
					}

					pContextForIoPort->UserEventHandle = pEvent_Information->UserEventHandle;
					pContextForIoPort->ProcessId = pEvent_Information->ProcessId;
		
					pIoWorkItemContext = (PWORKITEM_CONTEXT)ExAllocatePool( NonPagedPool, sizeof(WORKITEM_CONTEXT) );
					pIoWorkItemContext->pIoWorkItem = IoAllocateWorkItem( DeviceObject );
					pIoWorkItemContext->Context = (PVOID)pContextForIoPort;
					pIoWorkItemContext->dwIoControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
					IoQueueWorkItem( pIoWorkItemContext->pIoWorkItem, MyWorkItemRoutine, CriticalWorkQueue, pIoWorkItemContext ); 

					dwRequestSize = sizeof(EVENT_INFORMATION);
					break;
				case IOCTL_SAMPLE_IO_ENABLE:
					pContextForIoPort = (PCONTEXT_FOR_IOPORT)pStack->FileObject->FsContext;
					if( !pContextForIoPort->pUserEventObject )
					{
						returnStatus = STATUS_INVALID_PARAMETER;
						break;
					}

					pIoWorkItemContext = (PWORKITEM_CONTEXT)ExAllocatePool( NonPagedPool, sizeof(WORKITEM_CONTEXT) );
					pIoWorkItemContext->pIoWorkItem = IoAllocateWorkItem( DeviceObject );
					pIoWorkItemContext->Context = (PVOID)pContextForIoPort;
					pIoWorkItemContext->dwIoControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
					IoQueueWorkItem( pIoWorkItemContext->pIoWorkItem, MyWorkItemRoutine, CriticalWorkQueue, pIoWorkItemContext ); 

					break;
				case IOCTL_SAMPLE_IO_DISABLE:
					pContextForIoPort = (PCONTEXT_FOR_IOPORT)pStack->FileObject->FsContext;
					if( !pContextForIoPort->pUserEventObject )
					{
						returnStatus = STATUS_INVALID_PARAMETER;
						break;
					}

					pIoWorkItemContext = (PWORKITEM_CONTEXT)ExAllocatePool( NonPagedPool, sizeof(WORKITEM_CONTEXT) );
					pIoWorkItemContext->pIoWorkItem = IoAllocateWorkItem( DeviceObject );
					pIoWorkItemContext->Context = (PVOID)pContextForIoPort;
					pIoWorkItemContext->dwIoControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;
					IoQueueWorkItem( pIoWorkItemContext->pIoWorkItem, MyWorkItemRoutine, CriticalWorkQueue, pIoWorkItemContext ); 

					break;
			}
			break;
		default:
			break;
	}
	Irp->IoStatus.Information = dwRequestSize; // 실제수행된 크기를 적는다..
	
	DbgPrint("SAMPLE5_DeviceIoControl -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
}
