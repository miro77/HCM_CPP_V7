// Generator der synthetischen Testdaten nach SPEC-001 (Gelaende, Morphologie)
// und SPEC-002 (Grenzlinien). Zusage: gleiche Ausgabe bei jedem Lauf auf
// derselben Plattform — der Golden-Datensatz haengt an genau diesen Bytes.
//
// Aufruf: hcmms_fixtures <zielverzeichnis>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

constexpr double kPi = 3.14159265358979323846;

// Gelaendehoehe in Metern an (lon, lat) in Grad. Glatt, mit Huegeln und
// einem See (Hoehe 0) suedwestlich von 8.3E/48.3N.
int terrain_height(double lon, double lat) {
    if (lon < 8.3 && lat < 48.3) {
        return 0;
    }
    const double h = 320.0 + 180.0 * std::sin(lon * 7.1) * std::cos(lat * 5.3) +
                     60.0 * std::sin(lon * 31.0 + lat * 23.0) +
                     25.0 * std::cos(lon * 97.0 - lat * 61.0);
    return static_cast<int>(std::lround(h));
}

// Morphologie: Bit 0 = See.
int morpho_value(double lon, double lat) {
    return (lon < 8.3 && lat < 48.3) ? 1 : 2;
}

// Loch ohne Hoehendaten (-9999) fuer Fehler 400: kleines Rechteck.
bool in_hole(double lon, double lat) {
    return lon > 9.70 && lon < 9.72 && lat > 49.70 && lat < 49.72;
}

void put_i16(std::vector<unsigned char> &buf, std::size_t pos, int v) {
    const auto u = static_cast<std::uint16_t>(static_cast<std::int16_t>(v));
    buf[pos] = static_cast<unsigned char>(u & 0xFFU);
    buf[pos + 1] = static_cast<unsigned char>(u >> 8U);
}

int grid_value(double lon, double lat, bool morpho) {
    if (morpho) {
        return morpho_value(lon, lat);
    }
    return in_hole(lon, lat) ? -9999 : terrain_height(lon, lat);
}

void write_tile(const fs::path &root, int lod, int lad, bool morpho) {
    const int resh = (lad < 50) ? 101 : 51;
    char name[16];
    std::snprintf(name, sizeof name, "e%03dn%02d.%s%c", lod, lad, (resh == 101) ? "33" : "63",
                  morpho ? 'm' : 'e');
    char dir[8];
    std::snprintf(dir, sizeof dir, "e%03d", lod);
    const fs::path d = root / (morpho ? "morpho" : "topo") / dir;
    fs::create_directories(d);
    const std::size_t reclen = 202U * static_cast<std::size_t>(resh);
    std::vector<unsigned char> buf(reclen * 144U, 0);
    const int step = 300 / (resh - 1);
    for (int bv = 0; bv < 12; ++bv) {
        for (int bh = 0; bh < 12; ++bh) {
            const std::size_t base = static_cast<std::size_t>(12 * bv + bh) * reclen;
            for (int r = 0; r < 101; ++r) {
                for (int c = 0; c < resh; ++c) {
                    const double lon = lod + (bh * 300 + c * step) / 3599.0;
                    const double lat = lad + (bv * 300 + r * 3) / 3599.0;
                    put_i16(buf, base + 2U * static_cast<std::size_t>(r * resh + c), grid_value(lon, lat, morpho));
                }
            }
        }
    }
    std::ofstream out(d / name, std::ios::binary);
    out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

struct Pt {
    double lon;
    double lat;
};

// Grenzlinie zwischen F (West) und D (Ost), Sued nach Nord.
std::vector<Pt> border_line(double shift_km) {
    std::vector<Pt> pts;
    for (int i = 0; i <= 279; ++i) {
        const double lat = 48.05 + i * 0.009;
        const double lon = 8.62 + 0.04 * std::sin(lat * 9.0) -
                           shift_km / (111.2 * std::cos(lat * kPi / 180.0));
        pts.push_back({lon, lat});
    }
    return pts;
}

void put_f64(std::vector<unsigned char> &buf, double v) {
    std::uint64_t u = 0;
    static_assert(sizeof u == sizeof v, "double ist 64 Bit");
    std::memcpy(&u, &v, sizeof u);
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<unsigned char>((u >> (8U * static_cast<unsigned>(i))) & 0xFFU));
    }
}

// 10 Punkte je Record plus Mittelpunkt; der letzte Record wird mit dem
// letzten Punkt aufgefuellt.
void write_line_file(const fs::path &file, const std::vector<Pt> &pts) {
    std::vector<unsigned char> buf;
    const double rad = kPi / 180.0;
    for (std::size_t s = 0; s < pts.size(); s += 10) {
        double slon = 0.0;
        double slat = 0.0;
        for (std::size_t k = 0; k < 10; ++k) {
            const Pt &p = pts[std::min(s + k, pts.size() - 1)];
            put_f64(buf, p.lon * rad);
            put_f64(buf, p.lat * rad);
            slon += p.lon;
            slat += p.lat;
        }
        put_f64(buf, slon / 10.0 * rad);
        put_f64(buf, slat / 10.0 * rad);
    }
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
}

void write_borders(const fs::path &root) {
    const fs::path d = root / "border";
    fs::create_directories(d);
    const std::vector<Pt> line = border_line(0.0);
    write_line_file(d / "D__F__.000", line);
    write_line_file(d / "D__F__.cbr", line);
    write_line_file(d / "D__F__.020", border_line(20.0));
    std::vector<Pt> rev(line.rbegin(), line.rend());
    write_line_file(d / "F__D__.000", rev);
    // Landesgrenzen als geschlossene Polygone.
    std::vector<Pt> dpoly = line;
    dpoly.push_back({9.95, line.back().lat});
    dpoly.push_back({9.95, line.front().lat});
    dpoly.push_back(line.front());
    write_line_file(d / "D__.all", dpoly);
    std::vector<Pt> fpoly = rev;
    fpoly.push_back({8.02, line.front().lat});
    fpoly.push_back({8.02, line.back().lat});
    fpoly.push_back(rev.front());
    write_line_file(d / "F__.all", fpoly);
}

}  // namespace

// FNV-1a (64 Bit) ueber relative Pfade und Inhalte aller Dateien, sortiert.
std::uint64_t checksum(const fs::path &root) {
    std::vector<fs::path> files;
    for (const auto &e : fs::recursive_directory_iterator(root)) {
        if (e.is_regular_file()) {
            files.push_back(e.path());
        }
    }
    std::sort(files.begin(), files.end());
    std::uint64_t h = 1469598103934665603ULL;
    const auto mix = [&h](unsigned char c) {
        h ^= c;
        h *= 1099511628211ULL;
    };
    for (const fs::path &f : files) {
        for (const char c : fs::relative(f, root).generic_string()) {
            mix(static_cast<unsigned char>(c));
        }
        std::ifstream in(f, std::ios::binary);
        for (std::istreambuf_iterator<char> it(in), end; it != end; ++it) {
            mix(static_cast<unsigned char>(*it));
        }
    }
    return h;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Aufruf: hcmms_fixtures <zielverzeichnis>\n");
        return 2;
    }
    const fs::path root(argv[1]);
    for (int lod = 8; lod <= 9; ++lod) {
        for (int lad = 48; lad <= 50; ++lad) {
            write_tile(root, lod, lad, false);
            if (!(lod == 9 && lad == 50)) {
                write_tile(root, lod, lad, true);
            }
        }
    }
    write_borders(root);
    std::printf("fnv=%016llx\n", static_cast<unsigned long long>(checksum(root)));
    return 0;
}
