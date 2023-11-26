#ifndef LAB7_EVENT_H
#define LAB7_EVENT_H

#include <windows.h>

void SvcReportEvent(WORD wType, LPCTSTR szEventMsg);
void SvcLogEventToFile(WORD wType, LPCTSTR szEventMsg);

#endif //LAB7_EVENT_H
