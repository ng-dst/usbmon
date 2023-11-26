#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include "../include/devinfo.h"


LPSTR GetDevicePropByHandle(HDEVINFO deviceInfoHandle, PSP_DEVINFO_DATA deviceInfoData, DWORD property) {
    /**
     * @brief Call SetupDiDeviceRegistryProperty to get the property
     */
    DWORD descByteSize = 0;
    BOOL success;

    // Get the size of the friendly device name
    SetupDiGetDeviceRegistryProperty(deviceInfoHandle,
                                     deviceInfoData,
                                     property,
                                     NULL,
                                     NULL,
                                     0,
                                     &descByteSize);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return NULL;

    // Prepare a buffer for the friendly device name (plus space for a null terminator)
    TCHAR* valueBuffer = calloc(descByteSize,  sizeof(TCHAR));
    if (!valueBuffer) return NULL;

    success = SetupDiGetDeviceRegistryProperty(deviceInfoHandle, deviceInfoData, property, NULL, valueBuffer, descByteSize, &descByteSize);
    if (success) return valueBuffer;

    free(valueBuffer);
    return NULL;
}


LPSTR GetDevicePropByName(LPCSTR aDeviceInterfaceDbccName, DWORD property) {
    /**
     * @brief Get device property by dbcc_name
     *
     * @details Creates Device Info List, opens Interface by name, cycles through device branch to find the property
     */
    HDEVINFO deviceInfoHandle;
    SP_DEVINFO_DATA deviceInfoData;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

    // Create a new empty "device info set"
    deviceInfoHandle = SetupDiCreateDeviceInfoList(NULL, 0);
    if (deviceInfoHandle == INVALID_HANDLE_VALUE)
        return NULL;

    // Add "aDeviceInterfaceDbccName" to the device info set
    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiOpenDeviceInterface(deviceInfoHandle, aDeviceInterfaceDbccName, 0, &deviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoHandle);
        return NULL;
    }

    DWORD memberIndex = 0;
    while (TRUE) {

        // Get device info that corresponds to the next memberIndex
        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(deviceInfoHandle, memberIndex, &deviceInfoData)) {
            // The enumerator is exhausted when SetupDiEnumDeviceInfo returns false
            break;
        }

        // Try to get property for that device info
        TCHAR *prop_value = GetDevicePropByHandle(deviceInfoHandle, &deviceInfoData, property);

        if (prop_value) {
            SetupDiDestroyDeviceInfoList(deviceInfoHandle);
            return prop_value;
        }

        memberIndex++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoHandle);
    return NULL;
}


WINBOOL DisableUsbDevice(LPCTSTR aDeviceInterfaceDbccName) {
    /**
     * @brief Disable device by dbcc_name
     *
     * @details Similar to Device Manager's "disable" option. To enable again, use Device Manager
     */
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

    // Create a device information set for the specified device interface
    deviceInfoSet = SetupDiCreateDeviceInfoList(NULL, 0);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return FALSE;  // Failed to get the device information set

    // Add "aDeviceInterfaceDbccName" to the device info set
    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiOpenDeviceInterface(deviceInfoSet, aDeviceInterfaceDbccName, 0, &deviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;
    }

    // Retrieve the device interface data
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(deviceInfoSet, 0, &deviceInfoData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to enumerate device info
    }

    // Set the device installation parameters
    SP_CLASSINSTALL_HEADER classInstallHeader;
    classInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    classInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    SP_PROPCHANGE_PARAMS propChangeParams;
    propChangeParams.ClassInstallHeader = classInstallHeader;
    propChangeParams.StateChange = DICS_DISABLE;
    propChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    propChangeParams.HwProfile = 0;

    if (!SetupDiSetClassInstallParams(deviceInfoSet, &deviceInfoData, &propChangeParams.ClassInstallHeader, sizeof(SP_PROPCHANGE_PARAMS))) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to set class install parameters
    }

    // Call the class installer to disable the device
    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, deviceInfoSet, &deviceInfoData)) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return FALSE;  // Failed to call class installer
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return TRUE;
}