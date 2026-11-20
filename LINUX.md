# Linux host tool

This note describes the Linux port of the selected source `abeacdaccde0b80a74e61903843756be9e628c6d`. It does not change the Windows or macOS implementations.

The host still starts `eve_crashmon` from its own directory. The public Crashpad package builds that program as `tools/crashpad_handler`, and the imported `crashpad::handler` target in this prefix has no location. A deployment copies `crashpad_handler` to `eve_crashmon` beside `exefile`. The client aborts if that file is missing. This port does not rename the handler or change the upload URL. Verification starts the handler only with uploads disabled.

`__cdecl` is defined as empty on Linux. Windows keeps the compiler's calling convention. Application data uses `XDG_DATA_HOME` when it is set, and otherwise `$HOME/.local/share`. Log files use the existing `CCP/EVE` suffix. `IsSupportedOSVersion` returns true because the existing source has no Linux version table. Windows still requires Windows 10 or newer, and macOS still requires 10.15 or newer.

`exefile /?` prints usage and exits before Blue is loaded. With no Blue library beside the tool, `exefile` exits 5 after reporting that the release flavor could not be loaded.
