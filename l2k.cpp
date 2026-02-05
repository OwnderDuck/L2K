#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <pwd.h>
#define L2KVER "3.0"
using namespace std;
namespace fs = filesystem;

// --- I18N Logic Start ---
#if __has_include("i18n.h")
#include "i18n.h"
#define HAS_I18N
#else
struct LanguagePack {
    const char* info_load_conf       = "Loading configuration from: ";
    const char* info_create_conf     = "Default configuration created with global write permissions.";
    const char* info_ask_auth       = "Authentication required: Type 'frog' and press Enter to grant write access to l2k.conf, or any other key to skip.";
    const char* info_daemon          = "Running in daemon mode (background).";
    const char* info_stop            = "To stop the service, run: sudo pkill l2k";
    const char* help_stop            = "Stop: sudo pkill l2k";
    const char* help_front           = "Foreground mode: Use -f to keep the process in your terminal.";
    const char* help_version         = "Version: Use -v to display current version.";
    const char* help_config          = "Configuration: Edit /etc/l2k.conf to customize LED mappings.";
    const char* conf_th_desc         = "Threshold (0-100) for constant LED light.";
    const char* conf_fr_desc         = "Base flash interval at 0% load (in 10ms ticks).";
    const char* conf_map_led         = "LED_NAME  METRIC(CPU/RAM/DISK)  DISK_NAME(if applicable). Example:";
    const char* fatal_chmod          = "Failed to set global write permissions (chmod 0666).";
    const char* fatal_open_led1      = "Permission denied while accessing ";
    const char* fatal_open_led2      = "Try running with 'sudo' or check if the device exists.";
    const char* fatal_daemon         = "Failed to daemonize process.";
    const char* fatal_failed_to_create1 = "Could not create configuration file at ";
    const char* fatal_failed_to_create2 = ". Please check directory permissions.";
} en_fallback;
#endif

#ifdef HAS_I18N
const LanguagePack *lang = &en_US;
#else
const LanguagePack *lang = &en_fallback;
#endif

void init_i18n() {
#ifdef HAS_I18N
    char *env = getenv("LANG");
    if (env && string(env).find("zh_CN") != string::npos) {
        lang = &zh_CN;
    }
#endif
}
// --- I18N Logic End ---

struct ledInfo {
    string name;
    string path;
    string lookLoad;
    string diskName;
    int fd;
    bool state = 0;
    int *usageSource;
};

unordered_map<string, int> diskUsageList;

string GetLedName(string type);
void readConfig(vector<ledInfo> &v, int &th, int &fr) {
    fs::path config_path = "/etc/l2k.conf";
    if (!fs::exists(config_path)) {
        ofstream outfile(config_path);
        if (outfile) {
            outfile << "#================= L2K ==================#" << endl;
            outfile << "#         Load 2 KeyboardLED v" L2KVER<<"        #" << endl;
            outfile << "#   Map system metrics to keyboard LEDs  #" << endl;
            outfile << "#============================= OwnderDuck#" << endl;
            outfile << endl;
            outfile << "# [GLOBAL SETTINGS]" << endl;
            outfile << "# " << lang->conf_th_desc << endl;
            outfile << "# " << lang->conf_fr_desc << endl;
            outfile << "# -----------------------------------------------------------" << endl;
            outfile << "# th   fr" << endl;
            outfile << "  91   184" << endl;
            outfile << endl;
            outfile << "# [LED MAPPINGS]" << endl;
            outfile << "# " << lang->conf_map_led << endl;
            outfile << "numlock    CPU" << endl;
            outfile << "capslock   RAM" << endl;
            outfile << "#scrolllock DISK sda" << endl;
            outfile << endl;
            outfile << "#================ ABOUT =================#" << endl;
            outfile << "#           My Blog: froog.icu           #" << endl;
            outfile << "#    repo: github.com/OwnderDuck/L2K/    #" << endl;
            outfile.close();
        } else {
            cerr << "[L2K] FATAL: " << lang->fatal_failed_to_create1 << config_path
                 << lang->fatal_failed_to_create2 << endl;
            exit(2);
        }
    }
    cout << "[L2K] INFO: " << lang->info_load_conf << config_path << endl;

    ifstream inFile(config_path);
    string line;
    string base_path = "/sys/class/leds/";
    while (getline(inFile, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        if (ss >> th >> fr) {
            break;
        }
    }
    while (getline(inFile, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        string name, type;
        if (ss >> name >> type) {
            string fullName = GetLedName(name);
            string path     = base_path + fullName + "/brightness";
            v.push_back({fullName, path, type, "frog"});
            if (type == "DISK") {
                string diskName;
                ss >> diskName;
                v.back().diskName = diskName;
            }
            cout << "[L2K] BIND: " << name << " -> " << type << " (Device: " << path << ")" << endl;
        }
    }
}
void listLed() {
    string base_path = "/sys/class/leds/";
    cout << "Found LEDs:";
    for (const auto &entry : fs::directory_iterator(base_path)) {
        string name = entry.path().filename().string();
        cout << " " << name;
    }
    cout << endl;
}

string GetLedName(string type) {
    string base_path = "/sys/class/leds/";
    for (const auto &entry : fs::directory_iterator(base_path)) {
        string name = entry.path().filename().string();
        if (name.size() >= type.size() &&
            name.substr(name.size() - type.size(), type.size()) == type) {
            return name;
        }
    }
    return "";
}

int getCpu() {
    static size_t lastIdle = 0, lastTotal = 0;
    ifstream file("/proc/stat");
    string line;
    getline(file, line);
    stringstream ss(line);
    string label;
    ss >> label;
    size_t v, total = 0, idle = 0, i = 0;
    while (ss >> v) {
        total += v;
        if (i == 3 || i == 4) idle += v;
        i++;
    }
    unsigned long long total_diff = total - lastTotal;
    unsigned long long idle_diff  = idle - lastIdle;
    lastTotal                     = total;
    lastIdle                      = idle;
    return (total_diff == 0) ? 0 : (1.0 - static_cast<double>(idle_diff) / total_diff) * 100;
}
int getRam() {
    ifstream file("/proc/meminfo");
    string line;
    size_t total = 0, available = 0;
    while (getline(file, line)) {
        if (line.find("MemTotal:") == 0) stringstream(line.substr(9)) >> total;
        if (line.find("MemAvailable:") == 0) stringstream(line.substr(13)) >> available;
        if (total && available) break;
    }
    return (total > 0) ? (static_cast<double>(total - available) / total) * 100 : 0;
}
int getDiskBusy(const string &dev) {
    static size_t last_io_ms = 0;
    static auto last_time    = chrono::steady_clock::now();
    auto get_io              = [&](const string &d) -> size_t {
        ifstream file("/proc/diskstats");
        string line;
        while (getline(file, line)) {
            if (line.find(" " + d + " ") != string::npos) {
                stringstream ss(line);
                string tmp;
                for (int i = 0; i < 12; ++i)
                    ss >> tmp;
                size_t ms;
                ss >> ms;
                return ms * 100;
            }
        }
        return 0;
    };
    size_t cur_io = get_io(dev);
    auto cur_time = chrono::steady_clock::now();
    long long dur = chrono::duration_cast<chrono::milliseconds>(cur_time - last_time).count();
    double ratio  = (dur > 0) ? (double)(cur_io - last_io_ms) / dur : 0.0;
    last_io_ms    = cur_io;
    last_time     = cur_time;
    int usage     = (int)(ratio * 100);
    return usage > 100 ? 100 : usage;
}

volatile sig_atomic_t alive = 1;
void signalHandler(int signum) { alive = false; }
int main(int argc, char *argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    init_i18n();

    int opt;
    bool runAsDaemon = 1;
    // "dc:h" 意思是有 d, c, h 三个参数，其中 c 后面必须带值 (冒号表示)
    while ((opt = getopt(argc, argv, "fhv")) != -1) {
        switch (opt) {
        case 'f':
            runAsDaemon = 0;
            break;
        case 'h': {
            printf("================= L2K ==================\n");
            printf("         Load 2 KeyboardLED v" L2KVER"        \n");
            printf("   Map system metrics to keyboard LEDs  \n");
            printf("============================= OwnderDuck\n");
            cout << "[L2K] HELP: " << lang->help_front << endl;
            cout << "[L2K] HELP: " << lang->help_version << endl;
            cout << "[L2K] HELP: " << lang->help_stop << endl;
            cout << "[L2K] HELP: " << lang->help_config << endl;
            printf("================ ABOUT =================\n");
            printf("           My Blog: froog.icu           \n");
            printf("    repo: github.com/OwnderDuck/L2K/    \n");

            return 0;
        }
        case 'v': {
            cout << "[L2K] VERSION: " L2KVER << endl;
            return 0;
        }
        }
    }
    printf("================= L2K ==================\n");
    printf("         Load 2 KeyboardLED v" L2KVER"        \n");
    printf("   Map system metrics to keyboard LEDs  \n");
    printf("============================= OwnderDuck\n");

    vector<ledInfo> led;
    listLed();
    int threshold, frequency;
    readConfig(led, threshold, frequency);
    for (ledInfo &ledNow : led) {
        ledNow.fd = open(ledNow.path.c_str(), O_WRONLY);
        if (ledNow.fd < 0) {
            cerr << "[L2K] FATAL: " << lang->fatal_open_led1 << ledNow.path << endl;
            cerr << "      " << lang->fatal_open_led2 << endl;
            exit(1);
        }
    }
    int cpuUsage = 0, ramUsage = 0, diskUsage = 0;
    for (ledInfo &ledNow : led) {
        if (ledNow.lookLoad == "CPU") ledNow.usageSource = &cpuUsage;
        if (ledNow.lookLoad == "RAM") ledNow.usageSource = &ramUsage;
        if (ledNow.lookLoad == "DISK") ledNow.usageSource = &diskUsageList[ledNow.diskName];
    }
    int *displayDiskUsage  = nullptr;
    string displayDiskName = "None";

    for (ledInfo &ledNow : led) {
        if (ledNow.lookLoad == "DISK") {
            diskUsageList[ledNow.diskName] = 0;
            ledNow.usageSource             = &diskUsageList[ledNow.diskName];
            if (displayDiskUsage == nullptr) {
                displayDiskUsage = ledNow.usageSource;
                displayDiskName  = ledNow.diskName;
            }
        }
    }
    auto next_tick = chrono::steady_clock::now();
    long duration  = 0;
    int tick       = 0;
    if (runAsDaemon) {
        cout << "[L2K] INFO: " << lang->info_daemon << endl;
        cout << "[L2K] INFO: " << lang->info_stop << endl;
        if (daemon(1, 0) != 0) {
            cerr << "[L2K] FATAL: " << lang->fatal_daemon << endl;
            exit(3);
        }
    }
    while (alive) {
        auto tick_start = chrono::steady_clock::now();
        tick++;
        next_tick += chrono::milliseconds(10);

        if (tick % 32 == 0) {
            cpuUsage = getCpu();
            ramUsage = getRam();

            for (ledInfo &ledNow : led) {
                if (ledNow.lookLoad == "DISK") {
                    diskUsageList[ledNow.diskName] = getDiskBusy(ledNow.diskName);
                }
            }
            /*
            printf("\rCPU:%3d%% RAM:%3d%% sda:%3d%% MSPT:%6.2fus    ",
                cpuUsage, ramUsage, diskUsageList["sda"], (double)duration / 1000.0);
                */
            if (!runAsDaemon) {
                printf("\rCPU:%3d%% RAM:%3d%% %s:%3d%% MSPT:%6.2fus    ", cpuUsage, ramUsage,
                       displayDiskName.c_str(), displayDiskUsage ? *displayDiskUsage : 0,
                       (double)duration / 1000.0);
                fflush(stdout);
            }
        }
        auto processLed = [&](int usage, bool &state, int fd) {
            if (usage > threshold) {
                if (!state) {
                    state = true;
                    pwrite(fd, "1", 1, 0);
                }
            } else {
                double interval = frequency - (usage * frequency * 1.0 / threshold);
                if (interval < 2) interval = 2;
                if (tick % (int)interval == 0) {
                    state = true;
                    pwrite(fd, "1", 1, 0);
                } else if (state) {
                    state = false;
                    pwrite(fd, "0", 1, 0);
                }
            }
        };
        for (ledInfo &ledNow : led) {
            processLed(*(ledNow.usageSource), ledNow.state, ledNow.fd);
        }

        auto tick_end = chrono::steady_clock::now();
        duration      = chrono::duration_cast<chrono::nanoseconds>(tick_end - tick_start).count();

        this_thread::sleep_until(next_tick);
    }
    for (ledInfo &ledNow : led) {
        pwrite(ledNow.fd, "0", 1, 0);
        close(ledNow.fd);
    }
    return 0;
}