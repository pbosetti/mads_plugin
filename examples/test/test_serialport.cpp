/*
  ____            _       _ ____            _     _____         _
 / ___|  ___ _ __(_) __ _| |  _ \ ___  _ __| |_  |_   _|__  ___| |_
 \___ \ / _ \ '__| |/ _` | | |_) / _ \| '__| __|   | |/ _ \/ __| __|
  ___) |  __/ |  | | (_| | |  __/ (_) | |  | |_    | |  __/\__ \ |_
 |____/ \___|_|  |_|\__,_|_|_|   \___/|_|   \__|   |_|\___||___/\__|

Test suite for the SerialPort class. The checks that need a real port are run
over a pty pair, which exists on POSIX systems only; on Windows the suite runs
the subset that needs no device. The program exits with 0 when every check
passes, with 1 otherwise.
*/

#include "serialport.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#if !defined(_WIN32)
#include <array>
#include <cstdlib>
#include <fcntl.h>
#include <span>
#include <termios.h>
#include <unistd.h>
#endif

using namespace std::chrono_literals;

static int Failures = 0; /**< Number of checks that did not pass */
static int Checks = 0;   /**< Number of checks that were run */

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++Checks;                                                                  \
    if (!(cond)) {                                                             \
      ++Failures;                                                              \
      std::cout << "  FAIL " << __LINE__ << ": " << #cond << "\n";             \
    }                                                                          \
  } while (0)

#define CHECK_EQ(a, b)                                                         \
  do {                                                                         \
    ++Checks;                                                                  \
    const auto value_a = (a);                                                  \
    const auto value_b = (b);                                                  \
    if (!(value_a == value_b)) {                                               \
      ++Failures;                                                              \
      std::cout << "  FAIL " << __LINE__ << ": " << #a << " == " << #b << " (" \
                << value_a << " != " << value_b << ")\n";                      \
    }                                                                          \
  } while (0)

/* CHECKS THAT NEED NO DEVICE ***********************************************/

/** A configuration is validated before the port is even opened. */
static void test_validation() {
  std::cout << "[validation]\n";
  const char *const missing = "definitely_not_a_serial_port";
  const auto rejected = [missing](const SerialPort::Config &config) {
    try {
      SerialPort port(missing, config);
    } catch (const std::invalid_argument &) {
      return true;
    } catch (const std::exception &) {
      return false;
    }
    return false;
  };

  CHECK(rejected({.stop_bits = 3}));
  CHECK(rejected({.data_bits = 9}));
  CHECK(rejected({.data_bits = 4}));
  CHECK(rejected({.baud_rate = 0}));
  CHECK(rejected({.max_line_length = 0}));

  bool threw = false;
  try {
    SerialPort port("");
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  CHECK(threw);
}

/** Opening a port that is not there fails with a readable message. */
static void test_missing_port() {
  std::cout << "[missing port]\n";
#if defined(_WIN32)
  const char *const missing = "COM231";
#else
  const char *const missing = "/dev/does/not/exist";
#endif
  bool threw = false;
  try {
    SerialPort port(missing);
  } catch (const std::system_error &e) {
    threw = true;
    CHECK(std::string(e.what()).find(missing) != std::string::npos);
    std::cout << "  " << e.what() << "\n";
  }
  CHECK(threw);
}

/** Enumerating the ports must work, whether or not any is present. */
static void test_port_list() {
  std::cout << "[available_ports]\n";
  const auto ports = SerialPort::available_ports();
  std::cout << "  found " << ports.size() << " port(s)";
  for (std::size_t i = 0; i < ports.size() && i < 4; ++i) {
    std::cout << (i ? ", " : ": ") << ports[i];
  }
  std::cout << "\n";
  for (const auto &name : ports) {
    CHECK(!name.empty());
  }
}

/* CHECKS THAT NEED A PORT **************************************************/

#if !defined(_WIN32)

/** A pty pair: the master stands in for the device at the other end of the
 * cable, the slave is the port SerialPort opens. */
class Pty {
public:
  Pty() {
    _master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (_master < 0 || ::grantpt(_master) < 0 || ::unlockpt(_master) < 0) {
      std::cout << "cannot create a pty: " << std::strerror(errno) << "\n";
      std::exit(2);
    }
    // Non-blocking, so that a missing answer times out instead of hanging.
    ::fcntl(_master, F_SETFL, O_NONBLOCK);
    _slave = ::ptsname(_master);
  }
  ~Pty() { hangup(); }
  Pty(const Pty &) = delete;
  Pty &operator=(const Pty &) = delete;

  const std::string &slave() const { return _slave; }

  /** Send data to the port. */
  void send(std::string_view data) {
    const ssize_t n = ::write(_master, data.data(), data.size());
    if (n != static_cast<ssize_t>(data.size())) {
      std::cout << "  (short pty write)\n";
    }
  }

  /** Collect what the port has written, up to n bytes. */
  std::string receive(std::size_t n, std::chrono::milliseconds limit = 2s) {
    std::string out;
    char buf[256];
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (out.size() < n && std::chrono::steady_clock::now() < deadline) {
      const ssize_t got = ::read(_master, buf, sizeof(buf));
      if (got > 0) {
        out.append(buf, static_cast<std::size_t>(got));
      } else {
        std::this_thread::sleep_for(1ms);
      }
    }
    return out;
  }

  /** Unplug the cable. */
  void hangup() {
    if (_master >= 0) {
      ::close(_master);
      _master = -1;
    }
  }

private:
  int _master = -1;
  std::string _slave;
};

/** Wait until the port has at least n bytes to read, so that the checks do
 * not depend on how busy the machine is. */
static void wait_for_input(const SerialPort &port, std::size_t n,
                           std::chrono::milliseconds limit = 2s) {
  const auto deadline = std::chrono::steady_clock::now() + limit;
  while (port.available() < n && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
  }
}

static void test_line_reading(Pty &pty, const std::string &dev) {
  std::cout << "[line reading]\n";
  SerialPort port(dev.c_str(), 115200);
  CHECK(port.is_open());
  CHECK_EQ(port.baud_rate(), 115200u);
  CHECK_EQ(port.port(), dev);
  CHECK(port.native_handle() != SerialPort::invalid_handle);
  CHECK(port.timeout() == SerialPort::no_timeout);

  pty.send("hello\nworld\r\nthird\rfourth\n");
  std::string line;
  CHECK_EQ(port.read_line(line), 5);
  CHECK_EQ(line, "hello");
  CHECK_EQ(port.readLine(line), 5); // legacy alias
  CHECK_EQ(line, "world");
  // a CRLF pair must not produce a spurious empty line
  CHECK_EQ(port.read_line(line), 5);
  CHECK_EQ(line, "third");
  CHECK_EQ(port.read_line(line), 6);
  CHECK_EQ(line, "fourth");

  // empty lines are zero-length lines, not errors
  pty.send("\n\nx\n");
  CHECK_EQ(port.read_line(line), 0);
  CHECK(line.empty());
  CHECK(!port.timed_out());
  CHECK_EQ(port.read_line(line), 0);
  CHECK_EQ(port.read_line(line), 1);
  CHECK_EQ(line, "x");
}

static void test_settings_restored(const std::string &dev) {
  std::cout << "[settings restored on close]\n";
  termios before{};
  int fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  CHECK(fd >= 0);
  CHECK(::tcgetattr(fd, &before) == 0);
  ::close(fd);

  { SerialPort port(dev.c_str(), 115200, 2); }

  termios after{};
  fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  CHECK(fd >= 0);
  CHECK(::tcgetattr(fd, &after) == 0);
  CHECK_EQ(after.c_cflag, before.c_cflag);
  CHECK_EQ(after.c_lflag, before.c_lflag);
  CHECK_EQ(after.c_iflag, before.c_iflag);
  CHECK_EQ(after.c_oflag, before.c_oflag);
  ::close(fd);
}

static void test_timeouts(Pty &pty, const std::string &dev) {
  std::cout << "[timeouts]\n";
  SerialPort port(dev, SerialPort::Config{.baud_rate = 9600, .timeout = 150ms});

  const auto start = std::chrono::steady_clock::now();
  std::string line;
  CHECK_EQ(port.read_line(line), 0);
  CHECK(port.timed_out());
  const auto elapsed = std::chrono::steady_clock::now() - start;
  CHECK(elapsed >= 140ms); // it really waited
  CHECK(elapsed < 5s);     // and it really gave up

  // a partial line comes back with timed_out() set
  pty.send("abc");
  wait_for_input(port, 3);
  CHECK_EQ(port.read_line(line), 3);
  CHECK_EQ(line, "abc");
  CHECK(port.timed_out());

  // read() returns fewer bytes than asked for instead of blocking
  char buf[16];
  pty.send("12345");
  wait_for_input(port, 5);
  CHECK_EQ(port.read(buf, sizeof(buf)), 5);
  CHECK(port.timed_out());
  CHECK_EQ(std::string(buf, 5), "12345");

  // the optional flavour reports a timeout as no value
  CHECK(!port.read_line().has_value());
}

static void test_reads(Pty &pty, const std::string &dev) {
  std::cout << "[exact reads and spans]\n";
  SerialPort port(dev, SerialPort::Config{.timeout = 2s});
  pty.send("0123456789");
  wait_for_input(port, 10);
  CHECK(port.available() >= 10);

  char four[4];
  CHECK_EQ(port.read(four, 4), 4);
  CHECK_EQ(std::string(four, 4), "0123");
  CHECK(!port.timed_out());

  std::array<std::byte, 3> bytes{};
  CHECK_EQ(port.read(std::span<std::byte>(bytes)), 3);
  CHECK_EQ(static_cast<char>(bytes[0]), '4');

  std::vector<char> rest(3);
  CHECK_EQ(port.read(std::span<char>(rest)), 3);
  CHECK_EQ(std::string(rest.begin(), rest.end()), "789");
  CHECK_EQ(port.available(), 0u);

  // read_some() returns whatever has arrived
  pty.send("xy");
  char some[16];
  const int n = port.read_some(some, sizeof(some));
  CHECK(n >= 1 && n <= 2);
}

static void test_writes(Pty &pty, const std::string &dev) {
  std::cout << "[writing]\n";
  SerialPort port(dev, SerialPort::Config{.timeout = 2s});
  CHECK_EQ(port.write("abc", 3), 3);
  CHECK_EQ(pty.receive(3), "abc");
  CHECK_EQ(port.write(std::string("de")), 2);
  CHECK_EQ(pty.receive(2), "de");
  CHECK_EQ(port.write("fg"), 2); // string_view from a literal
  CHECK_EQ(pty.receive(2), "fg");
  CHECK_EQ(port.write_line("hi"), 3);
  CHECK_EQ(pty.receive(3), "hi\n");
  std::array<std::byte, 2> raw{std::byte{'z'}, std::byte{'w'}};
  CHECK_EQ(port.write(std::span<const std::byte>(raw)), 2);
  CHECK_EQ(pty.receive(2), "zw");
  CHECK_EQ(port.write(nullptr, 0), 0); // an empty write is a no-op
  CHECK_EQ(port.write(""), 0);
}

static void test_c_buffer_lines(Pty &pty, const std::string &dev) {
  std::cout << "[C buffer lines and truncation]\n";
  {
    SerialPort port(dev, SerialPort::Config{.timeout = 2s});
    pty.send("abcdefgh\nshort\n");
    char line[6];
    CHECK_EQ(port.read_line(line, sizeof(line)), 5);
    CHECK_EQ(std::string(line), "abcde");
    CHECK(port.last_error() == std::errc::message_size);
    // the tail of the long line is dropped, so the framing is preserved
    CHECK_EQ(port.readLine(line, sizeof(line)), 5);
    CHECK_EQ(std::string(line), "short");
    CHECK(!port.last_error());
    CHECK_EQ(port.read_line(line, 0), -1);
    CHECK(port.last_error() == std::errc::invalid_argument);
  }
  {
    // max_line_length truncates without losing what follows
    SerialPort port(dev,
                    SerialPort::Config{.timeout = 2s, .max_line_length = 4});
    pty.send("abcdefgh\nnext\n");
    std::string line;
    CHECK_EQ(port.read_line(line), 4);
    CHECK_EQ(line, "abcd");
    CHECK(port.last_error() == std::errc::message_size);
    CHECK_EQ(port.read_line(line), 4);
    CHECK_EQ(line, "next");
    CHECK(!port.last_error());
  }
}

static void test_open_errors(const std::string &dev) {
  std::cout << "[open errors]\n";
  bool threw = false;
  try {
    SerialPort port("/dev/null"); // exists, but is not a tty
  } catch (const std::system_error &) {
    threw = true;
  }
  CHECK(threw);

  // A non standard rate is refused up front on Linux; macOS asks the driver,
  // which a pty cannot satisfy. Either way it must not be accepted silently.
  threw = false;
  try {
    SerialPort port(dev.c_str(), 12345678);
  } catch (const std::exception &e) {
    threw = true;
    std::cout << "  custom rate: " << e.what() << "\n";
  }
  CHECK(threw);

  // the port is still usable after a failed construction
  SerialPort port(dev.c_str(), 9600);
  CHECK(port.is_open());
}

static void test_move_and_close(Pty &pty, const std::string &dev) {
  std::cout << "[move semantics and close]\n";
  SerialPort a(dev.c_str(), 9600);
  const SerialPort::NativeHandle handle = a.native_handle();
  SerialPort b(std::move(a));
  CHECK(!a.is_open());
  CHECK(b.native_handle() == handle);
  CHECK_EQ(b.port(), dev);

  std::string line;
  CHECK_EQ(a.read_line(line), -1); // a moved-from port fails cleanly
  CHECK(a.last_error() == std::errc::bad_file_descriptor);

  b.set_timeout(2s);
  pty.send("moved\n");
  CHECK_EQ(b.read_line(line), 5);
  CHECK_EQ(line, "moved");

  b.close();
  CHECK(!b.is_open());
  b.close(); // idempotent
  CHECK_EQ(b.read_line(line), -1);
  CHECK_EQ(b.write("x"), -1);

  SerialPort c(dev.c_str(), 9600);
  c = std::move(b); // move-assign over an open port
  CHECK(!c.is_open());
}

static void test_reconfiguration(Pty &pty, const std::string &dev) {
  std::cout << "[reconfiguration and control]\n";
  SerialPort port(dev.c_str(), 9600);
  port.set_baud_rate(115200);
  CHECK_EQ(port.baud_rate(), 115200u);

  port.set_timeout(150ms);
  pty.send("junk\n");
  wait_for_input(port, 5);
  port.flush_input();
  CHECK_EQ(port.available(), 0u);
  std::string line;
  CHECK_EQ(port.read_line(line), 0);
  CHECK(port.timed_out());

  port.flush_output();
  port.flush();
  port.drain();

  bool threw = false;
  try {
    port.set_baud_rate(0); // invalid everywhere
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  CHECK(threw);
  CHECK_EQ(port.baud_rate(), 115200u); // unchanged after a failure

  // and the port still works at the rate it was left on
  port.set_timeout(2s);
  pty.send("still here\n");
  CHECK_EQ(port.read_line(line), 10);
  CHECK_EQ(line, "still here");
}

static void test_frame_formats(const std::string &dev) {
  std::cout << "[frame formats]\n";
  // A pty is not a UART: it may well refuse anything but 8N1. What matters is
  // that a refusal is reported instead of being silently ignored.
  try {
    SerialPort port(
        dev,
        SerialPort::Config{.baud_rate = 19200,
                           .data_bits = 7,
                           .stop_bits = 2,
                           .parity = SerialPort::Parity::even,
                           .flow_control = SerialPort::FlowControl::software});
    CHECK(port.is_open());
    std::cout << "  7E2 with xon/xoff accepted\n";
  } catch (const std::exception &e) {
    std::cout << "  7E2 refused: " << e.what() << "\n";
  }
}

static void test_disconnection() {
  std::cout << "[disconnection]\n";
  Pty doomed;
  SerialPort port(doomed.slave().c_str(), 9600);
  doomed.hangup();
  std::string line;
  const int ret = port.read_line(line);
  std::cout << "  read_line after a hangup: " << ret << " ("
            << port.last_error().message() << ")\n";
  CHECK_EQ(ret, -1);
  CHECK(static_cast<bool>(port.last_error()));
}

static void test_exclusive_access(const std::string &dev) {
  std::cout << "[exclusive access]\n";
  {
    SerialPort first(dev.c_str(), 9600);
    try {
      SerialPort second(dev.c_str(), 9600);
      std::cout << "  a second open is allowed on this system\n";
    } catch (const std::system_error &e) {
      std::cout << "  a second open is refused: " << e.what() << "\n";
    }
  }
  // Exclusive mode must not outlive the port: reopening has to work once the
  // previous owner is gone, whether or not the system enforces exclusivity.
  {
    SerialPort again(dev.c_str(), 9600);
    CHECK(again.is_open());
  }
  {
    SerialPort shared(dev, SerialPort::Config{.exclusive = false});
    CHECK(shared.is_open());
  }
}

#endif /* !_WIN32 */

int main() {
  std::cout << std::unitbuf; // never lose output if a check throws

  test_validation();
  test_missing_port();
  test_port_list();

#if !defined(_WIN32)
  Pty pty;
  const std::string dev = pty.slave();
  std::cout << "pty slave: " << dev << "\n";

  test_line_reading(pty, dev);
  test_settings_restored(dev);
  test_timeouts(pty, dev);
  test_reads(pty, dev);
  test_writes(pty, dev);
  test_c_buffer_lines(pty, dev);
  test_open_errors(dev);
  test_move_and_close(pty, dev);
  test_reconfiguration(pty, dev);
  test_frame_formats(dev);
  test_disconnection();
  test_exclusive_access(dev);
#else
  std::cout << "(the checks that need a port are skipped: no pty on Windows)\n";
#endif

  std::cout << (Failures ? "FAILED " : "PASSED ") << (Checks - Failures) << "/"
            << Checks << " checks\n";
  return Failures ? 1 : 0;
}
