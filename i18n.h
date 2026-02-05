#ifndef I18N_H
#define I18N_H

struct LanguagePack {
    const char* info_load_conf;
    const char* info_create_conf;
    const char* info_ask_auth;
    const char* info_daemon;
    const char* info_stop;
    const char* help_stop;
    const char* help_front;
    const char* help_version;
    const char* help_config;
    const char* conf_th_desc;
    const char* conf_fr_desc;
    const char* conf_map_led;
    const char* fatal_chmod;
    const char* fatal_open_led1;
    const char* fatal_open_led2;
    const char* fatal_daemon;
    const char* fatal_failed_to_create1;
    const char* fatal_failed_to_create2;
}; 

// 中文 (zh_CN)
const LanguagePack zh_CN = {
    "加载配置，从：",
    "默认设置已创建，已设置全局可写",
    "如果你需要让普通用户也能修改配置文件且这是本程序同目录下的 `/etc/l2k.conf`？就输入“frog”并回车来验证，不需要就随便输入什么并回车。",
    "尝试在后台运行。",
    "执行 `sudo pkill l2k` 停止",
    "执行 `sudo pkill l2k` 停止",
    "使用 -f 在前台运行",
    "使用 -v 查看版本",
    "查看 l2k.conf 了解如何配置",
    "触发灯常亮的占用率阈值（0-100）",
    "0% 负载时的闪烁基准周期，单位为tick，每 tick 10ms",
    "灯的名称  指标（CPU/RAM/DISK） 磁盘名称（仅指标为DISK时使用）  示例：",
    "为配置文件创建全局写权限失败（chmod 0666）",
    "访问被拒绝：",
    "请用 sudo 权限运行或检查硬件",
    "切换到后台失败",
    "在 ",
    "创建配置文件失败"
};

// 英文 (en_US) - Optimized
const LanguagePack en_US = {
    "Loading configuration from: ",
    "Default configuration created with global write permissions.",
    "Authentication required: Type 'frog' and press Enter to grant write access to l2k.conf, or any other key to skip.",
    "Running in daemon mode (background).",
    "To stop the service, run: sudo pkill l2k",
    "Stop: sudo pkill l2k",
    "Foreground mode: Use -f to keep the process in your terminal.",
    "Version: Use -v to display current version.",
    "Configuration: Edit /etc/l2k.conf to customize LED mappings.",
    "Threshold (0-100) for constant LED light.",
    "Base flash interval at 0% load (in 10ms ticks).",
    "LED_NAME  METRIC(CPU/RAM/DISK)  DISK_NAME(if applicable). Example:",
    "Failed to set global write permissions (chmod 0666).",
    "Permission denied while accessing ",
    "Try running with 'sudo' or check if the device exists.",
    "Failed to daemonize process.",
    "Could not create configuration file at ",
    ". Please check directory permissions."
};

#endif