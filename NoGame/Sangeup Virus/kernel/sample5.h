#include <ddk/ntddk.H>

#ifndef _SAMPLE_
#define _SAMPLE_

typedef struct _DEVICE_EXTENSION 
{
	PDEVICE_OBJECT PhysicalDeviceObject; /* physical device object */
	PDEVICE_OBJECT NextLayerDeviceObject; /* Next Layer device object */
	PDEVICE_OBJECT DeviceObject; /* Self device object */
	UNICODE_STRING UnicodeString; 
	// 사용자에게 허용될 DeviceStack에 대한 이름을 보관하는곳
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS
SAMPLE5_AddDevice
	(
	IN PDRIVER_OBJECT DriverObject,         
	IN PDEVICE_OBJECT PhysicalDeviceObject  
	);

VOID                                                                
SAMPLE5_Unload                                                       
	(                                                               
	IN PDRIVER_OBJECT DriverObject  /* pointer to device object */
	);

NTSTATUS
SAMPLE5_DeferIrpCompletion
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to a device object */
	IN PIRP Irp,                        /* pointer to a I/O request packet */
	IN PVOID Context                    /* driver defined context */
	);

NTSTATUS
SAMPLE5_Create
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS
SAMPLE5_Read
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS
SAMPLE5_Write
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS                                                            
SAMPLE5_Cleanup
	(                                                               
		IN PDEVICE_OBJECT DeviceObject,                             
		IN PIRP Irp                                                 
	);

NTSTATUS
SAMPLE5_Close
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS
SAMPLE5_PnpDispatch
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS
SAMPLE5_PowerDispatch
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

NTSTATUS
SAMPLE5_DeviceIoControl
	(
	IN PDEVICE_OBJECT DeviceObject,     /* pointer to device object */
	IN PIRP Irp                         /* pointer to an I/O request packet */
	);

#endif //_SAMPLE_
