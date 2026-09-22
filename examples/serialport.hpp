/*
  ____            _       _ ____            _
 / ___|  ___ _ __(_) __ _| |  _ \ ___  _ __| |_
 \___ \ / _ \ '__| |/ _` | | |_) / _ \| '__| __|
  ___) |  __/ |  | | (_| | |  __/ (_) | |  | |_
 |____/ \___|_|  |_|\__,_|_|_|   \___/|_|   \__|

Serial port interface for Linux, macOS and Windows, C++20.
The class is a thin, RAII wrapper over the native port handle: it never
busy-waits, it buffers input internally, and it restores the original port
settings on close. Line framing, timeouts and buffering are implemented once,
on top of a small platform layer (termios plus poll(2) on POSIX, DCB plus
COMMTIMEOUTS on Windows).
*/

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

/** Class to interface serial ports under Linux, macOS and Windows.
 *
 * The port is opened by the constructor and closed by the destructor. Objects
 * are movable but not copyable, so that a handle is never closed twice. A
 * single port must not be used concurrently from more than one thread.
 *
 * All the I/O methods honour the configured timeout() and report failures via
 * a negative return value; the reason of the failure is available through
 * last_error(), while timed_out() tells whether a short read was caused by the
 * timeout expiring. The setup and control methods (the constructors,
 * set_baud_rate(), the flush family, drain() and the modem line accessors)
 * throw std::system_error or std::invalid_argument instead.
 */
class SerialPort {
public:
  /** Steady clock used for all timeouts. */
  using Clock = std::chrono::steady_clock;
  /** Convenience alias for timeout arguments. */
  using Milliseconds = std::chrono::milliseconds;

#if defined(_WIN32)
  /** Native port handle: a Win32 HANDLE, declared as void * so that this
   * header does not need to include windows.h. */
  using NativeHandle = void *;
  /** Value of native_handle() when the port is closed. */
  static constexpr NativeHandle invalid_handle = nullptr;
#else
  /** Native port handle: a file descriptor. */
  using NativeHandle = int;
  /** Value of native_handle() when the port is closed. */
  static constexpr NativeHandle invalid_handle = -1;
#endif

  /** Parity generation/checking mode. */
  enum class Parity { none, even, odd };

  /** Flow control mode. */
  enum class FlowControl { none, software, hardware };

  /** Timeout value meaning "block until the operation completes". */
  static constexpr Milliseconds no_timeout{-1};

  /** Full port configuration, designed for designated initializers:
   * @code
   * SerialPort port("/dev/ttyUSB0", {.baud_rate = 115200, .timeout = 500ms});
   * @endcode
   */
  struct Config {
    /** Baud rate, e.g. 9600 or 115200. Linux only accepts the rates listed
     * in termios.h, macOS and Windows accept any rate the driver supports. */
    unsigned baud_rate = 57600;
    /** Bits per character (5 to 8). */
    unsigned data_bits = 8;
    /** Number of stop bits (1 or 2). */
    unsigned stop_bits = 1;
    /** Parity mode. */
    Parity parity = Parity::none;
    /** Flow control mode. */
    FlowControl flow_control = FlowControl::none;
    /** Line-oriented input processing (ICANON). POSIX only: read_line()
     * does its own framing, so this rarely needs to be changed. */
    bool canonical_mode = false;
    /** Lower the modem lines when the port is closed (HUPCL). POSIX only. */
    bool hangup_on_close = true;
    /** Refuse any further open of the same port (TIOCEXCL). POSIX only:
     * Windows always opens serial ports exclusively. */
    bool exclusive = true;
    /** Timeout of every read and write, no_timeout to block. */
    Milliseconds timeout = no_timeout;
    /** Size of the internal input buffer, at least 64 bytes. */
    std::size_t buffer_size = 1024;
    /** Longest line accepted by read_line(). */
    std::size_t max_line_length = 65536;
  };

  /** Open a serial port.
   *  @param[in] port Path to the serial port, e.g. "/dev/ttyUSB0".
   *  @param[in] baud_rate Serial baud rate configuration.
   *  @param[in] stop_bits Number of stop bits (1 or 2).
   *  @param[in] canonical_mode Enable canonical (line oriented) input.
   *  @throws std::invalid_argument on an unsupported parameter.
   *  @throws std::system_error if the port cannot be opened or configured.
   */
  explicit SerialPort(std::string_view port, unsigned baud_rate = 57600,
                      unsigned stop_bits = 1, bool canonical_mode = false);

  /** Open a serial port with a full configuration.
   *  @param[in] port Path to the serial port.
   *  @param[in] config Port configuration.
   *  @throws std::invalid_argument on an unsupported parameter.
   *  @throws std::system_error if the port cannot be opened or configured.
   */
  SerialPort(std::string_view port, const Config &config);

  /** Restore the original port settings and close the descriptor. */
  ~SerialPort();

  SerialPort(const SerialPort &) = delete;
  SerialPort &operator=(const SerialPort &) = delete;
  SerialPort(SerialPort &&other) noexcept;
  SerialPort &operator=(SerialPort &&other) noexcept;

  /* WRITING ****************************************************************/

  /** Write a buffer, retrying until everything is written or the timeout
   * expires.
   * @param[in] buf Output buffer.
   * @param[in] n_bytes Number of bytes to be written.
   * @return Number of bytes written, negative on error.
   */
  int write(const char *buf, std::size_t n_bytes);

  /** Write a string (also accepts a std::string or a C string).
   * @param[in] data Text to be written.
   * @return Number of bytes written, negative on error.
   */
  int write(std::string_view data);

  /** Write raw bytes.
   * @param[in] data Bytes to be written.
   * @return Number of bytes written, negative on error.
   */
  int write(std::span<const std::byte> data);

  /** Write a string followed by a line terminator.
   * @param[in] data Text to be written.
   * @param[in] eol Line terminator appended to @p data.
   * @return Number of bytes written, negative on error.
   */
  int write_line(std::string_view data, std::string_view eol = "\n");

  /* READING ****************************************************************/

  /** Read exactly n_bytes into a buffer, unless the timeout expires first.
   * @param[out] buf Buffer to be filled.
   * @param[in] n_bytes Number of bytes to be read.
   * @return Number of bytes read (less than @p n_bytes on timeout), negative
   *         on error.
   */
  int read(char *buf, std::size_t n_bytes);

  /** Read exactly buf.size() bytes, unless the timeout expires first.
   * @param[out] buf Buffer to be filled.
   * @return Number of bytes read, negative on error.
   */
  int read(std::span<char> buf);

  /** Read exactly buf.size() bytes, unless the timeout expires first.
   * @param[out] buf Buffer to be filled.
   * @return Number of bytes read, negative on error.
   */
  int read(std::span<std::byte> buf);

  /** Read whatever is available, waiting for at least one byte.
   * @param[out] buf Buffer to be filled.
   * @param[in] n_max Capacity of @p buf.
   * @return Number of bytes read (0 on timeout), negative on error.
   */
  int read_some(char *buf, std::size_t n_max);

  /** Read a line terminated by a newline character (CR, LF or CRLF).
   * The terminator is consumed but not stored. A line longer than
   * @p n_max - 1 is truncated (the rest is discarded, so that the framing of
   * the following lines is preserved) and last_error() is set to EMSGSIZE.
   * @param[out] line Line read, NULL-terminated.
   * @param[in] n_max Capacity of line buffer inclusive NULL-termination.
   * @return Number of bytes read (without NULL-termination), negative on
   *         error.
   */
  int read_line(char *line, std::size_t n_max);

  /** Read a line terminated by a newline character (CR, LF or CRLF).
   * @p line is cleared first; the terminator is consumed but not stored.
   * @param[out] line Line read as a std::string.
   * @return Number of bytes read, negative on error.
   */
  int read_line(std::string &line);

  /** Read a line terminated by a newline character (CR, LF or CRLF).
   * @return The line read, or std::nullopt on error or timeout.
   */
  [[nodiscard]] std::optional<std::string> read_line();

  /** Alias of read_line(char *, size_t), kept for backward compatibility. */
  int readLine(char *line, std::size_t n_max) { return read_line(line, n_max); }

  /** Alias of read_line(std::string &), kept for backward compatibility. */
  int readLine(std::string &line) { return read_line(line); }

  /* PORT STATE *************************************************************/

  /** @return true if the port is open. */
  [[nodiscard]] bool is_open() const noexcept {
    return _handle != invalid_handle;
  }

  /** Drain, restore the original settings and close the port. Idempotent. */
  void close() noexcept;

  /** @return The native handle, invalid_handle if the port is closed. */
  [[nodiscard]] NativeHandle native_handle() const noexcept { return _handle; }

#if !defined(_WIN32)
  /** @return The underlying file descriptor, -1 if the port is closed.
   * Available on POSIX systems only, use native_handle() for portable code. */
  [[nodiscard]] int fd() const noexcept { return _handle; }
#endif

  /** @return The path of the port. */
  [[nodiscard]] const std::string &port() const noexcept { return _port; }

  /** @return The current configuration. */
  [[nodiscard]] const Config &config() const noexcept { return _config; }

  /** @return The current read/write timeout. */
  [[nodiscard]] Milliseconds timeout() const noexcept {
    return _config.timeout;
  }

  /** Set the read/write timeout (no_timeout to block indefinitely).
   * @param[in] value New timeout.
   */
  void set_timeout(Milliseconds value) noexcept { _config.timeout = value; }

  /** @return The current baud rate. */
  [[nodiscard]] unsigned baud_rate() const noexcept {
    return _config.baud_rate;
  }

  /** Change the baud rate on an open port.
   * @param[in] value New baud rate.
   * @throws std::invalid_argument on an unsupported baud rate.
   * @throws std::system_error if the port cannot be reconfigured.
   */
  void set_baud_rate(unsigned value);

  /** @return true if the last I/O operation was cut short by the timeout. */
  [[nodiscard]] bool timed_out() const noexcept { return _timed_out; }

  /** @return The error reported by the last failed operation. */
  [[nodiscard]] std::error_code last_error() const noexcept {
    return _last_error;
  }

  /** @return Number of bytes available for reading without blocking. */
  [[nodiscard]] std::size_t available() const;

  /** Discard all the data received but not read yet. */
  void flush_input();

  /** Discard all the data written but not transmitted yet. */
  void flush_output();

  /** Discard both pending input and pending output. */
  void flush();

  /** Block until all the pending output has been transmitted. */
  void drain();

  /* MODEM LINES ************************************************************/

  /** Set or clear the DTR line (toggling it resets most Arduino boards).
   * @param[in] on New state of the line.
   */
  void set_dtr(bool on);

  /** Set or clear the RTS line.
   * @param[in] on New state of the line.
   */
  void set_rts(bool on);

  /** @return The state of the CTS input line. */
  [[nodiscard]] bool cts() const;

  /** @return The state of the DSR input line. */
  [[nodiscard]] bool dsr() const;

  /** @return The state of the DCD (carrier detect) input line. */
  [[nodiscard]] bool dcd() const;

  /** Transmit a stream of zero bits.
   * @param[in] duration On POSIX an implementation defined duration, on
   *            Windows a number of milliseconds; 0 means about 0.25 s on both.
   */
  void send_break(int duration = 0);

  /** List the serial ports that look usable on this machine.
   * @return Paths of the candidate ports, sorted, possibly empty.
   */
  [[nodiscard]] static std::vector<std::string> available_ports();

private:
  /** Absolute instant after which an operation gives up, or nullopt if it
   * must block indefinitely. */
  using Deadline = std::optional<Clock::time_point>;

  /** One of the modem control lines. */
  enum class ModemLine { dtr, rts, cts, dsr, dcd };

  /** Platform specific state, kept out of the header. */
  struct State;

  /* Platform layer: one implementation per operating system. Each of these
     either throws (setup and control) or returns the number of bytes moved,
     0 on timeout and -1 on error after calling fail_native(). */
  void open_native(const Config &config);
  void close_native() noexcept;
  void apply_config(const Config &config);
  int read_native(char *buf, std::size_t n_max, const Deadline &deadline);
  int write_native(const char *buf, std::size_t n_bytes,
                   const Deadline &deadline);
  [[nodiscard]] std::size_t available_native() const;
  void flush_native(bool input, bool output);
  void drain_native();
  void modem_set_native(ModemLine line, bool on);
  [[nodiscard]] bool modem_get_native(ModemLine line) const;
  void break_native(int duration);

  /* Shared layer. */
  int fill_buffer(const Deadline &deadline);
  std::size_t take_buffered(char *dst, std::size_t n_max) noexcept;
  int fail(std::errc code) const noexcept;
  int fail_native(int error_number) const noexcept;
  void begin_operation() noexcept;

  [[nodiscard]] std::size_t buffered() const noexcept {
    return _rx_tail - _rx_head;
  }

  std::string _port;
  Config _config{};
  std::unique_ptr<State> _state;
  std::vector<char> _rx_buf;
  std::string _line_buf;
  std::size_t _rx_head = 0;
  std::size_t _rx_tail = 0;
  NativeHandle _handle = invalid_handle;
  bool _pending_lf = false;
  bool _timed_out = false;
  mutable std::error_code _last_error{};
};

#endif /* SERIALPORT_HPP */
