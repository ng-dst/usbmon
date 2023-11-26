#include <tchar.h>
#include <stdio.h>
#include "../include/event.h"
#include "../include/cfg.h"

#define SVC_EVENT_CODE 0
#define BUF_LEN 512


void SvcReportEvent(WORD wType, LPCTSTR szEventMsg) {
    /**
     * @brief Log message to Event Log
     */
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];

    hEventSource = RegisterEventSource(NULL, SVCNAME);

    if (hEventSource) {
        lpszStrings[0] = SVCNAME;
        lpszStrings[1] = szEventMsg;

        ReportEvent(hEventSource,        // event log handle
                    wType,                         // event type
                    0,                   // event category
                    SVC_EVENT_CODE,      // event identifier
                    NULL,                 // no security identifier
                    2,                 // size of lpszStrings array
                    0,                  // no binary data
                    lpszStrings,          // array of strings
                    NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}

void SvcLogEventToFile(WORD wType, LPCTSTR szEventMsg) {
    /**
     * @brief Log message to File (LogFile defined in registry)
     */
    LPTSTR logFilePath = GetLogFilePath();
    if (!logFilePath) return;

    DWORD bw;
    TCHAR buf[BUF_LEN] = {0};

    SYSTEMTIME st;
    GetLocalTime(&st);

    HANDLE hFile = CreateFile(logFilePath, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        sprintf(buf, "Failed to open / create log file (%lu)", GetLastError());
        SvcReportEvent(EVENTLOG_WARNING_TYPE, buf);
        free(logFilePath);
        return;
    }

    snprintf(buf, BUF_LEN, "[%02hu/%02hu/%04hu %02hu:%02hu:%02hu] (%d)  %s\r\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, wType, szEventMsg);

    SetFilePointer(hFile, 0, 0, FILE_END);
    BOOL success = WriteFile(hFile, buf, _tcslen(buf) * sizeof(TCHAR), &bw, 0);
    if (!success) {
        sprintf(buf, "Failed to write to log file (%lu)", GetLastError());
        SvcReportEvent(EVENTLOG_WARNING_TYPE, buf);
    }

    free(logFilePath);
    CloseHandle(hFile);
}