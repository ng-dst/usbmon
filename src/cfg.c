/**

 base path:   HKLM\SYSTEM\CurrentControlSet\Services\UsbMonitor\
    - \Parameters                  - subkey. if not exists, create
    - \Parameters\LogFile          - REG_SZ. optional (if none, do not log to file)
    - \Parameters\DenyList         - subkey. if not exists, create
    - \Parameters\DenyList\<hwId>  - REG_DWORD. if present -> hwId in deny list

 */

#include "../include/cfg.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define BASE_PATH _T("SYSTEM\\CurrentControlSet\\Services\\") SVCNAME
#define PARAMETERS_PATH BASE_PATH _T("\\Parameters")
#define DENY_LIST_PATH PARAMETERS_PATH _T("\\DenyList")
#define LOG_FILE _T("LogFile")


void TransformRegistryKey(LPTSTR str) {
    /**
     * @brief Replace any invalid characters with underscore
     */
    TCHAR* invalidChars = _T("\\/:*\"<>|");
    TCHAR* replacementChar = _T("_");

    size_t strLength = _tcslen(str);
    size_t invalidCharsCount = _tcslen(invalidChars);

    for (size_t i = 0; i < strLength; i++)
        for (size_t j = 0; j < invalidCharsCount; j++)
            if (str[i] == invalidChars[j])
            {
                str[i] = replacementChar[0];
                break;
            }
}


WINBOOL AddDeviceToDenyList(LPCTSTR hardwareId) {
    /**
     * @brief Create DWORD registry key under DenyList/
     */
    HKEY denyListKey;
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,DENY_LIST_PATH,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&denyListKey,NULL))
        return FALSE;  // Failed to create or open deny list key

    LPTSTR hardwareIdClean = strdup(hardwareId);
    if (!hardwareIdClean) return FALSE;

    TransformRegistryKey(hardwareIdClean);
    DWORD value = 1;
    WINBOOL result = (RegSetValueEx(denyListKey,
                                    hardwareIdClean,
                                    0,
                                    REG_DWORD,
                                    (LPVOID) &value,
                                    sizeof(DWORD)
                                    ) == ERROR_SUCCESS);

    RegCloseKey(denyListKey);
    free(hardwareIdClean);
    return result;
}


WINBOOL RemoveDeviceFromDenyList(LPCTSTR hardwareId) {
    /**
     * @brief Remove key from DenyList/
     * @note Outputs error messages itself
     */
    HKEY denyListKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, DENY_LIST_PATH, 0, KEY_WRITE, &denyListKey)) {
        printf("Failed. Try to run as admin\n");
        return FALSE;  // Failed to open deny list key
    }

    LPTSTR hardwareIdClean = strdup(hardwareId);
    if (!hardwareIdClean) return FALSE;

    TransformRegistryKey(hardwareIdClean);
    int result = RegDeleteValue(denyListKey, hardwareIdClean);
    if (result == ERROR_FILE_NOT_FOUND) printf("Device '%s' is not in deny list\n", hardwareId);
    else if (result != ERROR_SUCCESS) printf("Failed. Try to run as admin\n");

    free(hardwareIdClean);
    RegCloseKey(denyListKey);
    return result == ERROR_SUCCESS;
}


LPTSTR GetLogFilePath() {
    /**
     * @brief Read REG_SZ Parameters/LogFile
     */
    HKEY parametersKey;
    DWORD valueType;
    DWORD bufferSize = 0;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, PARAMETERS_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &parametersKey, NULL))
        return FALSE;  // Failed to create or open parameters key


    if (ERROR_SUCCESS != RegQueryValueEx(parametersKey, LOG_FILE, NULL, &valueType, NULL, &bufferSize)) {
        RegCloseKey(parametersKey);
        return NULL;  // Failed to get log file value size
    }

    if (valueType != REG_SZ) {
        RegCloseKey(parametersKey);
        return NULL;  // Log file value is not REG_SZ
    }

    LPTSTR filePath = calloc(bufferSize, sizeof(TCHAR));
    if (!filePath) {
        RegCloseKey(parametersKey);
        return NULL;
    }

    if (ERROR_SUCCESS != RegQueryValueEx(parametersKey, LOG_FILE, NULL, NULL, (LPVOID) filePath, &bufferSize)) {
        free(filePath);
        RegCloseKey(parametersKey);
        return NULL;  // Failed to get log file value
    }

    RegCloseKey(parametersKey);
    return filePath;
}


WINBOOL SetLogFilePath(LPCTSTR path) {
    /**
     * @brief Create or set REG_SZ at Parameters/LogFile
     */
    HKEY parametersKey;
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, PARAMETERS_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &parametersKey, NULL))
        return FALSE;  // Failed to create or open parameters key

    WINBOOL result = (RegSetValueEx(parametersKey,
                                    LOG_FILE,
                                    0,
                                    REG_SZ,
                                    (LPVOID) path,
                                    (_tcslen(path) + 1) * sizeof(TCHAR)
                                    ) == ERROR_SUCCESS);

    RegCloseKey(parametersKey);
    return result;
}


WINBOOL InitDenyList() {
    /**
     * @brief Create sub-keys Parameters, Parameters/DenyList
     */
    HKEY denyListKey;

    // Create (if needed) Parameters
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, PARAMETERS_PATH, 0, KEY_READ, &denyListKey)) {
        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,PARAMETERS_PATH,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&denyListKey,NULL))
            return FALSE;  // Failed to create or open deny list key
        RegCloseKey(denyListKey);
    }

    // Create (if needed) Parameters/DenyList
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, DENY_LIST_PATH, 0, KEY_READ, &denyListKey)) {
        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,DENY_LIST_PATH,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&denyListKey,NULL))
            return FALSE; // Failed to create or open deny list key
        RegCloseKey(denyListKey);
    }

    return TRUE;  // Deny list was initialized
}


WINBOOL PrintDenyList() {
    /**
     * @brief Print Hardware IDs from deny list
     */
    HKEY denyListKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, DENY_LIST_PATH, 0, KEY_READ, &denyListKey))
        return FALSE;  // Failed to open deny list key

    DWORD dwIndex = 0;
    TCHAR szSubkey[MAX_PATH];
    DWORD cbSubkey = MAX_PATH;

    printf("List of banned devices:\n");
    while (ERROR_SUCCESS == RegEnumValue(denyListKey, dwIndex, szSubkey, &cbSubkey, NULL, NULL, NULL, NULL)) {
        printf(_T("%s\n"), szSubkey);
        dwIndex++;
        cbSubkey = sizeof(szSubkey);
    }

    RegCloseKey(denyListKey);
    return TRUE;
}


#define cleanupRetFalse()           \
    do {                            \
        RegCloseKey(denyListKey);   \
        free(hardwareIdClean);      \
        return FALSE;               \
    } while (0)

WINBOOL IsInDenyList(LPCTSTR hardwareId) {
    /**
     * @brief Boolean value, whether hardwareId is in deny list
     */
    HKEY denyListKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, DENY_LIST_PATH, 0, KEY_READ, &denyListKey))
        return FALSE;  // Failed to open deny list key

    DWORD valueType;
    DWORD bufferSize = 0;

    LPTSTR hardwareIdClean = _tcsdup(hardwareId);
    if (!hardwareIdClean) return FALSE;

    TransformRegistryKey(hardwareIdClean);

    if (ERROR_SUCCESS != RegQueryValueEx(denyListKey, hardwareIdClean, NULL, &valueType, NULL, &bufferSize))
        cleanupRetFalse();

    if (valueType != REG_DWORD) cleanupRetFalse();

    DWORD value = 0;
    if (ERROR_SUCCESS != RegQueryValueEx(denyListKey, hardwareIdClean, NULL, NULL, (LPVOID) &value, &bufferSize)
     || value == 0)
        cleanupRetFalse();

    RegCloseKey(denyListKey);
    free(hardwareIdClean);
    return TRUE;
}
