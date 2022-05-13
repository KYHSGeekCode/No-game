// app.cpp : Defines the entry point for the console application.
//

// GUID를 사용하여 접근할 DeviceStack에 대한 접근이름을 구하는 응용프로그램 예재입니다

//#include "stdafx.h"
#include <windows.h>

#include <initguid.h>
#include <setupapi.h>
#include <stdio.h>

DEFINE_GUID(SampleGuid, 0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
// SampleGuid를 사용합니다. 이런 GUID는 현재 윈도우즈에서 정의되지 않은 값으로서, 샘플로 정의하여 사용합니다

#define MAXDEVICENUMBER 10 // 10개 까지의 같은 Guid를 사용하는 장치를 지원한다는 의미입니다

#include <winioctl.h>

// 드라이버와 주고받을 ControlCode를 정의합니다
#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct
{
	HANDLE UserEventHandle;
	HANDLE ProcessId;
}EVENT_INFORMATION, *PEVENT_INFORMATION;
// 드라이버측으로 전달한 데이터구조체

#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)

// 현재 제공되는 GUID를 지원하는 DeviceStack의 개수를 알려줍니다
int GetDeviceStackNameCount( struct _GUID * pGuid )
{
	SP_INTERFACE_DEVICE_DATA interfaceData;
	int index=0;
	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );

	if( Info == (HDEVINFO) -1 )
		return 0; // 시스템은 이런 명령을 지원하지 못한다

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

// 현재 제공되는 GUID를 지원하는 DeviceStack의 이름들을 알려줍니다
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
		return 0; // 시스템은 SampleGuid를 지원하는 장치가 설치되지 않았습니다

	bl = GetDeviceStackName( (struct _GUID *)&SampleGuid, &pDeviceName, 0 ); // 당연히 1나이상의 장치는 설치되어있으므로..0을 사용한다
	// pDeviceName는 함수내에서 할당되는 메모리이므로, 사용이 끝나면 반드시 해제하여야 한다

	if( bl == FALSE )
		return 0; // 이런경우는 없어야 한다

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
		return 0; // Stack은 있지만, 현재 접근이 금지되어 있다
	}

	EVENT_INFORMATION EventInformation;

	HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	EventInformation.ProcessId = (PVOID)GetCurrentProcessId();
	EventInformation.UserEventHandle = hEvent;
	// 응용프로그램은 드라이버측으로 응용프로그램의 ProcessId와 기다리고자 하는 Event핸들을 보내야 합니다

	DeviceIoControl( handle, IOCTL_SAMPLE_SET_EVENT, &EventInformation, sizeof(EVENT_INFORMATION), NULL, 0, &ret, NULL );
	// 드라이버측으로 Event를 알려줍니다
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "드라이버측으로 EventHandle을 전달하였습니다", "APP", MB_OK );

	DeviceIoControl( handle, IOCTL_SAMPLE_IO_ENABLE, NULL, 0, NULL, 0, &ret, NULL );
	// 드라이버에게 IO PORT접근허용을 요구합니다
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "IO Port를 접근할 준비가 되었습니다", "APP", MB_OK );

	// PC스피커를 켜봅니다
	_outp( 0x61, 0x03 );

	// 스피커주파수를 재 설정합니다
	_outp( 0x43, 0xb6 );
	_outp( 0x42, 0x40 );
	_outp( 0x42, 0x10 );

	printf("아무글자나 치세요\n");
	getch();

	// PC스피커를 끕니다
	_outp( 0x61, 0x00 );
	DeviceIoControl( handle, IOCTL_SAMPLE_IO_DISABLE, NULL, 0, NULL, 0, &ret, NULL );
	// 드라이버에게 IO PORT접근금지를 요구합니다
	
	WaitForSingleObject( hEvent, INFINITE );
	ResetEvent( hEvent );

	MessageBox( NULL, "IO Port를 접근금지가 되었습니다", "APP", MB_OK );

	CloseHandle( hEvent );
	CloseHandle( handle );

	free( pDeviceName );
	return 0;
}
