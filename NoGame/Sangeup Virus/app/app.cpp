// app.cpp : Defines the entry point for the console application.
//

// GUID�� ����Ͽ� ������ DeviceStack�� ���� �����̸��� ���ϴ� �������α׷� �����Դϴ�

//#include "stdafx.h"
#include <windows.h>

#include <initguid.h>
#include <setupapi.h>
#include <stdio.h>

DEFINE_GUID(SampleGuid, 0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
// SampleGuid�� ����մϴ�. �̷� GUID�� ���� ��������� ���ǵ��� ���� �����μ�, ���÷� �����Ͽ� ����մϴ�

#define MAXDEVICENUMBER 10 // 10�� ������ ���� Guid�� ����ϴ� ��ġ�� �����Ѵٴ� �ǹ��Դϴ�

#include <winioctl.h>

// ����̹��� �ְ���� ControlCode�� �����մϴ�
#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct
{
	HANDLE UserEventHandle;
	HANDLE ProcessId;
}EVENT_INFORMATION, *PEVENT_INFORMATION;
// ����̹������� ������ �����ͱ���ü

#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)

// ���� �����Ǵ� GUID�� �����ϴ� DeviceStack�� ������ �˷��ݴϴ�
int GetDeviceStackNameCount( struct _GUID * pGuid )
{
	SP_INTERFACE_DEVICE_DATA interfaceData;
	int index=0;
	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );

	if( Info == (HDEVINFO) -1 )
		return 0; // �ý����� �̷� ����� �������� ���Ѵ�

	interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
	
	while( 1 )
	{
		BOOLEAN bl;
		bl = SetupDiEnumDeviceInterfaces( Info, 0, pGuid, index, &interfaceData );
		if( bl == FALSE )
			break;
		index++;
	}

	SetupDiDestroyDeviceInfoList( Info );

	return index;
}

// ���� �����Ǵ� GUID�� �����ϴ� DeviceStack�� �̸����� �˷��ݴϴ�
BOOLEAN GetDeviceStackName( struct _GUID * pGuid, char ** ppDeviceName, int index )
{
	DWORD size;
	BOOLEAN bl;
	SP_INTERFACE_DEVICE_DATA interfaceData;
	PSP_INTERFACE_DEVICE_DETAIL_DATA pData;
	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );
	char *pDeviceName;
	*ppDeviceName = (char *)0;

	if( Info == (HANDLE) -1 )
		return FALSE;

	interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
	
	bl = SetupDiEnumDeviceInterfaces( Info, 0, pGuid, index, &interfaceData );
	if( bl == FALSE )
		return bl;

	SetupDiGetDeviceInterfaceDetail( Info, &interfaceData, 0, 0, &size, 0 );
	pData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc( size );
	pData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
	SetupDiGetDeviceInterfaceDetail( Info, &interfaceData, pData, size, 0, 0 );

	pDeviceName = (char *)malloc( strlen(pData->DevicePath) + 1 );
	memset( pDeviceName, 0, strlen(pData->DevicePath) );
	strcpy( pDeviceName, pData->DevicePath );
	free( pData );

	SetupDiDestroyDeviceInfoList( Info );
	*ppDeviceName = pDeviceName;
	return TRUE;
}

#include <conio.h>

int main(int argc, char* argv[])
{
	HANDLE handle;
	int count;
	char *pDeviceName;
	BOOLEAN bl;
	ULONG ret;

	count = GetDeviceStackNameCount( (struct _GUID *)&SampleGuid );
	if( count == 0 )
		return 0; // �ý����� SampleGuid�� �����ϴ� ��ġ�� ��ġ���� �ʾҽ��ϴ�

	bl = GetDeviceStackName( (struct _GUID *)&SampleGuid, &pDeviceName, 0 ); // �翬�� 1���̻��� ��ġ�� ��ġ�Ǿ������Ƿ�..0�� ����Ѵ�
	// pDeviceName�� �Լ������� �Ҵ�Ǵ� �޸��̹Ƿ�, ����� ������ �ݵ�� �����Ͽ��� �Ѵ�

	if( bl == FALSE )
		return 0; // �̷����� ����� �Ѵ�

	// for IOPORT
	{
		char FullDeviceName[100];
		strcpy( FullDeviceName, pDeviceName );
		strcat( FullDeviceName, "\\IOPORT" );
		handle = CreateFile( FullDeviceName, GENERIC_READ|GENERIC_WRITE
						   , 0, 0, OPEN_EXISTING, 0
						   , 0 );
	}
	if( handle == (HANDLE)-1 )
	{
		free( pDeviceName );
		return 0; // Stack�� ������, ���� ������ �����Ǿ� �ִ�
	}

	EVENT_INFORMATION EventInformation;

	HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	EventInformation.ProcessId = (PVOID)GetCurrentProcessId();
	EventInformation.UserEventHandle = hEvent;
	// �������α׷��� ����̹������� �������α׷��� ProcessId�� ��ٸ����� �ϴ� Event�ڵ��� ������ �մϴ�

	DeviceIoControl( handle, IOCTL_SAMPLE_SET_EVENT, &EventInformation, sizeof(EVENT_INFORMATION), NULL, 0, &ret, NULL );
	// ����̹������� Event�� �˷��ݴϴ�
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "����̹������� EventHandle�� �����Ͽ����ϴ�", "APP", MB_OK );

	DeviceIoControl( handle, IOCTL_SAMPLE_IO_ENABLE, NULL, 0, NULL, 0, &ret, NULL );
	// ����̹����� IO PORT��������� �䱸�մϴ�
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "IO Port�� ������ �غ� �Ǿ����ϴ�", "APP", MB_OK );

	// PC����Ŀ�� �Ѻ��ϴ�
	_outp( 0x61, 0x03 );

	// ����Ŀ���ļ��� �� �����մϴ�
	_outp( 0x43, 0xb6 );
	_outp( 0x42, 0x40 );
	_outp( 0x42, 0x10 );

	printf("�ƹ����ڳ� ġ����\n");
	getch();

	// PC����Ŀ�� ���ϴ�
	_outp( 0x61, 0x00 );
	DeviceIoControl( handle, IOCTL_SAMPLE_IO_DISABLE, NULL, 0, NULL, 0, &ret, NULL );
	// ����̹����� IO PORT���ٱ����� �䱸�մϴ�
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "IO Port�� ���ٱ����� �Ǿ����ϴ�", "APP", MB_OK );

	CloseHandle( hEvent );
	CloseHandle( handle );

	free( pDeviceName );
	return 0;
}
