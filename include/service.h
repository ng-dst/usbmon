#ifndef LAB7_SERVICE_H
#define LAB7_SERVICE_H

#include <windows.h>


void SvcInstall();
DWORD WINAPI SvcCtrlHandlerEx(DWORD, DWORD, LPVOID, LPVOID);
void WINAPI SvcMain(DWORD, LPTSTR*);

void ReportSvcStatus(DWORD, DWORD, DWORD);
void SvcInit(DWORD, LPTSTR*);

#endif //LAB7_SERVICE_H
