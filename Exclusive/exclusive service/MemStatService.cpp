#include <windows.h>

#define SERVICE_CONTROL_NEWFILE 128

void MyServiceMain(DWORD argc, LPTSTR *argv);
void MyServiceHandler(DWORD opCode);

SERVICE_STATUS_HANDLE g_hSrv;
DWORD g_NowState;
BOOL g_bPause;
HANDLE g_ExitEvent;
TCHAR gbuf[65536]="메모리 통계 파일\r\n";

int main()
{
	SERVICE_TABLE_ENTRY ste[]={
		{"MemStat",(LPSERVICE_MAIN_FUNCTION)MyServiceMain},
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
	HANDLE hFile;
	MEMORYSTATUS ms;
	DWORD dwWritten;
	TCHAR str[256];
	SYSTEMTIME st;

	// 서비스 핸들러를 등록한다.
	g_hSrv=RegisterServiceCtrlHandler("MemStat",(LPHANDLER_FUNCTION)MyServiceHandler);
	if (g_hSrv==0) {
		return;
	}

	// 서비스가 시작중임을 알린다.
	MySetStatus(SERVICE_START_PENDING);
	
	// 전역 변수를 초기화한다.
	g_bPause=FALSE;

	// 이벤트를 생성한다.
	g_ExitEvent=CreateEvent(NULL, TRUE, FALSE, "MemStatExit");

	// 새로운 로그 파일을 작성한다.
	hFile=CreateFile("c:\\MemStat.txt",GENERIC_WRITE,0,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	CloseHandle(hFile);
	
	// 서비스가 시작되었음을 알린다.
	MySetStatus(SERVICE_RUNNING);

	// 10초에 한번씩 메모리 통계를 작성한다.
	for (;;) {
		if (g_bPause == FALSE) {
			// 현재 시간과 메모리 양을 조사하여 문자열로 조립한다.
			GetLocalTime(&st);
			GlobalMemoryStatus(&ms);
			wsprintf(str, "%d월 %d일 %02d시 %02d분 %02d초 => "
				"사용가능 물리 메모리=%dMbytes(%d%%), 사용가능 가상메모리=%dMbytes,"
				" 사용가능 페이징 파일=%dMbytes\r\n",
				st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
				ms.dwAvailPhys/1048576,100-ms.dwMemoryLoad,ms.dwAvailVirtual/1048576,
				ms.dwAvailPageFile/1048576);

			// 파일로 통계를 출력한다. 버퍼가 가득찬 경우 파일을 다시 만든다.
			if (lstrlen(gbuf) > 60000) {
				hFile=CreateFile("c:\\MemStat.txt",GENERIC_WRITE,0,NULL,
					CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
				lstrcpy(gbuf,"메모리 통계 파일\r\n");
			} else {
				hFile=CreateFile("c:\\MemStat.txt",GENERIC_WRITE,0,NULL,
					OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
			}
			lstrcat(gbuf,str);
			WriteFile(hFile,gbuf,lstrlen(gbuf),&dwWritten,NULL);
			CloseHandle(hFile);
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
		hFile=CreateFile("c:\\MemStat.txt",GENERIC_WRITE,0,NULL,
			CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		lstrcpy(gbuf,"메모리 통계 파일\r\n");
		CloseHandle(hFile);
	case SERVICE_CONTROL_INTERROGATE:
	default:
		MySetStatus(g_NowState);
		break;
	}
}
