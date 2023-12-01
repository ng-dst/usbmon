# Lab 7: USB monitoring service

Service that responds to insertion / removal of USB devices. Reports them to log file and Event Log. \
Can block certain devices based on Hardware ID.

## Features

* Log all USB device connect / disconnect events into Event Log
* Log such events into a file (path is specified in registry)
* Block connection of different devices based on Hardware ID

## Options

* `install` &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;  Install service *
* `deny <HardwareID>` &nbsp; &nbsp;  Add device to deny list *
* `allow <HardwareID>` &nbsp;  Remove device from deny list *
* `denylist` &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;  Print deny list
* `logfile [path]` &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; Get or set* absolute path (like `C:\log.txt`) for log file
* `h`, `help` &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; Print help message

_*_ requires administrative privileges (Run as Admin)

## Usage

1. Install *_UsbMonitor_* service: `usbmon.exe install`*
2. (_optional_) Configure log file path: `usbmon.exe logfile "C:\path\to\log.txt"`*
3. Start service from *_Services_* menu or with `sc start UsbMonitor`*
4. Connect or disconnect some USB devices
5. Open log file or *_EventVwr.exe_* (go to *_Windows Logs/Application_*)
6. Notice _Hardware ID_ value of format `USB\VID_041D&PID_C584&REV_0409`
7. (_optional_) To block device, use _Hardware ID_: `usbmon.exe deny "USB\VID_041D&PID_C584&REV_0409"`*
8. (_optional_) To unblock device, use _Hardware ID_: `usbmon.exe allow "USB\VID_041D&PID_C584&REV_0409"`* and enable it in *_Device Manager_* (if needed)  

_*_ requires administrative privileges (Run as Admin)

## Registry

Service stores its config in registry. Base path is `HKLM\SYSTEM\CurrentControlSet\Services\UsbMonitor\ `.

Under this key:
* `Parameters\ `
  * `LogFile` (_REG_SZ_) - Path to log file, like `C:\path\to\log.txt`
  * `DenyList\ `
    * `USB_VID_041D&PID_C584&REV_0409` (_DWORD_, always 1) - if value exists, _Hardware ID_ is in deny list
    * < ... >

If some keys don't exist, they are created automatically.\
If `LogFile` is not set, logs are sent to *_Event Log_* only.

## Improvements

Some Chinese flash drives may have strange Unicode hardware IDs, which can be confusing for Registry if set as a value name.

It can be solved by storing hex-values for hardware IDs (possibly MD5 of ID string) with corresponding user-friendly name of rule.

#### Example

This scheme allows for quick dereference by hardware ID. Just calculate hash and check if it's there.

* `DenyList\ `
  * `9d2cff9017b505e8beb206df9fd2efb3` (_REG_SZ_) = `<rule_name>`
  * < ... >

#### Alternative

This scheme allows for quick dereference by rule name. Also guarantees that rule names are unique, and it generally looks more natural.

* `DenyList\ `
  * `<rule_name>` (_REG_SZ_) = `9d2cff9017b505e8beb206df9fd2efb3`
  * < ... >

#### Supposed usage

MD5 of Hardware IDs should appear in log file and Event Log instead of raw Hardware IDs.

```
usbmon.exe deny <name> <hwId_MD5>
usbmon.exe allow <name> <hwId_MD5>
```

I don't know if I implement this in near future, though I hope so &nbsp;ฅ ^•ﻌ•^ ฅ &nbsp;°。

## References

* Service template: https://learn.microsoft.com/en-us/windows/win32/services/svc-cpp
* Device notifications: https://www.codeproject.com/Articles/15612/Receiving-Device-Event-Notification-in-Windows-Ser
  * *_Note_*: do not set `DEVICE_NOTIFY_ALL_INTERFACE_CLASSES`, instead specify `classguid` (see `SvcMain()` in `service.c`)
* Working with registry: https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexa 
* Handle USB events: https://learn.microsoft.com/en-us/answers/questions/985652/getting-instance-id-of-usb-drive-from-registerdevi


      ∩―――――――――――∩                .      .
      ||   ∧  ﾍ　 ||    . ゜ ﾟ.  ゜     ✧    。
      ||  (* ´ ｰ`) ZZzz     °   ★   ﾟ     ゜    ✧
      |ﾉ^⌒⌒づ`￣￣ ＼             ﾟ。 ﾟ  ★  ｡ﾟ 
      (  ノ　　⌒  ヽ  ＼         ﾟ    ★    。  
      ＼　　 ||￣￣￣￣￣||               ﾟ
      　 ＼,ﾉ||         || 