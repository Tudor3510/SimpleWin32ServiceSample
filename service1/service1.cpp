// service1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>

FILE* debugFile;

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

char SERVICE_NAME_CHAR[] = "My Sample Service";
LPSTR SERVICE_NAME = SERVICE_NAME_CHAR;

int _tmain(int argc, TCHAR* argv[])
{
    debugFile = fopen("C:\\Users\\windows\\Desktop\\Debug-File.txt", "w");
    if (debugFile == NULL)
    {
        fprintf(stderr, "Error while opening the debug file\n");
        return 0;
    }

    fprintf(debugFile, "We are in the main function\n");

    SERVICE_TABLE_ENTRYA ServiceTable[] =
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTIONA)ServiceMain},
        {NULL, NULL}
    };

    //Connecting the main thread of a service process to the service control manager
    if (StartServiceCtrlDispatcherA(ServiceTable))
    {
        fprintf(debugFile, "The service ctrl dispatcher worked properly\n");
    }
    else
    {
        fprintf(debugFile, "Failed to start the service ctrl dispatcher\n");
        return GetLastError();
    }

    fclose(debugFile);

    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandlerA(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle)
    {
        fprintf(debugFile, "Registered service ctrl handler successfully\n");
    }
    else
    {
        fprintf(debugFile, "Failed to register the service ctrl handler\n");
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;                  // may be replaced by SERVICE_USER_OWN_PROCESS !! Se SERVICE_INTERACTIVE_PROCESS for interacting with desktop
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
    {
        fprintf(debugFile, "SetServiceStatus worked successfully. The service controller should now that we are starting\n");
    }
    else
    {
        fprintf(debugFile, "SetServiceStatus failed. The service controller does not know that we are starting\n");
    }

    /*
     * Perform tasks necessary to start the service here
     */

     // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent)
    {
        fprintf(debugFile, "The ServiceStopEvent was created successfuly\n");
    }
    else
    {
        fprintf(debugFile, "The ServiceStopEvent could not be created....telling the service to stop\n");
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
        {
            fprintf(debugFile, "SetServiceStatus worked. The service controller knows that we have stopped because of the CreateEvent error\n");
        }
        else
        {
            fprintf(debugFile, "SetServiceStatus failed. The service controller does not know that we have stopped because of the CreateEvent error\n");
        }
        return;
    }

    // Tell the service controller we have started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 2;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
    {
        fprintf(debugFile, "SetServiceStatus worked. The service controller knows that we have started\n");
    }
    else
    {
        fprintf(debugFile, "SetServiceStatus failed. The service controller does not know that we have started\n");
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    if (hThread == INVALID_HANDLE_VALUE)
    {
        fprintf(debugFile, "The working thread could not be created\n");
        return;
    }

    // Wait until our worker thread exits
    WaitForSingleObject(hThread, INFINITE);
    /*
     * Perform any cleanup tasks
     */

    if (CloseHandle(g_ServiceStopEvent))
    {
        fprintf(debugFile, "The ServiceStopEvent was closed successfuly\n");
    }
    else
    {
        fprintf(debugFile, "Could not close the ServiceStopEvent\n");
    }

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 4;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
    {
        fprintf(debugFile, "SetServiceStatus worked. The service controller knows that we have stopped\n");
    }
    else
    {
        fprintf(debugFile, "SetServiceStatus failed. The service controller does not know that we have stopped\n");
    }

    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 3;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus))
        {
            fprintf(debugFile, "SetServiceStatus worked. The service controller knows that we are stopping\n");
        }
        else
        {
            fprintf(debugFile, "SetServiceStatus failed. The service controller does not know that we are stopping\n");
        }

        // This will signal the service thread to start shutting down
        if (SetEvent(g_ServiceStopEvent))
        {
            fprintf(debugFile, "The ServiceStopEvent was set successfully\n");
        }
        else
        {
            fprintf(debugFile, "The ServiceStopEvent could not be set\n");
        }

        break;

    default:
        break;
    }
}


// This is the worker thread
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
// Do stuff.  This will be the first function called on the new thread.
// When this function returns, the thread goes away.  See MSDN for more details.
    fprintf(debugFile, "This is the beginnig of the worker thread\n");

    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        fprintf(debugFile, "\nPrinting once a second\n");
        fflush(debugFile);
        Sleep(1000);
    }

    return ERROR_SUCCESS;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
