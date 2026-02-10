#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#define L2KVER "3.3.1"

#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

using namespace std;
namespace fs = filesystem;

// --- I18N Logic Start ---
/* clang-format off */
struct LanguagePack {
    const char *info_ls; const char *info_load_conf; const char *info_create_conf; const char *info_ask_auth; const char *info_daemon; const char *info_stop;
    const char *help_ls; const char *help_stop; const char *help_front; const char *help_version; const char *help_config;
    const char *conf_th_desc; const char *conf_fr_desc; const char *conf_map_led;
    const char *fatal_chmod; const char *fatal_open_led1; const char *fatal_open_led2; const char *fatal_daemon;
    const char *fatal_failed_to_create; const char *fatal_no_led; const char *fatal_no_look;
};

#define HAS_REAL_STRUCT

#pragma message("Compiling L2K Version: " L2KVER)
#if __has_include("i18n.h")
    #include "i18n.h"
    #define HAS_I18N
#endif
const LanguagePack en_fallback = {
    "Loading configuration from: ",
    "MYY",
    "MYY",
    "Running in daemon mode (background).",
    "To stop the service, run: sudo pkill l2k",
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
    "were not found on this system. Check /etc/l2k.conf",
    "were not found on this system. Check /etc/l2k.conf"
};
#ifdef HAS_I18N
    const LanguagePack *lang = &en_US;
#else
    const LanguagePack *lang = &en_fallback;
#endif
/* clang-format on */
void init_i18n() {
#ifdef HAS_I18N
    char* env = getenv("LANG");
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
    int* usageSource;
};

unordered_map<string, int> diskUsageList;

string GetLedName(string type);
void readConfig(vector<ledInfo>& v, int& th, int& fr) {
    fs::path config_path = "/etc/l2k.conf";
    if (!fs::exists(config_path)) {
        ofstream outfile(config_path);
        if (outfile) {
            outfile << "#================= L2K ==================#" << endl;
            outfile << "#         Load 2 KeyboardLED v" L2KVER << "      #" << endl;
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
            cerr << "[L2K] FATAL: " << lang->fatal_failed_to_create << endl;
            exit(2);
        }
    }
    cout << "[L2K] INFO: " << lang->info_load_conf << endl;

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
            string path = base_path + fullName + "/brightness";
            v.push_back({fullName, path, type, "frog"});

            if (type == "DISK") {
                string diskName;
                ss >> diskName;
                v.back().diskName = diskName;
            }
            if (type == "CPU" || type == "RAM" || type == "DISK") {
                cout << "[L2K] BIND: " << name << " -> " << type;
                if (type == "DISK") cout << " " << v.back().diskName;
                cout << " (Device: " << path << ")" << endl;
            } else {
                cout << "[L2K] FATAL: " << type << " " << lang->fatal_no_look << endl;
                exit(6);
            }
        }
    }
}
void listLed() {
    string base_path = "/sys/class/leds/";
    for (const auto& entry : fs::directory_iterator(base_path)) {
        string name = entry.path().filename().string();
        cout << " " << name;
    }
    cout << endl;
}

string GetLedName(string type) {
    string base_path = "/sys/class/leds/";
    for (const auto& entry : fs::directory_iterator(base_path)) {
        string name = entry.path().filename().string();
        if (name.size() >= type.size() &&
            name.substr(name.size() - type.size(), type.size()) == type) {
            return name;
        }
    }
    cout << "[L2K] FATAL: " << type << " " << lang->fatal_no_led << endl;
    exit(5);
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
    unsigned long long idle_diff = idle - lastIdle;
    lastTotal = total;
    lastIdle = idle;
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

int getDiskBusy(const string& dev) {
    struct Stat {
        size_t io_ms;
        chrono::steady_clock::time_point ts;
    };
    static map<string, Stat> history;

    auto get_io = [&](const string& d) -> size_t {
        ifstream file("/proc/diskstats");
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string maj, min, name;
            ss >> maj >> min >> name;
            if (name == d) {
                size_t val = 0;
                for (int i = 0; i < 10; ++i) ss >> val;
                return val;
            }
        }
        return 0;
    };

    size_t cur_io = get_io(dev);
    auto cur_time = chrono::steady_clock::now();

    if (history.find(dev) == history.end()) {
        history[dev] = {cur_io, cur_time};
        return 0;
    }

    Stat& prev = history[dev];
    auto duration = chrono::duration_cast<chrono::milliseconds>(cur_time - prev.ts).count();

    int usage = 0;
    if (duration > 0) {
        double delta_io = (double)(cur_io - prev.io_ms);
        usage = (int)((delta_io * 100.0) / duration);
    }

    prev.io_ms = cur_io;
    prev.ts = cur_time;

    return (usage > 100) ? 100 : usage;
}

volatile sig_atomic_t alive = 1;
void signalHandler(int signum) { alive = false; }
int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    init_i18n();

    int opt;
    bool runAsDaemon = 1;
    while ((opt = getopt(argc, argv, "fhvl")) != -1) {
        switch (opt) {
            case 'f':
                runAsDaemon = 0;
                break;
            case 'h': {

                printf(" ██╗     ██████╗ ██╗  ██╗\n");
                printf(" ██║     ╚════██╗██║ ██╔╝   Load 2 KeyboardLED v" L2KVER "\n");
                printf(" ██║      █████╔╝█████╔╝    Map system metrics to keyboard LEDs \n");
                printf(" ██║     ██╔═══╝ ██╔═██╗    Github: OwnderDuck/L2K/\n");
                printf(" ███████╗███████╗██║  ██╗\n");
                printf(" ╚══════╝╚══════╝╚═╝  ╚═╝\n");
                cout << "[L2K] HELP: " << lang->help_front << endl;
                cout << "[L2K] HELP: " << lang->help_version << endl;
                cout << "[L2K] HELP: " << lang->help_stop << endl;
                cout << "[L2K] HELP: " << lang->help_config << endl;
                cout << "[L2K] HELP: " << lang->help_ls << endl;
                printf("================ ABOUT =================\n");
                printf("           My Blog: froog.icu           \n");
                printf("    repo: github.com/OwnderDuck/L2K/    \n");

                return 0;
            }
            case 'v': {
                cout << "[L2K] VERSION: " L2KVER << endl;
                return 0;
            }
            case 'l': {
                cout<<"[L2K] INFO: "<<lang->info_ls;
                listLed();
                return 0;
            }
        }
    }    
    printf(" ██╗     ██████╗ ██╗  ██╗\n");
    printf(" ██║     ╚════██╗██║ ██╔╝   Load 2 KeyboardLED v" L2KVER "\n");
    printf(" ██║      █████╔╝█████╔╝    Map system metrics to keyboard LEDs \n");
    printf(" ██║     ██╔═══╝ ██╔═██╗    Github: OwnderDuck/L2K/\n");
    printf(" ███████╗███████╗██║  ██╗\n");
    printf(" ╚══════╝╚══════╝╚═╝  ╚═╝\n");
    vector<ledInfo> led;
    cout << "[L2K] INFO: " << lang->info_ls << endl;
    listLed();
    int threshold, frequency;
    readConfig(led, threshold, frequency);
    for (ledInfo& ledNow : led) {
        ledNow.fd = open(ledNow.path.c_str(), O_WRONLY);
        if (ledNow.fd < 0) {
            cerr << "[L2K] FATAL: " << lang->fatal_open_led1 << ledNow.path << endl;
            cerr << "      " << lang->fatal_open_led2 << endl;
            exit(1);
        }
    }
    int cpuUsage = 0, ramUsage = 0, diskUsage = 0;
    for (ledInfo& ledNow : led) {
        if (ledNow.lookLoad == "CPU") ledNow.usageSource = &cpuUsage;
        if (ledNow.lookLoad == "RAM") ledNow.usageSource = &ramUsage;
        if (ledNow.lookLoad == "DISK") ledNow.usageSource = &diskUsageList[ledNow.diskName];
    }
    int* displayDiskUsage = nullptr;
    string displayDiskName = "None";

    for (ledInfo& ledNow : led) {
        if (ledNow.lookLoad == "DISK") {
            diskUsageList[ledNow.diskName] = 0;
            ledNow.usageSource = &diskUsageList[ledNow.diskName];
            if (displayDiskUsage == nullptr) {
                displayDiskUsage = ledNow.usageSource;
                displayDiskName = ledNow.diskName;
            }
        }
    }
    auto next_tick = chrono::steady_clock::now();
    long duration = 0;
    int tick = 0;
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

            for (ledInfo& ledNow : led) {
                if (ledNow.lookLoad == "DISK") {
                    diskUsageList[ledNow.diskName] = getDiskBusy(ledNow.diskName);
                }
            }
            if (!runAsDaemon) {
                printf("\rCPU:%3d%% RAM:%3d%% %s:%3d%% MSPT:%6.2fus    ", cpuUsage, ramUsage,
                       displayDiskName.c_str(), displayDiskUsage ? *displayDiskUsage : 0,
                       (double)duration / 1000.0);
                fflush(stdout);
            }
        }
        auto processLed = [threshold, frequency, &tick](int usage, bool& state, int fd) {
            if (LIKELY(usage > 0 && usage <= threshold)) {
                int interval = frequency - (usage * frequency / threshold);
                if (interval < 2) interval = 2;

                bool should_be_on = (tick % interval == 0);
                if (should_be_on != state) {
                    state = should_be_on;
                    (void)pwrite(fd, state ? "1" : "0", 1, 0);
                }
            } else if (usage > threshold) {
                if (!state) {
                    state = true;
                    (void)pwrite(fd, "1", 1, 0);
                }
            } else {
                if (state) {
                    state = false;
                    (void)pwrite(fd, "0", 1, 0);
                }
            }
        };
        for (ledInfo& ledNow : led) {
            processLed(*(ledNow.usageSource), ledNow.state, ledNow.fd);
        }

        auto tick_end = chrono::steady_clock::now();
        duration = chrono::duration_cast<chrono::nanoseconds>(tick_end - tick_start).count();

        this_thread::sleep_until(next_tick);
    }
    for (ledInfo& ledNow : led) {
        pwrite(ledNow.fd, "0", 1, 0);
        close(ledNow.fd);
    }
    cout << endl;
    return 0;
}
