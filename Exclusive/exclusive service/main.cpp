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
	MessageBox(NULL,TEXT("������ �ð��� �帥��"),TEXT("��ǥ"),MB_ICONINFORMATION);
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

// ������ ���� ���¸� �����ϴ� �Լ�
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

	// ���� ���¸� ������ �д�.
	g_NowState=dwState;
	SetServiceStatus(g_hSrv,&ss);
}

void MyServiceMain(DWORD argc, LPTSTR *argv)
{
	TCHAR str[256];

	// ���� �ڵ鷯�� ����Ѵ�.
	g_hSrv=RegisterServiceCtrlHandler("NoGame",(LPHANDLER_FUNCTION)MyServiceHandler);
	if (g_hSrv==0) {
		return;
	}

	// ���񽺰� ���������� �˸���.
	MySetStatus(SERVICE_START_PENDING);
	
	// ���� ������ �ʱ�ȭ�Ѵ�.
	g_bPause=FALSE;

	// �̺�Ʈ�� �����Ѵ�.
	g_ExitEvent=CreateEvent(NULL, TRUE, FALSE, "NoGameExit");
	
	// ���񽺰� ���۵Ǿ����� �˸���.
	MySetStatus(SERVICE_RUNNING);

	// 10�ʿ� �ѹ��� �޸� ��踦 �ۼ��Ѵ�.
	for (;;) {
		if (g_bPause == FALSE) {
			if(FindWindow(NULL,TEXT("Chess Titans"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("Mahjong Titans"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("Purble Place"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("�����̴� ī�����"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("���� ã��"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("ī�����"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("������"))!=NULL)detect();
			if(FindWindow(NULL,TEXT("��Ʈ"))!=NULL)detect();	
		}
		if (WaitForSingleObject(g_ExitEvent, 10000) == WAIT_OBJECT_0)
			break;
	}
	MySetStatus(SERVICE_STOPPED);
}

// �ڵ鷯 �Լ�
void MyServiceHandler(DWORD fdwControl)
{
	HANDLE hFile;

	// ���� ���¿� ���� ���� �ڵ��� ���� ó���� �ʿ� ����.
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
