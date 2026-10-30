#pragma once

#include "update-core.hpp"
#include <atomic>
#include <mutex>
#if defined(__SWITCH__) || defined(SYSTEM_BREACH_HTTP_TEST)
#include <curl/curl.h>
#include <switch.h>
#include <sys/statvfs.h>
#endif
namespace update {
enum class Phase { Idle, Checking, Downloading, Ready, Current, Error };
struct Status {
  Phase phase = Phase::Idle;
  std::string message = "CHECKING STARTS AT THE MAIN MENU", version;
  uint64_t downloaded = 0, total = 0;
};
struct Manager {
  Slots slots;
  char running = 0;
  bool automatic = true, started = false, stopped = false, deferred = false, restart = false;
  double readySince = -1;
  std::atomic<bool> cancel{false}, busy{false};
  std::mutex mutex;
  Status status;
  Manifest candidate;
#ifdef __SWITCH__
  Thread thread{};
  bool threadCreated = false, curlReady = false;
#endif
  explicit Manager(std::string root=Root):slots(std::move(root)) { automatic = readText(slots.root + "/updates.cfg", 8) != "off\n"; }
  void init(const std::string &path) {
    if (path == slots.file('a') || path == slots.file('a').substr(5))
      running = 'a';
    if (path == slots.file('b') || path == slots.file('b').substr(5))
      running = 'b';
  }
  Status snapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return status;
  }
  void set(Phase p, const std::string &msg) {
    std::lock_guard<std::mutex> lock(mutex);
    status.phase = p;
    status.message = msg;
  }
  bool configure(bool enabled) {
    if (!writeSafe(slots.root + "/updates.cfg", enabled ? "on\n" : "off\n"))
      return false;
    automatic = enabled;
    if(!enabled)cancel=true;
    deferred = !enabled;
    return true;
  }
#if defined(__SWITCH__) || defined(SYSTEM_BREACH_HTTP_TEST)
  struct Transfer {
    Manager *self;
    std::string *text = nullptr;
    FILE *file = nullptr;
    uint64_t limit = 0, received = 0;
  };
  static size_t receive(char *data, size_t size, size_t count, void *ptr) {
    auto &t = *static_cast<Transfer *>(ptr);
    if (size && count > SIZE_MAX / size)
      return 0;
    size_t n = size * count;
    if (t.self->cancel || n > t.limit - t.received)
      return 0;
    if (t.text)
      t.text->append(data, n);
    else if (!t.file || fwrite(data, 1, n, t.file) != n)
      return 0;
    t.received += n;
    if (t.file) {
      std::lock_guard<std::mutex> lock(t.self->mutex);
      t.self->status.downloaded = t.received;
    }
    return n;
  }
  static int progress(void *p, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return static_cast<Manager *>(p)->cancel ? 1 : 0;
  }
  bool fetch(const std::string &url, Transfer &t) {
    CURL *c = curl_easy_init();
    if (!c)
      return false;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_USERAGENT, "SwitchTalk/1.1.0");
    curl_easy_setopt(c, CURLOPT_CAINFO, "romfs:/update/cacert.pem");
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(c, CURLOPT_PROTOCOLS, long(CURLPROTO_HTTPS));
    curl_easy_setopt(c, CURLOPT_REDIR_PROTOCOLS, long(CURLPROTO_HTTPS));
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, t.file ? 180L : 15L);
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_LIMIT, 512L);
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_TIME, 15L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c, CURLOPT_IPRESOLVE, long(CURL_IPRESOLVE_V4));
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, receive);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &t);
    curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(c, CURLOPT_XFERINFOFUNCTION, progress);
    curl_easy_setopt(c, CURLOPT_XFERINFODATA, this);
    auto result = curl_easy_perform(c);
    long code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(c);
    return result == CURLE_OK && code == 200 && !cancel;
  }
  void work() {
    set(Phase::Checking, "CHECKING GITHUB FOR A NEW VERSION");
    std::string text;
    Transfer t{this, &text, nullptr, 1024, 0};
    if (!fetch(Channel, t)) {
      set(Phase::Error, cancel ? "CHECK CANCELLED - YOUR APP IS UNCHANGED"
                               : "UPDATE SERVER UNAVAILABLE - PLAY OFFLINE OR A TO RETRY");
      return;
    }
    Manifest m;
    if (!parse(text, m)) {
      set(Phase::Error, "UPDATE DETAILS WERE INVALID - APP UNCHANGED");
      return;
    }
    if (compare(m.version, Version) <= 0) {
      set(Phase::Current, "YOU HAVE THE LATEST VERSION");
      return;
    }
    if (!running) {
      set(Phase::Error, "INSTALL THE COMPLETE ZIP TO ENABLE AUTOMATIC UPDATES");
      return;
    }
    struct statvfs space{};
    if (statvfs(slots.root.c_str(), &space) ||
        uint64_t(space.f_bavail) * space.f_frsize < m.size + 2 * 1024 * 1024) {
      set(Phase::Error, "NOT ENOUGH FREE SD SPACE - NEED UPDATE PLUS 2 MB");
      return;
    }
    char next = running == 'a' ? 'b' : 'a';
    std::string part = slots.file(next) + ".part";
    FILE *f = fopen(part.c_str(), "wb");
    if (!f) {
      set(Phase::Error, "CANNOT WRITE TO THE SD CARD - APP UNCHANGED");
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      status = {Phase::Downloading, "DOWNLOADING - YOU CAN KEEP PLAYING", m.version, 0, m.size};
    }
    Transfer download{this, nullptr, f, m.size, 0};
    bool ok = fetch(std::string(Releases) + m.version + "/SwitchTalkApp.nro", download) &&
              download.received == m.size;
    if (ok)
      ok = flush(f);
    if (fclose(f))
      ok = false;
    if (!ok) {
      erase(part);
      set(Phase::Error, cancel ? "DOWNLOAD CANCELLED - YOUR APP IS UNCHANGED"
                               : "DOWNLOAD FAILED - YOUR APP IS UNCHANGED");
      return;
    }
    if (cancel || !slots.stage(running, m)) {
      erase(part);
      set(Phase::Error, "UPDATE DID NOT PASS VERIFICATION - APP UNCHANGED");
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      candidate = m;
      status.phase = Phase::Ready;
      status.message = "UPDATE VERIFIED - READY TO RESTART";
    }
  }
#ifdef __SWITCH__
  static void entry(void *arg) {
    auto *self = static_cast<Manager *>(arg);
    self->work();
    self->busy = false;
  }
  void reap() {
    if (threadCreated && !busy) {
      threadWaitForExit(&thread);
      threadClose(&thread);
      threadCreated = false;
    }
  }
#endif
#endif
  void start() {
#ifdef __SWITCH__
    reap();
    if (busy)
      return;
    if (!curlReady) {
      curlReady = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
      if (!curlReady) {
        set(Phase::Error, "COULD NOT START UPDATE NETWORKING");
        return;
      }
    }
    cancel = false;
    busy = true;
    deferred = false;
    readySince = -1;
    set(Phase::Checking, "CHECKING GITHUB FOR A NEW VERSION");
    if (R_FAILED(threadCreate(&thread, entry, this, nullptr, 512 * 1024, 0x31, -2))) {
      busy = false;
      set(Phase::Error, "COULD NOT START UPDATE WORKER");
      return;
    }
    threadCreated = true;
    if (R_FAILED(threadStart(&thread))) {
      threadClose(&thread);
      threadCreated = false;
      busy = false;
      set(Phase::Error, "COULD NOT START UPDATE WORKER");
    }
#else
    set(Phase::Current, "DESKTOP TEST - NETWORK WORKER DISABLED");
#endif
  }
  void tick(bool idle, double now) {
#ifdef __SWITCH__
    reap();
#endif
    if (!started && idle) {
      started = true;
      if (automatic)
        start();
    }
    auto s = snapshot();
    if (s.phase != Phase::Ready || !idle || busy || deferred) {
      readySince = -1;
      return;
    }
    if (readySince < 0)
      readySince = now;
    if (automatic && now - readySince >= 5)
      restart = true;
  }
  void input(uint64_t key, uint64_t a, uint64_t b, uint64_t x, uint64_t y) {
    auto s = snapshot();
    if (key & b) {
      deferred = true;
      readySince = -1;
    }
    if (key & x) {
      if (!configure(!automatic))
        set(Phase::Error, "COULD NOT SAVE UPDATE SETTING");
    }
    if (key & a) {
      if (s.phase == Phase::Ready && !busy)
        restart = true;
      else if (!busy)
        start();
    }
    if ((key & y) && !busy && running) {
      char previous = running == 'a' ? 'b' : 'a';
      Manifest m;
      if (slots.valid(previous, m) && compare(m.version,Version)<0) {
        if (configure(false) && slots.activate(previous)) {
          restart = true;
          candidate = {};
        } else
          set(Phase::Error, "COULD NOT SELECT THE BACKUP");
      } else
        set(Phase::Error, "NO VERIFIED PREVIOUS VERSION AVAILABLE");
    }
  }
  bool commit() {
    if (busy || !running)
      return false;
    auto s = snapshot();
    if (s.phase == Phase::Ready && !candidate.version.empty()) {
      char next = running == 'a' ? 'b' : 'a';
      if (!slots.activate(next)) {
        set(Phase::Error, "COULD NOT ACTIVATE UPDATE - ORIGINAL RETAINED");
        restart = false;
        deferred = true;
        return false;
      }
    }
    return true;
  }
  void stop() {
    if (stopped)
      return;
    cancel = true;
#ifdef __SWITCH__
    if (threadCreated) {
      threadWaitForExit(&thread);
      threadClose(&thread);
      threadCreated = false;
    }
    if (curlReady) {
      curl_global_cleanup();
      curlReady = false;
    }
#endif
    busy = false;
    stopped = true;
  }
  ~Manager() { stop(); }
};
} // namespace update
