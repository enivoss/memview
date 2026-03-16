# memview

real-time process memory inspector for Windows. dark terminal UI, no bloat.

```
  memview by enivoss
  ------------------------------------------------------------------------
  PID     PROCESS                          WORKING SET     PRIVATE
  ------------------------------------------------------------------------
  1234    chrome.exe                       512.00 MB       488.00 MB
  5678    explorer.exe                     84.20 MB        61.00 MB
  ...
```

## what it does

reads live memory usage from every running process via Win32 API. shows working set and private bytes. color codes by usage: red for heavy hitters, yellow for mid-range, white for everything else.

## controls

| key | action |
|-----|--------|
| UP / DOWN | navigate |
| M | sort by memory |
| N | sort by name |
| R | refresh process list |
| Q | quit |

## build

requires MSVC (Visual Studio). run:

```
build.bat
```

outputs `memview.exe` in the same directory.

## why

task manager is cluttered. this isn't.
