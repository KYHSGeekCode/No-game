#include <cstdlib>
#include <iostream>
#include <windows.h>
#include "resource.h"

using namespace std;

#define SERVICE_CONTROL_NEWFILE 128

void MyServiceMain(DWORD argc, LPTSTR *argv);
void MyServiceHandler(DWORD opCode);

SERVICE_STATUS_HANDLE g_hSrv;
DWORD g_NowState;
BOOL g_bPause;
HANDLE g_ExitEvent;





void detect(){
	PlaySound(MAKEINTRESOURCE(IDR_FOOL),0,SND_RESOURCE|SND_ASYNC);
	MessageBox(NULL,TEXT("언제나 시간은 흐른다"),TEXT("별표"),MB_ICONINFORMATION);
}

int main()
{
	SERVICE_TABLE_ENTRY ste[]={
		{"NoGame",(LPSERVICE_MAIN_FUNCTION)MyServiceMain},
		{NULL,NULL}
	};

	StartServiceCtrlDispatcher(ste);

	return 0;
}

// 서비스의 현재 상태를 변경하는 함수
void MySetStatus(DWORD dwState, DWORD dwAccept=SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE)
{
	SERVICE_STATUS ss;
	ss.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
	ss.dwCurrentState=dwState;
	ss.dwControlsAccepted=dwAccept;
	ss.dwWin32ExitCode=0;
	ss.dwServiceSpecificExitCode=0;
	ss.dwCheckPoint=0;
	ss.dwWaitHint=0;

	// 현재 상태를 보관해 둔다.
	g_NowState=dwState;
	SetServiceStatus(g_hSrv,&ss);
}

void MyServiceMain(DWORD argc, LPTSTR *argv)
{
	TCHAR str[256];

	// 서비스 핸들러를 등록한다.
	g_hSrv=RegisterServiceCtrlHandler("NoGame",(LPHANDLER_FUNCTION)MyServiceHandler);
	if (g_hSrv==0) {
		return;
	}

	// 서비스가 시작중임을 알린다.
	MySetStatus(SERVICE_START_PENDING);
	
	// 전역 변수를 초기화한다.
	g_bPause=FALSE;

	// 이벤트를 생성한다.
	g_ExitEvent=CreateEvent(NULL, TRUE, FALSE, "NoGameExit");
	
	// 서비스가 시작되었음을 알린다.
	MySetStatus(SERVICE_RUNNING);

	// 10초에 한번씩 메모리 통계를 작성한다.
	for (;;) {
		if (g_bPause == FALSE) {
			if(FindWindow(NULL,TEXT("Chess Titans"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("Mahjong Titans"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("Purble Place"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("스파이더 카드놀이"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("지뢰 찾기"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("카드놀이"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("프리셀"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("하트"))!=NULL)detect();	
		}
		if (WaitForSingleObject(g_ExitEvent, 10000) == WAIT_OBJECT_0)
			break;
	}
	MySetStatus(SERVICE_STOPPED);
}

// 핸들러 함수
void MyServiceHandler(DWORD fdwControl)
{
	HANDLE hFile;

	// 현재 상태와 같은 제어 코드일 경우는 처리할 필요 없다.
	if (fdwControl == g_NowState)
		return;

	switch (fdwControl) {
	case SERVICE_CONTROL_PAUSE:
		MySetStatus(SERVICE_PAUSE_PENDING,0);
		g_bPause=TRUE;
		MySetStatus(SERVICE_PAUSED);
		break;
	case SERVICE_CONTROL_CONTINUE:
		MySetStatus(SERVICE_CONTINUE_PENDING,0);
		g_bPause=FALSE;
		MySetStatus(SERVICE_RUNNING);
		break;
	case SERVICE_CONTROL_STOP:
		MySetStatus(SERVICE_STOP_PENDING,0);
		SetEvent(g_ExitEvent);
		break;
	case SERVICE_CONTROL_NEWFILE:
	case SERVICE_CONTROL_INTERROGATE:
	default:
		MySetStatus(g_NowState);
		break;
	}
}
