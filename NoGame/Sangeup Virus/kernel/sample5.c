/*
	SAMPLE5����̹��� ������ �����Ϸ���, ���� �ó������� ���� �����ؾ� �մϴ�

	�������α׷� -> DeviceIoControl() -> SAMPLE5_DeviceIoControl() -> System's Process WorkItem Routine ->
	System's Process System Thread Routine

	����, System's Process System Thread Routine�� �������� ������ �������α׷��������� �ٲ�ġ�� �մϴ�
	�׷�����, �������α׷��� ������ �̺�Ʈ�ڵ��� �̺�ƮObject�� ��ȯ�մϴ�.
	����, ������� System's Process System Thread Routine���� ������ ȯ���մϴ�

	�̿� ���� �κ��� �ݵ�� �����ؾ� �ҽ��� �ľ��� �� �ֽ��ϴ�
*/

// SAMPLE5����̹��� ����̹������� ��������α׷������� ����� �˷��ִ� ����� �������α׷������� �����Ӱ� IOPORT�� �����ϴ� ����� �˷��ݴϴ�

#include <ddk/ntddk.h>        /* required for WDM driver development */
#include <stdarg.h>     /* standard io include files */
#include <stdio.h>

//#include <devioctl.h>
#include <initguid.h>

DEFINE_GUID(GUID_SAMPLE, 
0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
// ���� �츮�� ���� DeviceStack�� ���� Interface GUID�� �����մϴ�

//// ���� �ҽ��� ntddk.h�� ����ϰ� �ֱ� ������ �߰��������� ������ ����� �� ����
// NTIFS.H ���� ���ǵǾ��ִ� �Լ������� �ڷᱸ����
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
// �������α׷��� �ְ���� ControlCode�� �����մϴ�
#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct
{
	HANDLE UserEventHandle;
	HANDLE ProcessId;
}EVENT_INFORMATION, *PEVENT_INFORMATION;
// �������α׷������� ���޵� �����ͱ���ü

#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)
////

typedef struct
{
	ULONG dwIoControlCode;
	PEPROCESS Process;
	PVOID Context;
}THREAD_CONTEXT, *PTHREAD_CONTEXT;
// System Thread��ƾ������ ���� Context

typedef struct
{
	ULONG dwIoControlCode;
	PIO_WORKITEM pIoWorkItem;
	PVOID Context;
}WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;
// WorkItem��ƾ������ ���� Context

#pragma optimize("", off)
#include "SAMPLE5.h"    
#include "createmethod.h"

char IOPM[0x2000];

void IrpStringOut( PIRP Irp ) // IRP�� ���������Ʈ�մϴ�
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
	)// ����̹��� �޸𸮿� ó�������ϴ� ��쿡�� �ѹ� ȣ��˴ϴ�
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
	)// PNPManager�� �̰��� ���ؼ� IRP DeviceStack�� �����϶�� ��û�� �մϴ�
{
	/* IRP DeviceStack�� �����ϱ� ���ؼ��� ��� �ܰ踦 ��ġ�� �˴ϴ� 
	1. DeviceObject�����۾�
	2. �����ϴ� DeviceStack���� DeviceObject�� �ø���
	3. DeviceObject�� Flag�� �����ϱ�
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
					);// DeviceObject�� �����մϴ�

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
	);// �����ϴ� DeviceStack���� DeviceObject�� �ø��ϴ�

	DeviceObject->Flags |= deviceExtension->NextLayerDeviceObject->Flags & 
							( DO_POWER_PAGABLE  | DO_POWER_INRUSH); // DeviceObject�� Flag�� �����մϴ�

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING; // DeviceObject�� Flag�� �����մϴ�

	DeviceObject->Flags |= DO_BUFFERED_IO; // IRP�� SystemBuffer�� ����ؼ� �������α׷����ۿ� ����ɰ����� �Ͻ�

	IoRegisterDeviceInterface( PhysicalDeviceObject, &GUID_SAMPLE, NULL, &deviceExtension->UnicodeString );
	// ���� �츮�� ���� DeviceStack�� ���� InterfaceGuid�� ����մϴ�
	// ���ϵǴ� de->UnicodeString�� ����ڰ� �����ϴ� �̸��Դϴ�


SAMPLE5_AddDevice_Exit:
	DbgPrint("SAMPLE5_AddDevice -- \n" );
	return returnStatus;
}

VOID                                                                
SAMPLE5_Unload                                                       
	(                                                               
	IN PDRIVER_OBJECT DriverObject  /* pointer to device object */
	)
{// ���̻� ����̹��� ������ DeviceObject�� ������ �̰��� ȣ�����, ����̹��� �޸𸮿��� ���ŵ˴ϴ�

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
	)// ��ɾ� IRP�� NextLayer ����̹����� IoCompleteRequest()�Լ��� ���ؼ� �����û�Ǵ� ��쿡 ȣ��˴ϴ�
{
	PKEVENT event = Context;    /* kernel event passed in Context */

	DbgPrint("SAMPLE5_DeferIrpCompletion ++ \n" );

	KeSetEvent(event,
			   1,
			   FALSE);

	
	DbgPrint("SAMPLE5_DeferIrpCompletion -- \n" );

	return STATUS_MORE_PROCESSING_REQUIRED; // IoCompleteRequest()�Լ��� �Ͽ���, ���� IRP�� ��������� �ߴ��϶�� ��û�� �մϴ�
}

NTSTATUS
SAMPLE5_PnpDispatch
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	)
{// PNPManager�� ������ IRP�� ó���ϴ� ó����
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
		{// DeviceStack�� �����϶�� �㰡�� �ǹ��մϴ�
			KEVENT event;   /* event used to wait for an operation to complete */
			KeInitializeEvent(&event, NotificationEvent, FALSE);
			IoCopyCurrentIrpStackLocationToNext(Irp);  
			IoSetCompletionRoutine(Irp,
								   SAMPLE5_DeferIrpCompletion,
								   &event,
								   TRUE,
								   TRUE,
								   TRUE); // SAMPLE5_DeferIrpCompletion()�Լ��� ���� IRP�� ���� Next Layer Driver�� ȣ���ϴ�
										  // IoCompleteRequest()�Լ��� ���ؼ� ȣ��ǵ��� �����մϴ�
			
			returnStatus = IoCallDriver(NextLayerDeviceObject,Irp); // NextLayer Driver���� �� ����� �����ѵ�, ����� Ȯ���մϴ�
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
			// NextLayer Driver�� �� ����� ���������� �ٷ������, �츮�� �����ΰ��� �ص� ���ٴ� �ǹ��Դϴ�

			// Do Job...
			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, TRUE ); 
			// ����ڰ� �츮�� DeviceStack�� �����ϴ� ���� ���

			IoCompleteRequest( Irp, IO_NO_INCREMENT ); // ���� IRP�� �����û�մϴ�
			// SAMPLE5_DeferIrpCompletion()�Լ����� �츮�� IRP�� ���� Next Layer Driver�� ȣ���� IoCompleteRequest()�� ����
		}	// �����ϵ��� "STATUS_MORE_PROCESSING_REQUIRED"������ ���ױ⶧����, �츮�� �����û�� �ؾ��մϴ�
		break;

		case IRP_MN_SURPRISE_REMOVAL :
		{ // ���ڱ� ��ġ�� PC���� ���ŵǾ��ٴ� �ǹ̿��� ���޵˴ϴ�

			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, FALSE );
			// ����ڰ� �츮�� DeviceStack�� �����ϴ� ���� ����

			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation( Irp ); // ���� IRP�� ���� ����ɶ����� ���̻� ������ ���� �ʰڴٴ� �ǹ��Դϴ�
			returnStatus = IoCallDriver(NextLayerDeviceObject, Irp);
		}
		break;

		case IRP_MN_REMOVE_DEVICE :
		{ // IRP DeviceStack�� ��ü�϶�� ��û�� �ǹ̷� ���޵˴ϴ�
			IoSetDeviceInterfaceState( &deviceExtension->UnicodeString, FALSE );
			// ����ڰ� �츮�� DeviceStack�� �����ϴ� ���� ����

			RtlFreeUnicodeString( &deviceExtension->UnicodeString );
			// ����ڰ� �����ϴ� �̸��� �޸𸮿��� �����Ѵ�

			IoDetachDevice ( NextLayerDeviceObject );// Next Layer Driver���� ������� �켱 �����ϴ�
													 // SAMPLE5_AddDevice()���� IoAttachDeviceToDeviceStack()�Լ��� �ݴ��ǹ�
			IoDeleteDevice ( DeviceObject );		 // �츮�� DeviceObject�� �����մϴ�
													 // SAMPLE5_AddDevice()���� IoCreateDevice()�Լ��� �ݴ��ǹ�
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation( Irp ); // ���� IRP�� ���� ����ɶ����� ���̻� ������ ���� �ʰڴٴ� �ǹ��Դϴ�
			returnStatus = IoCallDriver(NextLayerDeviceObject, Irp);
		}
		break;

		default:
		{
			IoSkipCurrentIrpStackLocation( Irp ); // ���� IRP�� ���� ����ɶ����� ���̻� ������ ���� �ʰڴٴ� �ǹ��Դϴ�
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
{// POWERManager�� ������ IRP�� ó���ϴ� ó����
	PDEVICE_EXTENSION		deviceExtension;
	NTSTATUS returnStatus;
	PDEVICE_OBJECT NextLayerDeviceObject;           /* pointer to the stack device object */
	
	DbgPrint("SAMPLE5_PowerDispatch ++ \n" );

	IrpStringOut( Irp );

	deviceExtension = DeviceObject->DeviceExtension;
	NextLayerDeviceObject = deviceExtension->NextLayerDeviceObject;

	PoStartNextPowerIrp( Irp ); // �� �ٸ� Power IRP�� ���� �� �ִٰ� PowerManager���� �˸��ϴ�
	IoSkipCurrentIrpStackLocation( Irp ); // ���� IRP�� ���� ����ɶ����� ���̻� ������ ���� �ʰڴٴ� �ǹ��Դϴ�
	returnStatus = PoCallDriver(NextLayerDeviceObject, Irp); // Power IRP�� �ٸ� IRP�� �޸�, �ݵ�� PoCallDriver()�Լ��� ����ؾ� �մϴ�
	
	DbgPrint("SAMPLE5_PowerDispatch -- \n" );
	return returnStatus;
} 

NTSTATUS                                                            
SAMPLE5_Create
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	)
{// Application�� ������ CreateFile()�Լ��� �����Ǵ� ó����
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
{// Application�� ������ CloseHandle()�Լ��� �����Ǵ� ó����
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;

	DbgPrint("SAMPLE5_Close ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	FreeStructForFileContext( pStack->FileObject ); // ���� FsContext�� �����մϴ�
	
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
{// Application�� ������ CloseHandle()�Լ��� �����Ǵ� ó����
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
{// Application�� ������ ReadFile()�Լ��� �����Ǵ� ó����
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;
	ULONG dwRequestSize;
	char * pClientBuffer;

	DbgPrint("SAMPLE5_Read ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	dwRequestSize = pStack->Parameters.Read.Length;
	// ���밡���� ũ�⸦ Ȯ���Ѵ�... 
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
	Irp->IoStatus.Information = dwRequestSize; // ��������� ũ�⸦ ���´�..
	
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
{// Application�� ������ WriteFile()�Լ��� �����Ǵ� ó����
	NTSTATUS returnStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pStack;
	ULONG dwRequestSize;
	char * pClientBuffer;

	DbgPrint("SAMPLE5_Write ++ \n" );

	IrpStringOut( Irp );

	pStack = IoGetCurrentIrpStackLocation( Irp );

	dwRequestSize = pStack->Parameters.Write.Length;
	// ���밡���� ũ�⸦ Ȯ���Ѵ�...
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

			// ����ڰ� ������ �����޸𸮿� ũ�������� ������
			// ����ڰ� ���ٰ����� ���ο� �����ּҸ� �����մϴ�

			// �켱, ������ ������̴� ���������� �����ִٸ�, �̰��� ���� �����մϴ�
			_memorytype_flush_context( pContextForMemory );

			// ���ο� ����޸𸮸� �����մϴ�
			_memorytype_mapping_context( pContextForMemory, pWriteParameter );
		}
		break;

		default:
			break;
	}
	Irp->IoStatus.Information = dwRequestSize; // ��������� ũ�⸦ ���´�..
	
	DbgPrint("SAMPLE5_Write -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
} 

////////////////////////////////////////////////////////////////
//
// �߰��� �Լ����Դϴ�
//

VOID
MyThreadRoutine(
    IN PVOID Context
    )
{ // MyThreadRoutine�� System���μ����� ���ƾƷ����� ����˴ϴ�
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
			// EventHandle�� ����ؼ� EventObject�� ���մϴ�
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
			// APP���μ����� IOPORT�� �����ϴ� ���� ����ŵ�ϴ�
			break;
		case IOCTL_SAMPLE_IO_DISABLE:
			Ke386IoSetAccessProcess( PsGetCurrentProcess(), 0 ); 
			// APP���μ����� IOPORT�� �����ϴ� ���� �����ϴ�
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
{ // MyWorkItemRoutine�� System���μ����� ���ƾƷ����� ����˴ϴ�
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
{ // �������α׷������� ȣ���ϴ� Win32 API DeviceIoControl()�Լ��� ����Ǵ� �Լ�
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
			// �������α׷��� �����ϴ� ControlCode�� Ȯ���մϴ�
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
	Irp->IoStatus.Information = dwRequestSize; // ��������� ũ�⸦ ���´�..
	
	DbgPrint("SAMPLE5_DeviceIoControl -- \n" );

	Irp->IoStatus.Status = returnStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return returnStatus;
}
