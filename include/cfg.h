#ifndef LAB7_CFG_H
#define LAB7_CFG_H

#include <windows.h>

WINBOOL AddDeviceToDenyList(LPCTSTR hardwareId);
WINBOOL RemoveDeviceFromDenyList(LPCTSTR hardwareId);

LPTSTR GetLogFilePath();
WINBOOL SetLogFilePath(LPCTSTR path);

WINBOOL InitDenyList();
WINBOOL PrintDenyList();

WINBOOL IsInDenyList(LPCTSTR hardwareId);

#endif //LAB7_CFG_H
