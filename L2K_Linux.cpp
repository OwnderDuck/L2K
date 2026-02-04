#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

using namespace std;
namespace fs = filesystem;

string GetLedPath(string type) {
    string base_path = "/sys/class/leds/";
    for (const auto &entry : fs::directory_iterator(base_path)) {
        string name = entry.path().filename().string();
        if (name.find(type) != string::npos) {
            return entry.path().string() + "/brightness";
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
        if (i == 3 || i == 4)
            idle += v;
        i++;
    }
    unsigned long long total_diff = total - lastTotal;
    unsigned long long idle_diff = idle - lastIdle;
    lastTotal = total;
    lastIdle = idle;
    return (total_diff == 0)
                         ? 0
                         : (1.0 - static_cast<double>(idle_diff) / total_diff) * 100;
}
int getRam() {
    ifstream file("/proc/meminfo");
    string line;
    size_t total = 0, available = 0;
    while (getline(file, line)) {
        if (line.find("MemTotal:") == 0)
            stringstream(line.substr(9)) >> total;
        if (line.find("MemAvailable:") == 0)
            stringstream(line.substr(13)) >> available;
        if (total && available)
            break;
    }
    return (total > 0) ? (static_cast<double>(total - available) / total) * 100
                                         : 0;
}
double getDiskBusy(const string &dev) {
    static size_t last_io_ms = 0;
    static auto last_time = chrono::steady_clock::now();
    auto get_io = [&](const string &d) -> size_t {
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
                return ms;
            }
        }
        return 0;
    };
    size_t cur_io = get_io(dev);
    auto cur_time = chrono::steady_clock::now();
    auto dur =
            chrono::duration_cast<chrono::milliseconds>(cur_time - last_time).count();
    double ratio = (dur > 0) ? (double)(cur_io - last_io_ms) / dur : 0.0;
    last_io_ms = cur_io;
    last_time = cur_time;
    return ratio > 1.0 ? 1.0 : ratio;
}
int main() {
    string pathNumlock = GetLedPath("numlock"),
                 pathCapslock = GetLedPath("capslock"),
                 pathScrolllock = GetLedPath("scrolllock");
    int fdCpu = open(pathNumlock.c_str(), O_WRONLY);
    int fdRam = open(pathCapslock.c_str(), O_WRONLY);
    int fdDisk = open(pathScrolllock.c_str(), O_WRONLY);

    if (fdCpu < 0 || fdRam < 0 || fdDisk < 0) {
        cerr << "Maybe I need sudo. Or there is no numlock/capslock/scrolllock led"
                 << endl;
        return 1;
    }

    auto next_tick = chrono::steady_clock::now();
    int cpuUsage = 0, ramUsage = 0, diskUsage = 0;
    bool s1 = 0, s2 = 0, s3 = 0;
    long duration = 0;
    int tick = 0;

    cout << "Start..." << endl;

    while (true) {
        auto tick_start = chrono::steady_clock::now();
        tick++;
        next_tick += chrono::milliseconds(10);

        if (tick % 32 == 0) {
            cpuUsage = getCpu();
            ramUsage = getRam();
            diskUsage = getDiskBusy("sda") * 100;
            printf("\rCPU:%3d%% RAM:%3d%% DSK:%3d%% MSPT:%6.2fus    ", cpuUsage,
                         ramUsage, diskUsage, (double)duration / 1000.0);
            fflush(stdout);
        }

        auto processLed = [&](int usage, bool &state, int fd) {
            if (usage > 91) {
                if (!state) {
                    state = true;
                    pwrite(fd, "1", 1, 0);
                }
            } else {
                int interval = 184 - (usage * 2);
                if (interval < 2)
                    interval = 2;
                if (tick % interval == 0) {
                    state = true;
                    pwrite(fd, "1", 1, 0);
                } else if (state) {
                    state = false;
                    pwrite(fd, "0", 1, 0);
                }
            }
        };

        processLed(cpuUsage, s1, fdCpu);
        processLed(ramUsage, s2, fdRam);
        processLed(diskUsage, s3, fdDisk);

        auto tick_end = chrono::steady_clock::now();
        duration = chrono::duration_cast<chrono::nanoseconds>(tick_end - tick_start)
                                     .count();

        this_thread::sleep_until(next_tick);
    }
    close(fdCpu);close(fdRam);close(fdDisk);
    return 0;
}
