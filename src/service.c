#include <stdio.h>
#include <windows.h>
#include <dbt.h>
#include <usbiodef.h>
#include <tchar.h>
#include "service.h"
#include "usbmon.h"
#include "event.h"


SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;


int SvcInstall() {
    /**
     * @brief Install UsbMonitor service in SCM database
     *
     * @details https://learn.microsoft.com/en-us/windows/win32/services/svc-cpp
     */
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR path[MAX_PATH] = {0};
    int res;

    // path = \" <name> \"
    if (!GetModuleFileName(NULL, path+1, MAX_PATH-2)) {
        res = GetLastError();
        printf("Could not install service (%d)\n", res);
        return res;
    }
    path[0] = '"';
    path[strlen(path)+1] = '\0';
    path[strlen(path)] = '"';

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        res = GetLastError();
        if (res == ERROR_ACCESS_DENIED) printf("Access denied. Try to run as administrator\n");
        else printf("OpenSCManager failed (%d)\n", res);
        return res;
    }

    schService = CreateService(
            schSCManager,
            SVCNAME,
            SVCNAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            path,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );
    if (!schService) {
        res = GetLastError();
        if (res == ERROR_SERVICE_EXISTS) printf("Service is already installed\n");
        else printf("CreateService failed (%d)\n", res);
        CloseServiceHandle(schSCManager);
        return res;
    }

    printf("Service installed successfully\n");

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return ERROR_SUCCESS;
}


int SvcUninstall() {
    /**
     * @brief Uninstall UsbMonitor service from SCM database
     */

    SC_HANDLE schService;
    SC_HANDLE schSCManager;
    DWORD res;

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        res = GetLastError();
        if (res == ERROR_ACCESS_DENIED) printf("Access denied. Try to run as administrator\n");
        else printf("OpenSCManager failed (%d)\n", res);
        return res;
    }

    schService = OpenService(
            schSCManager,
            SVCNAME,
            SERVICE_ALL_ACCESS);

    if (!schService) {
        res = GetLastError();
        if (res == ERROR_SERVICE_DOES_NOT_EXIST) printf("Service is not installed\n");
        else printf("OpenService failed (%d)\n", res);
        CloseServiceHandle(schSCManager);
        return res;
    }

    res = ERROR_SUCCESS;

    if (DeleteService(schService))
        printf("Service uninstalled\n");
    else {
        res = GetLastError();
        printf("Could not uninstall service (%d). Stop the service before uninstalling\n", res);
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return res;
}


void WINAPI SvcMain(DWORD argc, LPTSTR *argv) {
    /**
     * @brief Entry point for service. Registers for device notification and continues to SvcInit()
     */

    // Register handle function for service
    gSvcStatusHandle = RegisterServiceCtrlHandlerEx(SVCNAME, SvcCtrlHandlerEx, NULL);
    if (!gSvcStatusHandle) {
        SvcReportEvent(EVENTLOG_ERROR_TYPE, "RegisterServiceCtrlHandler failed");
        return;
    }

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = EXIT_SUCCESS;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Device notification register
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
    ZeroMemory(&notificationFilter, sizeof(notificationFilter));
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    HDEVNOTIFY hDeviceNotify = RegisterDeviceNotification(gSvcStatusHandle, &notificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE);
    if (!hDeviceNotify) {
        SvcReportEvent(EVENTLOG_ERROR_TYPE, "hDeviceNotify is NULL");
        return;
    }

    // Continue execution
    SvcInit(argc, argv);

    UnregisterDeviceNotification(hDeviceNotify);
}


void SvcInit(DWORD argc, LPTSTR *argv) {
    /**
     * @brief Service main code (do nothing). Wait for stop signal
     */

    // Create an event for SvcCtrlHandler
    ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ghSvcStopEvent) {
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Report as running
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
    SvcReportEvent(EVENTLOG_INFORMATION_TYPE, "Service is running");

    // Stop service when stop signal is received
    WaitForSingleObject(ghSvcStopEvent, INFINITE);
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void ReportSvcStatus(DWORD state, DWORD exit_code, DWORD wait_hint) {
    /**
     * @brief Set and report service status to SCM
     */
    static DWORD checkPoint = 1;

    gSvcStatus.dwCurrentState = state;
    gSvcStatus.dwWin32ExitCode = exit_code;
    gSvcStatus.dwWaitHint = wait_hint;

    if (state == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ( (state == SERVICE_RUNNING) ||
         (state == SERVICE_STOPPED) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = checkPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}


DWORD WINAPI SvcCtrlHandlerEx(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    /**
     * @brief Called by SCM. Handle the requested control code.
     */
    PDEV_BROADCAST_HDR pHdr;
    PDEV_BROADCAST_DEVICEINTERFACE pDevInf;

    switch (dwCtrl) {

        case SERVICE_CONTROL_STOP:
            // Signal to stop service
            ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            SetEvent(ghSvcStopEvent);
            ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
            // Signal about device changes
            pHdr = (PDEV_BROADCAST_HDR) lpEventData;
            if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE) pHdr;

                // Call my function to work with device event
                ProcessUsbEvent(pDevInf, dwEventType);
            }
            break;

        default:
            break;
    }

    return NO_ERROR;
}
