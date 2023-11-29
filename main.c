#include <windows.h>
#include <stdio.h>
#include "include/service.h"
#include "include/event.h"
#include "include/cfg.h"

#pragma comment(lib, "advapi32.lib")


int main(int argc, char** argv) {
    // Initialize registry paths *
    InitDenyList();

    // "install" - Install service *
    if (argc > 1 && !strcmpi(argv[1], "install")) {
        SvcInstall();
        return EXIT_SUCCESS;
    }

    // "deny <hwId>" - Add device to deny list *
    if (argc > 2 && !strcmpi(argv[1], "deny")) {
        if (AddDeviceToDenyList(argv[2])) {
            printf("OK\n");
            return EXIT_SUCCESS;
        }
        else {
            printf("Failed. Try to run as admin\n");
            return EXIT_FAILURE;
        }
    }

    // "allow <hwId>" - Remove device from deny list *
    if (argc > 2 && !strcmpi(argv[1], "allow")) {
        if (RemoveDeviceFromDenyList(argv[2])) {
            printf("OK\n");
            return EXIT_SUCCESS;
        }
        // Error message is in RemoveDeviceFromDenyList
        else return EXIT_FAILURE;
    }

    // "denylist" - Print deny list
    if (argc > 1 && !strcmpi(argv[1], "denylist"))
        return PrintDenyList();

    // "logfile [path]" - Get / set absolute path for log file
    if (argc > 1 && !strcmpi(argv[1], "logfile")) {
        // no path specified, print existing
        if (argc == 2) {
            TCHAR* path = GetLogFilePath();
            if (!path) printf("No log path set\n");
            else {
                printf("%s\n", path);
                free(path);
            }
            return EXIT_SUCCESS;
        }
        // path specified, set path *
        else if (SetLogFilePath(argv[2])) {
            printf("OK\n");
            return EXIT_SUCCESS;
        }
        else {
            printf("Failed. Try to run as administrator\n");
            return EXIT_FAILURE;
        }
    }

    // "h", "help" - Print help message
    if (argc > 1 && (!strcmpi(argv[1], "h") ||
                     !strcmpi(argv[1], "help"))) {
        printf("Lab 7: USB Monitor service\n"
               "Available commands:\n"
               "\tinstall                -  Install service (run as admin)\n"
               "\tdeny <HardwareID>      -  Add device to deny list\n"
               "\tallow <HardwareID>     -  Remove device from deny list\n"
               "\tdenylist               -  Print deny list\n"
               "\tlogfile [path]         -  Get or set absolute path (like C:\\log.txt) for log file\n"
               "\th, help                -  Print this message\n");
        return EXIT_SUCCESS;
    }

    if (argc > 1) {
        printf("Unknown command. Use 'h' or 'help' for help\n");
        return EXIT_FAILURE;
    }

    // No args. Run service
    SERVICE_TABLE_ENTRY DispatchTable[] =
            {
            {SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain},
            {NULL, NULL}
            };

    if (!StartServiceCtrlDispatcher(DispatchTable)) {
        SvcReportEvent(EVENTLOG_ERROR_TYPE, "StartServiceCtrlDispatcher failed");
        // Could just run manually from terminal?
        printf("Cannot run manually without arguments. Use 'h' or 'help' for help\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
