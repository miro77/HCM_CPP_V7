#include "geo.hpp"

#include <cmath>

#include "fmath.hpp"
#include "fortran_io.hpp"

namespace hcmms {

namespace {

constexpr double kEarthRadius = 6.37129e3;

// Grad, Minuten, Sekunden mit Uebertrag aus NINT der Sekunden.
std::string dms(double v, int deg_width) {
    const int i0 = static_cast<int>(v);
    double rest = (v - i0) * 6.0e1;
    int i = i0;
    int j = static_cast<int>(rest);
    rest = (rest - j) * 6.0e1;
    int k = nint(rest);
    if (k >= 60) {
        k -= 60;
        ++j;
        if (j >= 60) {
            j -= 60;
            ++i;
        }
    }
    return fio::write_i(i, deg_width, deg_width) + "|" + fio::write_i(j, 2, 2) + fio::write_i(k, 2, 2);
}

}  // namespace

double calc_distance(const GeoPoint &a, const GeoPoint &b) {
    const double s1 = dsind((b.lat - a.lat) / 2.0);
    const double s2 = dsind((b.lon - a.lon) / 2.0);
    const double t = s1 * s1 + dcosd(a.lat) * dcosd(b.lat) * (s2 * s2);
    return 2.0 * kEarthRadius * std::atan2(std::sqrt(t), std::sqrt(1.0 - t));
}

double calc_direction(const GeoPoint &a, const GeoPoint &b) {
    double azi = datan2d(dsind(b.lon - a.lon) * dcosd(b.lat),
                         dcosd(a.lat) * dsind(b.lat) - dsind(a.lat) * dcosd(b.lat) * dcosd(b.lon - a.lon));
    if (azi < 0.0) {
        azi += 360.0;
    }
    return azi;
}

std::string coo_conv(const GeoPoint &p) {
    double lo = p.lon;
    double la = p.lat;
    char ew = 'E';
    char ns = 'N';
    if (lo < 0.0) {
        ew = 'W';
        lo = -lo;
    }
    if (la < 0.0) {
        ns = 'S';
        la = -la;
    }
    const std::string a = dms(lo, 3);
    const std::string b = dms(la, 2);
    // a = "DDD|MMSS", b = "DD|MMSS"
    return a.substr(0, 3) + ew + a.substr(4) + b.substr(0, 2) + ns + b.substr(3);
}

GeoPoint new_coordinates(const GeoPoint &p, double dir, float d) {
    if (d == 0.0F) {
        return p;
    }
    GeoPoint n = p;
    const double dp = static_cast<double>(d) / 1.112e2;
    const double t1 = dcosd(p.lat) * dsind(dp);
    const double t2 = (dsind(p.lat) * dcosd(dp)) / t1;
    double t = t1 * (dcosd(dir) + t2);
    t = std::fmin(std::fmax(t, -1.0), 1.0);
    n.lat = dasind(t);
    if (dir == 0.0 || dir == 3.6e2 || dir == 1.8e2) {
        n.lon = p.lon;
        return n;
    }
    t = (dcosd(dp) - dsind(p.lat) * dsind(n.lat)) / (dcosd(p.lat) * dcosd(n.lat));
    t = std::fmin(std::fmax(t, -1.0), 1.0);
    const double dl = dacosd(t);
    if (dir < 1.8e2) {
        n.lon = p.lon + dl;
    } else {
        n.lon = p.lon - dl;
    }
    return n;
}

int parse_coordinates(std::string_view coo, GeoPoint &out) {
    double ci = 0.0;
    if (!fio::read_f(coo.substr(0, 3), 0, out.lon)) {
        return 1;
    }
    if (!fio::read_f(coo.substr(4, 2), 0, ci)) {
        return 2;
    }
    out.lon = out.lon + ci / 6.0e1;
    if (!fio::read_f(coo.substr(6, 2), 0, ci)) {
        return 3;
    }
    out.lon = out.lon + ci / 3.6e3;
    if (coo[3] == 'W') {
        out.lon = 0.0 - out.lon;
    }
    if (coo[3] != 'E' && coo[3] != 'W') {
        return 4;
    }
    if (!fio::read_f(coo.substr(8, 2), 0, out.lat)) {
        return 5;
    }
    if (!fio::read_f(coo.substr(11, 2), 0, ci)) {
        return 6;
    }
    out.lat = out.lat + ci / 6.0e1;
    if (!fio::read_f(coo.substr(13, 2), 0, ci)) {
        return 7;
    }
    out.lat = out.lat + ci / 3.6e3;
    if (coo[10] == 'S') {
        out.lat = 0.0 - out.lat;
    }
    if (coo[10] != 'N' && coo[10] != 'S') {
        return 8;
    }
    return 0;
}

}  // namespace hcmms
