#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <io.h>
#else
extern "C" int fileno(FILE *);
#endif
#include <vector>
namespace update {
inline constexpr const char *Version = "1.1.0";
inline constexpr const char *Channel =
    "https://github.com/monster-hub14/SwitchTalk/releases/latest/"
    "download/update.txt";
inline constexpr const char *Releases =
    "https://github.com/monster-hub14/SwitchTalk/releases/download/"
    "v";
inline constexpr const char *Root = "sdmc:/switch/SwitchTalk";
inline constexpr uint64_t MaxSize = 64 * 1024 * 1024;
struct SHA256 {
  uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  uint8_t block[64]{};
  size_t used = 0;
  uint64_t bytes = 0;
  static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
  void compress() {
    static constexpr uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
      w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
             (uint32_t(block[i * 4 + 2]) << 8) | block[i * 4 + 3];
    for (int i = 16; i < 64; i++) {
      auto a = w[i - 15], b = w[i - 2];
      w[i] = w[i - 16] + (rotr(a, 7) ^ rotr(a, 18) ^ (a >> 3)) + w[i - 7] +
             (rotr(b, 17) ^ rotr(b, 19) ^ (b >> 10));
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], v = h[7];
    for (int i = 0; i < 64; i++) {
      auto t1 = v + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
      auto t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
      v = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    uint32_t z[] = {a, b, c, d, e, f, g, v};
    for (int i = 0; i < 8; i++)
      h[i] += z[i];
  }
  void add(const void *ptr, size_t n) {
    auto *p = static_cast<const uint8_t *>(ptr);
    bytes += n;
    while (n) {
      auto count = std::min(n, 64 - used);
      memcpy(block + used, p, count);
      used += count;
      p += count;
      n -= count;
      if (used == 64) {
        compress();
        used = 0;
      }
    }
  }
  std::string finish() {
    uint64_t bits = bytes * 8;
    uint8_t one = 0x80, zero = 0;
    add(&one, 1);
    while (used != 56)
      add(&zero, 1);
    uint8_t end[8];
    for (int i = 0; i < 8; i++)
      end[7 - i] = uint8_t(bits >> (8 * i));
    add(end, 8);
    std::string s;
    const char *hex = "0123456789abcdef";
    for (auto v : h)
      for (int j = 7; j >= 0; j--)
        s += hex[(v >> (j * 4)) & 15];
    return s;
  }
};
inline bool version(const std::string &s, std::array<int, 3> &out) {
  if (s.empty() || s.size() > 23)
    return false;
  size_t p = 0;
  for (int i = 0; i < 3; i++) {
    size_t start = p;
    int n = 0;
    while (p < s.size() && s[p] >= '0' && s[p] <= '9') {
      if (p - start >= 5)
        return false;
      n = n * 10 + s[p++] - '0';
    }
    if (p == start || (p - start > 1 && s[start] == '0'))
      return false;
    out[i] = n;
    if (i < 2) {
      if (p == s.size() || s[p++] != '.')
        return false;
    }
  }
  return p == s.size();
}
inline int compare(const std::string &a, const std::string &b) {
  std::array<int, 3> x{}, y{};
  if (!version(a, x) || !version(b, y))
    return 0;
  return x < y ? -1 : x > y ? 1 : 0;
}
struct Manifest {
  std::string version, sha;
  uint64_t size = 0;
  std::string encode() const {
    return "SWITCHTALK-UPDATE-1\nversion=" + version + "\nsize=" + std::to_string(size) +
           "\nsha256=" + sha + "\nprotocol=SWTU1\nasset=SwitchTalkApp.nro\nmin_launcher=1\n";
  }
};
inline bool parse(const std::string &s, Manifest &m) {
  if (s.size() > 1024 || s.find('\0') != std::string::npos)
    return false;
  size_t p = 0;
  std::array<std::string, 7> lines;
  for (auto &line : lines) {
    size_t e = s.find('\n', p);
    if (e == std::string::npos)
      return false;
    line = s.substr(p, e - p);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    p = e + 1;
  }
  if (p != s.size() || lines[0] != "SWITCHTALK-UPDATE-1" || lines[1].rfind("version=", 0) ||
      lines[2].rfind("size=", 0) || lines[3].rfind("sha256=", 0) || lines[4] != "protocol=SWTU1" ||
      lines[5] != "asset=SwitchTalkApp.nro" || lines[6] != "min_launcher=1")
    return false;
  Manifest n;
  n.version = lines[1].substr(8);
  std::array<int, 3> v;
  if (!version(n.version, v))
    return false;
  auto z = lines[2].substr(5);
  if (z.empty() || z.size() > 8)
    return false;
  for (char c : z) {
    if (c < '0' || c > '9')
      return false;
    n.size = n.size * 10 + c - '0';
  }
  if (n.size < 128 || n.size > MaxSize)
    return false;
  n.sha = lines[3].substr(7);
  if (n.sha.size() != 64 || n.sha.find_first_not_of("0123456789abcdef") != std::string::npos)
    return false;
  m = n;
  return true;
}
inline uint32_t le32(const uint8_t *p) {
  return p[0] | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
inline uint64_t le64(const uint8_t *p) { return le32(p) | (uint64_t(le32(p + 4)) << 32); }
inline bool bounded(uint64_t off, uint64_t size, uint64_t total) {
  return off <= total && size <= total - off;
}
inline bool nro(FILE *f, uint64_t total, const std::string &expected) {
  uint8_t h[128]{};
  if (fseek(f, 0, SEEK_SET) || fread(h, 1, 128, f) != 128 || memcmp(h + 16, "NRO0", 4))
    return false;
  uint32_t code = le32(h + 24);
  if (code < 128 || code > total || code % 4096)
    return false;
  uint64_t prev = 0;
  for (int i = 0; i < 3; i++) {
    auto off = le32(h + 32 + i * 8), size = le32(h + 36 + i * 8);
    if (off != prev || !bounded(off, size, code))
      return false;
    prev = uint64_t(off) + size;
  }
  if (prev != code)
    return false;
  uint8_t aset[56];
  if (fseek(f, code, SEEK_SET) || fread(aset, 1, 56, f) != 56 || memcmp(aset, "ASET", 4))
    return false;
  uint64_t tail = total - code, na = le64(aset + 24), ns = le64(aset + 32), ro = le64(aset + 40),
           rs = le64(aset + 48);
  if (ns != 0x4000 || na < 56 || !bounded(na, ns, tail) || !bounded(ro, rs, tail) || rs < 80 ||
      ro < na + ns || ro + rs != tail)
    return false;
  char title[11]{}, v[16]{};
  if (fseek(f, long(code + na), SEEK_SET) || fread(title, 1, 11, f) != 11 ||
      memcmp(title, "SwitchTalk", 11))
    return false;
  if (fseek(f, long(code + na + 0x3060), SEEK_SET) || fread(v, 1, 16, f) != 16 || v[15] != 0)
    return false;
  return std::string(v) == expected;
}
inline std::string readText(const std::string &path, size_t max = 1024) {
  FILE *f = fopen(path.c_str(), "rb");
  if (!f)
    return {};
  std::string s(max + 1, '\0');
  size_t n = fread(s.data(), 1, s.size(), f);
  bool ok = !ferror(f) && n <= max;
  fclose(f);
  if (!ok)
    return {};
  s.resize(n);
  return s;
}
inline bool flush(FILE *f) {
#ifdef _WIN32
  return fflush(f) == 0 && _commit(_fileno(f)) == 0;
#else
  return fflush(f) == 0 && fsync(fileno(f)) == 0;
#endif
}
inline bool writeText(const std::string &path, const std::string &s) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f)
    return false;
  bool ok = fwrite(s.data(), 1, s.size(), f) == s.size() && flush(f);
  return fclose(f) == 0 && ok;
}
inline bool exists(const std::string &p) {
  struct stat st{};
  return stat(p.c_str(), &st) == 0;
}
inline bool erase(const std::string &p) { return !exists(p) || remove(p.c_str()) == 0; }
inline bool replace(const std::string &from, const std::string &to) {
  return erase(to) && rename(from.c_str(), to.c_str()) == 0;
}
inline bool writeSafe(const std::string &path, const std::string &text) {
  return writeText(path + ".tmp", text) && replace(path + ".tmp", path);
}
inline bool verify(const std::string &path, const Manifest &m) {
  FILE *f = fopen(path.c_str(), "rb");
  if (!f)
    return false;
  uint8_t buf[16384];
  SHA256 h;
  uint64_t n = 0;
  size_t got = 0;
  while ((got = fread(buf, 1, sizeof(buf), f)) > 0) {
    n += got;
    if (n > m.size)
      break;
    h.add(buf, got);
  }
  bool ok = !ferror(f) && n == m.size && h.finish() == m.sha && nro(f, n, m.version);
  fclose(f);
  return ok;
}
struct Slots {
  std::string root;
  explicit Slots(std::string p = Root) : root(std::move(p)) {}
  std::string file(char s) const { return root + "/app-" + s + ".nro"; }
  std::string meta(char s) const { return root + "/app-" + s + ".txt"; }
  char active() const {
    auto s = readText(root + "/active.txt", 4);
    return s == "a\n" ? 'a' : s == "b\n" ? 'b' : 0;
  }
  bool valid(char s, Manifest &out) const {
    return (s == 'a' || s == 'b') && parse(readText(meta(s)), out) && verify(file(s), out);
  }
  char choose() const {
    Manifest a, b;
    char current = active();
    if (current && valid(current, a))
      return current;
    bool va = valid('a', a), vb = valid('b', b);
    return va && vb ? (compare(a.version, b.version) >= 0 ? 'a' : 'b') : va ? 'a' : vb ? 'b' : 0;
  }
  char bootChoice(bool &recovered) const {
    recovered = false;
    char slot = choose();
    auto boot = readText(root + "/booting.txt", 4);
    if (slot && boot == std::string(1, slot) + "\n") {
      char other = slot == 'a' ? 'b' : 'a';
      Manifest m;
      if (valid(other, m)) {
        if (!activate(other))
          return 0;
        writeSafe(root + "/updates.cfg", "off\n");
        slot = other;
        recovered = true;
      }
    }
    return slot;
  }
  bool activate(char s) const {
    Manifest m;
    if (!valid(s, m))
      return false;
    return writeSafe(root + "/active.txt", std::string(1, s) + "\n");
  }
  // The currently running slot is never removed or written, even if staging
  // fails.
  bool stage(char running, const Manifest &m) const {
    if (running != 'a' && running != 'b')
      return false;
    char next = running == 'a' ? 'b' : 'a';
    auto part = file(next) + ".part";
    if (!verify(part, m))
      return false;
    if (!replace(part, file(next)))
      return false;
    Manifest checked;
    return writeSafe(meta(next), m.encode()) && valid(next, checked);
  }
};
} // namespace update
