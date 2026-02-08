**Warning: Lacks thorough testing!!!**

**You’re still helpful, even if it's just for translations.**

**Pending: Native English speakers (or fluent speakers) needed to rewrite i18n and README_EN.md.**

**Pending: Translation: Translated by Gemini-3-Flash; verification required.**

# L2K (Load 2 KeyboardLED) v3.2.2

Ultra-lightweight Linux system monitor. Gone are the complex GUIs—we use LEDs for display.

No LEDs? You’ve got three on your keyboard. No keyboard backlights? Try your NIC or disk indicators—any kernel-managed LED will do.

If you lack even those, you're on your own.
## Features
- **Real-time Monitoring**: Track CPU, RAM, and Disk utilization (multi-disk support included).
- **Flashing**: LED indicators flash based on system load.
- **Customizable**: Customize LED mapping, thresholds, and flash frequency via `/etc/l2k.conf` .
- **Silent Mode**: Runs quietly in the background by default.
- **Multilingual Support**: Supports both Chinese and English.
## Usage
### Running
**Root privileges** are required to write to the LED devices under `/sys/class/leds/` .

By default, the tool runs in **background mode**.

Supported Command Line Arguments:
- `-f` Start in foreground mode.
- `-h` Show help message.
- `-v` Display version information.

To stop the program, use `sudo pkill l2k` . (We have a graceful shutdown, but how "graceful" it actually is, nobody knows.).

### Configuration

The configuration is stored at `/etc/l2k.conf`.

Lines starting with `#` are ignored as comments.

**Line 1**: Contains two integers.
- **Threshold** (0-100): The system load percentage at which the LED stays solid (always on).
- **Flash Interval** (at 1% load): The base flash period in ticks (1 tick = 10ms).

**Remaining lines**: Each line contains 2 to 3 space-separated strings, representing:
- **LED Name**: The system will search for this automatically (e.g., you can use "numlock").
- **Metric**: Select from `CPU`, `RAM`, or `DISK`.
- **Disk Name**: (Optional) e.g., "sda", "sdb". This field is only effective when the Metric is set to `DISK`.

### Compilation
Requires C++17 on Linux.
```bash
g++ --std=c++17 l2k.cpp -o l2k
```
If `i18n.h` is missing, the tool will default to **English**.
