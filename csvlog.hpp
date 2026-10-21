// csvlog.hpp -- append-only record of (parameters -> results).
//
// Opened lazily on the first write so that merely launching the program does
// not litter the working directory, and closed per row so a log survives a
// crash or a kill.
#pragma once

#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>

#include "physics.hpp"

namespace hc {

class CsvLog {
public:
    explicit CsvLog(std::string path) : path_(std::move(path)) {}

    const std::string& path() const { return path_; }
    long rows() const { return rows_; }
    const std::string& lastError() const { return lastError_; }

    bool append(const Solution& s) {
        const bool needHeader = !headerWritten_ && !fileExists();
        std::ofstream f(path_, std::ios::app);
        if (!f) {
            lastError_ = "could not open " + path_ + " for writing";
            return false;
        }
        if (needHeader) {
            f << "timestamp_utc,L_m,N_turns,i_A,"
                 "B0_uT,B0_G,B0_dir,"
                 "separation_m,coil_plane_z_m,"
                 "stable_radius_m,stable_diameter_m\n";
        }
        headerWritten_ = true;

        char stamp[32];
        const std::time_t t = std::time(nullptr);
        std::strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));

        f << stamp << ','
          << fmt(s.L, 6) << ',' << s.N << ',' << fmt(s.i, 4) << ','
          << fmt(s.B0_uT, 6) << ',' << fmt(s.B0_G, 8) << ','
          << (s.B0_dir.z >= 0 ? "+z" : "-z") << ','
          << fmt(s.separation, 6) << ',' << fmt(s.coilZ, 6) << ','
          << fmt(s.radius, 6) << ',' << fmt(s.diameter, 6) << '\n';

        if (!f) {
            lastError_ = "write to " + path_ + " failed";
            return false;
        }
        ++rows_;
        lastError_.clear();
        return true;
    }

private:
    bool fileExists() const {
        std::ifstream f(path_);
        return f.good();
    }

    static std::string fmt(double v, int digits) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", digits, v);
        return buf;
    }

    std::string path_;
    bool headerWritten_ = false;
    long rows_ = 0;
    std::string lastError_;
};

}  // namespace hc
