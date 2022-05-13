#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
ShellExecute(NULL, bUserAnAdmin ? _T("open") : _T("runas"),
                            lpFile, lpParameters, lpDirectory, SW_SHOW) > (HINSTANCE)32;


#define SERVICE_CONTROL_NEWFILE 128

void MyServiceMain(DWORD argc, LPTSTR *argv);
void MyServiceHandler(DWORD opCode);

SERVICE_STATUS_HANDLE g_hSrv;
DWORD g_NowState;
BOOL g_bPause;
HANDLE g_ExitEvent;
//TCHAR gbuf[65536]="Student Protect\r\n";

int main()
{
	SERVICE_TABLE_ENTRY ste[]={
		{"Student Protect",(LPSERVICE_MAIN_FUNCTION)MyServiceMain},
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

BOOL KillLOL( );
void printError( TCHAR* msg );

void MyServiceMain(DWORD argc, LPTSTR *argv)
{
     TCHAR str[256];	
	// 서비스 핸들러를 등록한다.
	g_hSrv=RegisterServiceCtrlHandler("Student Protect",(LPHANDLER_FUNCTION)MyServiceHandler);
	if (g_hSrv==0) {
		return;
	}
	// 서비스가 시작중임을 알린다.
	MySetStatus(SERVICE_START_PENDING);
	// 전역 변수를 초기화한다.
	g_bPause=FALSE;
	// 이벤트를 생성한다.
	g_ExitEvent=CreateEvent(NULL, TRUE, FALSE, "Exit");
	//할 일 초기화 
	// 서비스가 시작되었음을 알린다.
	MySetStatus(SERVICE_RUNNING);
	// 30초에 한번씩 롤이 켜져 있는지 검사하여 끈다.
	for (;;) {
		if (g_bPause == FALSE) {
			//롤을 찾아 꺼버린다.
			//CreateToolhelp32Snapshot API를 이용해서 현재 실행중인 프로세스들의 스냅샷을 뜹니다.
            //그 다음, Process32First, Process32Next 로 스냅샷의 내용을 탐색하면서,
            //GetProcessModule 로 모듈 핸들을 얻은 다음
            //GetModuleFileName 으로 파일명을 읽어 찾는 파일명과 일치하는지 보면 됩니다.
            //그러면 해당 파일에 의해 실행된 프로세스ID를 찾을 수 있는데요, ID를 찾은 후,
            // EnumWindows 로 실행중인 윈도우들의 핸들을 찾아
            //GetWindowThreadProcessId 로 윈도우의 프로세스 ID가 아까 찾은 ID와 일치하는지 찾으면,
            // 그 윈도우가 해당 파일에 의해 실행된 윈도우가 됩니다.
            //KillLOL();
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
            HWND lol=FindWindow(NULL,"lol.launcher");
            if(lol)DestroyWindow(lol);
            lol=FindWindow(NULL,"LoL Patcher");
            if(lol)DestroyWindow(lol);
            lol=FindWindow(NULL,"PVP.Net 클라이언트");
            if(lol)DestroyWindow(lol);
            
            //이제 프로세스를 끄자.
             //taskkill /f /im xxxx.exe
             //taskkill /f /pid 0000
             system("taskkill /f /im LoLPatcherUx.exe");
             system("taskkill /f /im LoLPatcher.exe");
             system("taskkill /f /im LolClient.exe");
             system("taskkill /f /im LoLLauncher.exe");
             system("taskkill /f /im jpatch.exe");
             system("taskkill /f /im LolClient.exe");
             system("taskkill /f /im lol.launcher.exe");
             system("taskkill /f /im lol.launcher.admin.exe");
 
		}
		if (WaitForSingleObject(g_ExitEvent, 30000) == WAIT_OBJECT_0)
			break;
	}
	MySetStatus(SERVICE_STOPPED);
}

// 핸들러 함수
void MyServiceHandler(DWORD fdwControl)
{
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
	//case SERVICE_CONTROL_OK:
         break;
	case SERVICE_CONTROL_INTERROGATE:
	default:
		MySetStatus(g_NowState);
		break;
	}
}

//MSDN GetProcessList
//BOOL KillLOL( )
//{
//  HANDLE hProcessSnap;
//  HANDLE hProcess;
//  PROCESSENTRY32 pe32;
//  DWORD dwPriorityClass;
//  BOOL dwRet;
//  // Take a snapshot of all processes in the system.
//  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
//  if( hProcessSnap == INVALID_HANDLE_VALUE )
//  {
//    printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
//    return( FALSE );
//  }
//
//  // Set the size of the structure before using it.
//  pe32.dwSize = sizeof( PROCESSENTRY32 );
//
//  // Retrieve information about the first process,
//  // and exit if unsuccessful
//  if( !Process32First( hProcessSnap, &pe32 ) )
//  {
//    printError( TEXT("Process32First") ); // show cause of failure
//    CloseHandle( hProcessSnap );          // clean the snapshot object
//    return( FALSE );
//  }
//
//  // Now walk the snapshot of processes, and
//  // display information about each process in turn
//  do
//  {
//    //_tprintf( TEXT("\n\n=====================================================" ));
//    //_tprintf( TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile );
//    //_tprintf( TEXT("\n-------------------------------------------------------" ));
//    if(stricmp(pe32.szExeFile,"lol.launcher")==0)dwRet=TRUE;
//    // Retrieve the priority class.
//    //dwPriorityClass = 0;
//    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
//    if( hProcess == NULL )
//      printError( TEXT("OpenProcess") );
//    //else
//    //{
//      //dwPriorityClass = GetPriorityClass( hProcess );
//      //if( !dwPriorityClass )
//       // printError( TEXT("GetPriorityClass") );
//      //CloseHandle( hProcess );
//    //}
//    int pid=pe32.th32ProcessID;
//    int ppid=pe32.th32ParentProcessID;
//    if(dwRet)
//    {
//             MessageBox(HWND_DESKTOP,"ERROR!!","failed to start lol.exe",MB_OK);
//             TerminateProcess(hProcess,0);
//             break;   
//    }
//    //_tprintf( TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID );
//    //_tprintf( TEXT("\n  Thread count      = %d"),   pe32.cntThreads );
//    //_tprintf( TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID );
//    //_tprintf( TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase );
//    //if( dwPriorityClass )
//      //_tprintf( TEXT("\n  Priority class    = %d"), dwPriorityClass );
//
//    // List the modules and threads associated with this process
//    //ListProcessModules( pe32.th32ProcessID );
//    //ListProcessThreads( pe32.th32ProcessID );
//
//  } while( Process32Next( hProcessSnap, &pe32 ) );
//  HWND hwnd=FindWindow(NULL,"lol*");
//  if(hwnd)
//  {
//          MessageBox(HWND_DESKTOP,"ERROR!!","failed to start lol.exe",MB_OK); 
//          DestroyWindow(hwnd);
//  }
//  CloseHandle( hProcessSnap );
//  return( dwRet );
//}
//
//
//BOOL ListProcessModules( DWORD dwPID )
//{
//  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
//  MODULEENTRY32 me32;
//
//  // Take a snapshot of all modules in the specified process.
//  hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
//  if( hModuleSnap == INVALID_HANDLE_VALUE )
//  {
//    printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
//    return( FALSE );
//  }
//
//  // Set the size of the structure before using it.
//  me32.dwSize = sizeof( MODULEENTRY32 );
//
//  // Retrieve information about the first module,
//  // and exit if unsuccessful
//  if( !Module32First( hModuleSnap, &me32 ) )
//  {
//    printError( TEXT("Module32First") );  // show cause of failure
//    CloseHandle( hModuleSnap );           // clean the snapshot object
//    return( FALSE );
//  }
//
//  // Now walk the module list of the process,
//  // and display information about each module
//  do
//  {
//    _tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
//    _tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
//    _tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
//    _tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
//    _tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
//    _tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
//    _tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );
//
//  } while( Module32Next( hModuleSnap, &me32 ) );
//
//  CloseHandle( hModuleSnap );
//  return( TRUE );
//}
//
//BOOL ListProcessThreads( DWORD dwOwnerPID ) 
//{ 
//  HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
//  THREADENTRY32 te32; 
// 
//  // Take a snapshot of all running threads  
//  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
//  if( hThreadSnap == INVALID_HANDLE_VALUE ) 
//    return( FALSE ); 
// 
//  // Fill in the size of the structure before using it. 
//  te32.dwSize = sizeof(THREADENTRY32); 
// 
//  // Retrieve information about the first thread,
//  // and exit if unsuccessful
//  if( !Thread32First( hThreadSnap, &te32 ) ) 
//  {
//    printError( TEXT("Thread32First") ); // show cause of failure
//    CloseHandle( hThreadSnap );          // clean the snapshot object
//    return( FALSE );
//  }
//
//  // Now walk the thread list of the system,
//  // and display information about each thread
//  // associated with the specified process
//  do 
//  { 
//    if( te32.th32OwnerProcessID == dwOwnerPID )
//    {
//      _tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID ); 
//      _tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri ); 
//      _tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri ); 
//      _tprintf( TEXT("\n"));
//    }
//  } while( Thread32Next(hThreadSnap, &te32 ) ); 
//
//  CloseHandle( hThreadSnap );
//  return( TRUE );
//}
//
//void printError( TCHAR* msg )
//{
//  DWORD eNum;
//  TCHAR sysMsg[256];
//  TCHAR* p;
//
//  eNum = GetLastError( );
//  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//         NULL, eNum,
//         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
//         sysMsg, 256, NULL );
//
//  // Trim the end of the line and terminate it with a null
//  p = sysMsg;
//  while( ( *p > 31 ) || ( *p == 9 ) )
//    ++p;
//  do { *p-- = 0; } while( ( p >= sysMsg ) &&
//                          ( ( *p == '.' ) || ( *p < 33 ) ) );
//
//  // Display the message
//  _tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
//}
//
//
