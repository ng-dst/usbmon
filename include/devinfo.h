#ifndef LAB7_DEVINFO_H
#define LAB7_DEVINFO_H

#include <setupapi.h>

LPSTR GetDevicePropByHandle(HDEVINFO deviceInfoHandle, PSP_DEVINFO_DATA deviceInfoData, DWORD property);
LPSTR GetDevicePropByName(LPCSTR aDeviceInterfaceDbccName, DWORD property);

WINBOOL DisableUsbDevice(LPCTSTR aDeviceInterfaceDbccName);

#endif //LAB7_DEVINFO_H
