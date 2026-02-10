#ifndef I18N_H
#define I18N_H
#ifndef HAS_REAL_STRUCT
struct LanguagePack {
    const char *placeholder[21];
};
#endif
// 中文 (zh_CN)
const LanguagePack zh_CN = {
    // MYY 是 没意义的占位符
    "系统中有这些 LED：",/*LED list*/
    "加载配置，从：`/etc/l2k.conf`",
    "MYY",
    "MYY",
    "尝试在后台运行。",
    "执行 `sudo pkill l2k` 停止",
    "使用 -l 查看系统中有的 LED",
    "执行 `sudo pkill l2k` 停止",
    "使用 -f 在前台运行",
    "使用 -v 查看版本",
    "查看 l2k.conf 了解如何配置",
    "触发灯常亮的占用率阈值（0-100）",
    "1% 负载时的闪烁基准周期，单位为tick，每 tick 10ms",
    "灯的名称  指标（CPU/RAM/DISK） 磁盘名称（仅指标为DISK时使用）  示例：",
    "MYY",
    "访问被拒绝：",/*LED path*/ "请用 sudo 权限运行或检查硬件",
    "切换到后台失败",
    "创建 `/etc/l2k.conf` 失败",
    /*LED name*/"？系统里没有这个灯，请检查配置文件。",
    /*metric name*/"？我们还不能监控这个，请检查配置文件。"
};

// 英文 (en_US) - Optimized
const LanguagePack en_US = {
    // MYY is a meaningless placeholder.
    "Available LEDs:",/*LED list*/
    "Loading configuration from: `/etc/l2k.conf`",
    "MYY",
    "MYY",
    "Running in daemon mode (background).",
    "To stop the service, run: sudo pkill l2k",
    "Use -l to view all system LEDs.",
    "Stop: sudo pkill l2k",
    "Foreground mode: Use -f to keep the process in your terminal.",
    "Version: Use -v to display current version.",
    "Configuration: Edit /etc/l2k.conf to customize LED mappings.",
    "Threshold (0-100) for constant LED light.",
    "Base flash interval at 1% load (in 10ms ticks).",
    "LED_NAME  METRIC(CPU/RAM/DISK)  DISK_NAME(if applicable). Example:",
    "MYY",
    "Permission denied while accessing ",/*LED path*/ "Try running with 'sudo' or check if the device exists.",
    "Failed to daemonize process.",
    "Could not create `/etc/l2k.conf` . Please check directory permissions.",
    /*LED name*/"were not found on this system. Check /etc/l2k.conf",
    /*metric name*/"were not found on this system. Check /etc/l2k.conf"
};

#endif
