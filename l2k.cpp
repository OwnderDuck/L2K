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

using namespace std;
namespace fs = filesystem;

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

fs::path getExeDir() {
    try {
        return fs::canonical("/proc/self/exe").parent_path();
    } catch (...) {
        return fs::current_path();
    }
}

void readConfig(vector<ledInfo> &v, int &th, int &fr) {
    fs::path config_path = getExeDir() / "l2k.conf";
    if (!fs::exists(config_path)) {
        ofstream outfile(config_path);
        if (outfile) {
            outfile << "#==================L2K===================#" << endl;
            outfile << "#         Load 2 KeyboardLED v2.0        #" << endl;
            outfile << "#   Map system metrics to keyboard LEDs  #" << endl;
            outfile << "#============================= OwnderDuck#" << endl;
            outfile << endl;
            outfile << "# [GLOBAL SETTINGS]" << endl;
            outfile << "# threshold (th): Usage percentage (0-100) to trigger constant light."
                    << endl;
            outfile << "# frequency (fr): Base interval (in 10ms ticks) when usage is 0%." << endl;
            outfile << "# -----------------------------------------------------------" << endl;
            outfile << "# th   fr" << endl;
            outfile << "  91   184" << endl;
            outfile << endl;
            outfile << "# [LED MAPPINGS]" << endl;
            outfile << "# Format: <LED_Type> <Metric_Type> [Device_Name]" << endl;
            outfile << "# " << endl;
            outfile << "# LED_Type:    numlock, capslock, scrolllock" << endl;
            outfile << "# Metric_Type: CPU, RAM, DISK" << endl;
            outfile << "# Device_Name: Required for DISK (e.g., sda, nvme0n1)" << endl;
            outfile << "# -----------------------------------------------------------" << endl;
            outfile << "numlock    CPU" << endl;
            outfile << "capslock   RAM" << endl;
            outfile << "#scrolllock DISK sda" << endl;
            outfile << endl;
            outfile << "================ ABOUT =================" << endl;
            outfile << "           My Blog: froog.icu           " << endl;
            outfile << "repo: https://github.com/OwnderDuck/L2K/" << endl;

            outfile.close();
            if (chmod(config_path.c_str(), 0666) != 0) {
                cerr << "[L2K] FATAL: Failed to set global write permissions (chmod 0666)." << endl;
                exit(1);
            } else {
                cout << "[L2K] INFO: Default config created with global write permissions." << endl;
            }
        } else {
            cerr << "[L2K] FATAL: Could not create config file at " << config_path
                 << ". Check directory permissions." << endl;
            exit(2);
        }
    }
    cout << "[L2K] INFO: Loading configuration from: " << config_path << endl;

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

    int opt;
    bool runAsDaemon = 1;
    // "dc:h" 意思是有 d, c, h 三个参数，其中 c 后面必须带值 (冒号表示)
    while ((opt = getopt(argc, argv, "fhv")) != -1) {
        switch (opt) {
        case 'f':
            runAsDaemon = 0;
            break;
        case 'h': {
            printf("================= L2K =================\n");
            printf("         Load 2 KeyboardLED v2.0        \n");
            printf("   Map system metrics to keyboard LEDs  \n");
            printf("============================= OwnderDuck\n");
            cout << "[L2K] HELP: Use -f to run in front." << endl;
            cout << "[L2K] HELP: Use -v to see version." << endl;
            cout << "[L2K] HELP: Press `sudo pkill l2k` to stop." << endl;
            cout << "[L2K] HELP: see l2k.conf for info of config" << endl;
            printf("================ ABOUT =================\n");
            printf("           My Blog: froog.icu           \n");
            printf("repo: https://github.com/OwnderDuck/L2K/\n");

            return 0;
        }
        case 'v': {
            cout << "[L2K] VERSION: 2.0" << endl;
            return 0;
        }
        }
    }
    printf("================= L2K ==================\n");
    printf("         Load 2 KeyboardLED v2.0        \n");
    printf("   Map system metrics to keyboard LEDs  \n");
    printf("============================= OwnderDuck\n");
    cout << "[L2K] INFO: Try to run as daemon." << endl;
    cout << "[L2K] INFO: Press `sudo pkill l2k` to stop." << endl;

    if (runAsDaemon) {
        if (daemon(1, 0) != 0) {
            cerr << "[L2K] FATAL: Failed to daemonize." << endl;
            exit(3);
        }
    }
    vector<ledInfo> led;
    listLed();
    int threshold, frequency;
    readConfig(led, threshold, frequency);
    for (ledInfo &ledNow : led) {
        ledNow.fd = open(ledNow.path.c_str(), O_WRONLY);
        if (ledNow.fd < 0) {
            cerr << "[L2K] FATAL: Access denied to " << ledNow.path << endl;
            cerr << "      Please run with 'sudo' or check hardware availability." << endl;
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