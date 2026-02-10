**Warning: Lacks thorough testing!!! | 警告：未经大量测试！！！**

**You’re still helpful, even if it's just for translations. | 帮助我：即使只是翻译！**

**Pending: Native English speakers (or fluent speakers) needed to rewrite i18n and README_EN.md.**

**Pending: Translation: Translated by Gemini-3-Flash; verification required.**

**翻译：需要会英文的人**

# L2K (Load 2 KeyboardLED) v3.2.2

[**English**](docs/README_EN.md)

Ultra-lightweight Linux system monitor. Gone are the complex GUIs—we use LEDs for display.

超轻量级 Linux 系统监控工具，我们除去了繁杂的 GUI，使用 LED 灯来显示。

No LEDs? You’ve got three on your keyboard. No keyboard backlights? Try your NIC or disk indicators—any kernel-managed LED will do.

什么？你没有 LED 灯？你键盘上不是有三个吗？没有键盘灯？看看网卡灯、硬盘灯……任何内核管理的灯。

If you lack even those, you're on your own.

要是都没有，我帮不了你。。。
## Features
- **Real-time Monitoring | 实时监控**：Track CPU, RAM, and Disk utilization (multi-disk support included). | CPU、RAM、磁盘繁忙比率（多个盘也行）。
- **Flashing | 闪烁**: LED indicators flash based on system load. | 根据系统负载闪烁 LED。
- **Customizable | 可定制**: Customize LED mapping, thresholds, and flash frequency via `/etc/l2k.conf` . | 通过 `/etc/l2k.conf` 自定义 LED映射关系、阈值与闪烁频率。
- **Silent Mode | 静默模式**：Runs quietly in the background by default. | 默认在后台静默运行。
- **i18n Support | 多语言支持**: Supports both Chinese and English. | 支持中英文。
## Usage
### Running
**Root privileges** are required to write to the LED devices under `/sys/class/leds/` .

需要 **root** 权限，因为要写入 `/sys/class/leds/` 下的灯。

By default, the tool runs in **background mode**.

默认为后台模式运行。

Supported Command Line Arguments | 支持命令行参数:
- `-f` Supported Command Line Arguments. | 以前台模式启动。
- `-h` Show help message. | 显示帮助。
- `-v` Display version information. | 版本号？

To stop the program, use `sudo pkill l2k` . (We have a graceful shutdown, but how "graceful" it actually is, nobody knows.).

使用 `sudo pkill l2k` 来关闭。（应该有安全关闭的）。

### Configuration
The configuration is stored at `/etc/l2k.conf`.

存在 `/etc/l2k.conf`。

Lines starting with `#` are ignored as comments.

忽略所有 `#` 开头的行：

**Line 1**: Contains two integers.
- **Threshold** (0-100): The system load percentage at which the LED stays solid (always on).
- **Flash Interval** (at 1% load): The base flash period in ticks (1 tick = 10ms).

第 1 行， 2 个整数。分别代表灯常亮该指标的阀值（0-100）与负载为 1% 时的闪烁周期（以 tick 为单位，1 个 10ms ）。

**Remaining lines**: Each line contains 2 to 3 space-separated strings, representing:
- **LED Name**: The system will search for this automatically (e.g., you can use "numlock").
- **Metric**: Select from `CPU`, `RAM`, or `DISK`.
- **Disk Name**: (Optional) e.g., "sda", "sdb". This field is only effective when the Metric is set to `DISK`.

接下来一直到文件末尾，每行 2 到 3 个以空格分割的字符串，分别代表：
- 灯名字（会自动搜索，你可以写 “numlock” 等）。
- 指标名，可选项：“CPU”/“RAM“/”DISK“。
- 磁盘名，如 “sda”、“sdb”等。（仅在指标名为 DISK 时生效）

### Compilation
Requires C++17 on Linux.

Linux 下，需要 C++17。
```bash
g++ --std=c++17 l2k.cpp -o l2k
```
If `i18n.h` is missing, the tool will default to **English**.

如果找不到 `i18n.h`，就默认使用英文。
