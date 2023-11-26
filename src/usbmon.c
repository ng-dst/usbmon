#include <stdio.h>
#include "../include/usbmon.h"
#include "../include/devinfo.h"
#include "../include/event.h"
#include "../include/cfg.h"

#define BUF_LEN 512


LPSTR WINAPI UnicodeToAnsi(LPVOID unicodeStr) {
    /**
     * @brief Convert Unicode string to ANSI using wc2mb
     */

    // Get required string length and allocate buffer
    int mbsLength = WideCharToMultiByte(CP_ACP, 0, unicodeStr, -1, 0, 0, 0, 0);

    TCHAR *mbsString = calloc(mbsLength, sizeof(TCHAR));
    if (!mbsString) return NULL;

    // Get actual ANSI string from Unicode
    WideCharToMultiByte(CP_ACP, 0, unicodeStr, -1, mbsString, mbsLength, 0, 0);
    return mbsString;
}


void WINAPI ProcessUsbEvent(PDEV_BROADCAST_DEVICEINTERFACE pDevInf, DWORD dwEventType) {
    /**
     * @brief Work with USB device event (connection / disconnection)
     *
     * @details
     *  On connect: Check if hardware ID is in Deny List. If it is (or has no hwId?), disable it. Report.
     *  On disconnect: Report.
     */
    TCHAR buf[BUF_LEN] = {0};

    // dbcc_name is returned as Unicode string, need to convert to ANSI
    LPSTR dbccName = UnicodeToAnsi(pDevInf->dbcc_name);
    if (!dbccName) return;

    // Get Friendly Name:  either FriendlyName or DeviceDesc.
    TCHAR *friendlyName = GetDevicePropByName(dbccName, SPDRP_FRIENDLYNAME);
    if (!friendlyName)
        friendlyName = GetDevicePropByName(dbccName, SPDRP_DEVICEDESC);

    // Get Hardware ID
    TCHAR *hwId = GetDevicePropByName(dbccName, SPDRP_HARDWAREID);

    // Get Manufacturer
    TCHAR *mfg = GetDevicePropByName(dbccName, SPDRP_MFG);

    // Connected device. Check for deny list, Report
    if (dwEventType == DBT_DEVICEARRIVAL) {

        // In deny list (or missing hwId) => block and report
        if (!hwId || IsInDenyList(hwId)) {

            if (DisableUsbDevice(dbccName))
                 snprintf(buf, BUF_LEN, "Blocked USB device (in deny list): %s (%s, HardwareID: %s)",
                    friendlyName ? friendlyName : "Unknown device",
                    mfg ? mfg : "Unknown",
                    hwId ? hwId : "Unknown");

            else snprintf(buf, BUF_LEN, "Could not block USB device: %s (%s, HardwareID: %s)",
                    friendlyName ? friendlyName : "Unknown device",
                    mfg ? mfg : "Unknown",
                    hwId ? hwId : "Unknown");

            SvcReportEvent(EVENTLOG_WARNING_TYPE, buf);
            SvcLogEventToFile(EVENTLOG_WARNING_TYPE, buf);
        }

        // Not in deny list => just report
        else {
            snprintf(buf, BUF_LEN, "Connected USB device: %s (%s, HardwareID: %s)",
                    friendlyName ? friendlyName : "Unknown device",
                    mfg ? mfg : "Unknown",
                    hwId);

            SvcReportEvent(EVENTLOG_INFORMATION_TYPE, buf);
            SvcLogEventToFile(EVENTLOG_INFORMATION_TYPE, buf);
        }
    }

    // Disconnected device. Report
    else if (dwEventType == DBT_DEVICEREMOVECOMPLETE) {
        snprintf(buf, BUF_LEN, "Disconnected USB device: %s (%s, HardwareID: %s)",
                friendlyName ? friendlyName : "Unknown device",
                mfg ? mfg : "Unknown",
                hwId ? hwId : "Unknown");

        SvcReportEvent(EVENTLOG_INFORMATION_TYPE, buf);
        SvcLogEventToFile(EVENTLOG_INFORMATION_TYPE, buf);
    }

    free(dbccName);
    if (hwId) free(hwId);
    if (mfg) free(mfg);
    if (friendlyName) free(friendlyName);
}
