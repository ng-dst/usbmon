#ifndef LAB7_USBMON_H
#define LAB7_USBMON_H

#include <windows.h>
#include <dbt.h>

LPSTR WINAPI UnicodeToAnsi(LPVOID unicodeStr);
void WINAPI ProcessUsbEvent(PDEV_BROADCAST_DEVICEINTERFACE pDevInf, DWORD dwEventType);

#endif //LAB7_USBMON_H
