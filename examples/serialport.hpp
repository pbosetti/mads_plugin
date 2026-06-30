/*
MIT License

Copyright (c) 2018, Michael Spieler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _SERIALPORT_H
#define _SERIALPORT_H

#include <string>
#include <termios.h>

/** Class to interface serialport under Linux and macOS */
class SerialPort {
public:
  /** SerialPort
   *  @param[in] port Path to the serialport.
   *  @param[in] baudRate Serial baud rate configuration.
   *  @param[in] stopBits Number of stop bits (1 or 2).
   */
  SerialPort(const char *port, unsigned baudRate = 57600, unsigned stopBits = 1,
             bool canonical_mode = false);
  ~SerialPort();

  /** Write a buffer.
   * @param[in] buf Output buffer.
   * @param[in] nBytes Number of bytes to be written.
   * @return Number of bytes read, negative on error.
   */
  int write(const char *buf, size_t nBytes);

  /** Wrire a std::string.
   * @param[in] string String to be written.
   * @return Number of bytes written, negative on error.
   */
  int write(const std::string &string);

  /** Read nBytes into a buffer.
   * @param[out] buf Buffer read.
   * @return Number of bytes read, negative on error.
   */
  int read(char *buf, size_t nBytes);

  /** Read a line terminated by a newline character (CR or LF).
   * @param[out] line Line read, NULL-terminated.
   * @param[in] nmax Capacity of line buffer inclusive NULL-termination.
   * @return Number of bytes read (without NULL-termination), negative on error.
   */
  int readLine(char *line, size_t nmax);

  /** Read a line terminated by a newline character (CR or LF).
   * @param[out] line Line read as a std::string.
   * @return Number of bytes read, negative on error.
   */
  int readLine(std::string &line);

protected:
  int mFileDesc; /**< File descriptor */

private:
  struct termios mOriginalTTYAttrs; /**< Original termios options */
};

#endif /* _SERIALPORT_H */
