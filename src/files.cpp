#include "files.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace hcmms {

namespace {

namespace fs = std::filesystem;

bool iequal(const std::string &a, const std::string &b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

// Sucht jede Komponente von `p` ohne Ruecksicht auf Gross-/Kleinschreibung.
std::string resolve_case(const std::string &p) {
    const fs::path in(p);
    fs::path cur = in.root_path();
    std::error_code ec;
    for (const auto &part : in.relative_path()) {
        const fs::path exact = cur / part;
        if (fs::exists(exact, ec)) {
            cur = exact;
            continue;
        }
        bool found = false;
        const fs::path dir = cur.empty() ? fs::path(".") : cur;
        for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
            if (iequal(it->path().filename().string(), part.string())) {
                cur /= it->path().filename();
                found = true;
                break;
            }
        }
        if (!found) {
            return p;
        }
    }
    return cur.string();
}

}  // namespace

bool open_binary(std::ifstream &f, const std::string &path) {
    if (f.is_open()) {
        f.close();
    }
    f.clear();
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
        const std::string alt = resolve_case(path);
        if (!fs::is_regular_file(alt, ec)) {
            return false;
        }
        f.open(alt, std::ios::binary);
    } else {
        f.open(path, std::ios::binary);
    }
    return f.is_open();
}

bool read_record(std::ifstream &f, long rec, std::size_t len, unsigned char *buf) {
    if (!f.is_open() || rec < 1) {
        return false;
    }
    f.clear();
    f.seekg(static_cast<std::streamoff>(rec - 1) * static_cast<std::streamoff>(len));
    f.read(reinterpret_cast<char *>(buf), static_cast<std::streamsize>(len));
    return f.good() && static_cast<std::size_t>(f.gcount()) == len;
}

}  // namespace hcmms
