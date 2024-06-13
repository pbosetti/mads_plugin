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

#include "serialport.hpp"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

int connect(const char *port, unsigned baudRate, unsigned stopBits,
            bool canonical_mode, struct termios *originalTTYAttrs) {
  struct termios options;
  int fileDesc = -1;

  fileDesc = open(port, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if (fileDesc == -1) {
    goto error;
  }

  // Exclusive file open
  if (ioctl(fileDesc, TIOCEXCL) == -1) {
    goto error;
  }

  // allow communications to block
  if (fcntl(fileDesc, F_SETFL, 0) == -1) {
    goto error;
  }

  // Save current TTY settings
  if (tcgetattr(fileDesc, originalTTYAttrs) == -1) {
    goto error;
  }

  // Setup the terminal for ordinary I/O
  // e.g. as an ordinary modem or serial port printer.
  //
  // Default config: hangup on close.
  // if the HUPCL bit set, closing the port will hang up
  // the modem. (i.e. will lower the modem control lines)
  //
  // Default config:
  // 8 bits, no parity
  options = *originalTTYAttrs;
  options.c_cflag = CREAD | CS8 | HUPCL | CLOCAL;

  // Send 2 stop bits, default: 1
  if (stopBits == 2) {
    options.c_cflag |= CSTOPB;
  }

  speed_t speed;
  switch (baudRate) {
  case 230400:
    speed = B230400;
    break;

  case 115200:
    speed = B115200;
    break;

  case 57600:
    speed = B57600;
    break;

  case 38400:
    speed = B38400;
    break;

  case 19200:
    speed = B19200;
    break;

  case 9600:
    speed = B9600;
    break;

  case 4800:
    speed = B4800;
    break;

  case 2400:
    speed = B2400;
    break;

  case 1800:
    speed = B1800;
    break;

  case 1200:
    speed = B1200;
    break;

  case 600:
    speed = B600;
    break;

  case 300:
    speed = B300;
    break;

  default:
    goto error;
  }
  cfsetspeed(&options, speed);

  // disable canonical processing
  if (canonical_mode) {
    options.c_lflag |= ICANON;
  } else {
    options.c_lflag &= ~ICANON;
  }

  // disable echoing of input
  options.c_lflag &= ~ECHO;

  // disable all special control characters
  options.c_cc[VEOF] = _POSIX_VDISABLE;
  options.c_cc[VEOL] = _POSIX_VDISABLE;
  options.c_cc[VEOL2] = _POSIX_VDISABLE;
  options.c_cc[VERASE] = _POSIX_VDISABLE;
  options.c_cc[VWERASE] = _POSIX_VDISABLE;
  options.c_cc[VKILL] = _POSIX_VDISABLE;
  options.c_cc[VREPRINT] = _POSIX_VDISABLE;
  options.c_cc[VINTR] = _POSIX_VDISABLE;
  options.c_cc[VQUIT] = _POSIX_VDISABLE;
  options.c_cc[VSUSP] = _POSIX_VDISABLE;
  options.c_cc[VSTART] = _POSIX_VDISABLE;
  options.c_cc[VSTOP] = _POSIX_VDISABLE;
  options.c_cc[VLNEXT] = _POSIX_VDISABLE;
  options.c_cc[VDISCARD] = _POSIX_VDISABLE;
#ifndef __linux__
  options.c_cc[VSTATUS] = _POSIX_VDISABLE;
  options.c_cc[VDSUSP] = _POSIX_VDISABLE;
#endif
  // more info on VMIN and VTIME here:
  // http://unixwiz.net/techtips/termios-vmin-vtime.html
  options.c_cc[VMIN] = 0;  // min bytes in input queue
  options.c_cc[VTIME] = 1; // timeout in 0.1 sec

  // write options
  if (tcsetattr(fileDesc, TCSANOW, &options) == -1) {
    goto error;
  }

  // flush any unwritten, unread data
  if (tcflush(fileDesc, TCIOFLUSH) == -1) {
    goto error;
  }

  return fileDesc;

error:
  if (fileDesc != -1) {
    close(fileDesc);
  }

  return -1;
}

SerialPort::SerialPort(const char *port, unsigned baudRate, unsigned stopBits,
                       bool canonical_mode) {
  mFileDesc =
      connect(port, baudRate, stopBits, canonical_mode, &mOriginalTTYAttrs);
  if (mFileDesc == -1) {
    throw std::runtime_error(std::string(strerror(errno)));
  }
}

SerialPort::~SerialPort() {
  if (mFileDesc == -1) {
    return;
  }

  tcflush(mFileDesc, TCIOFLUSH);
  close(mFileDesc);
}

int SerialPort::write(const char *buf, size_t nBytes) {
  if (nBytes == 0) {
    return 0;
  }
  return ::write(mFileDesc, buf, nBytes);
}

int SerialPort::write(const std::string &string) {
  size_t len = string.length();
  if (len == 0) {
    return 0;
  }
  return ::write(mFileDesc, string.c_str(), len);
}

int SerialPort::read(char *buf, size_t nBytes) {
  int n = 0;
  while (n < nBytes) {
    int ret = ::read(mFileDesc, &buf[n], nBytes - n);
    if (ret < 0) {
      return ret;
    }
  }
  return n;
}

int SerialPort::readLine(char *line, size_t nmax) {
  if (nmax == 0) {
    return 0;
  }

  int n = 0;
  while (n < nmax - 1) {
    char c;
    int ret = ::read(mFileDesc, &c, 1);
    if (ret > 0) {
      if (c == '\n' || c == '\r') {
        break;
      }
      line[n] = c;
      n++;
    } else if (ret < 0) {
      return ret;
    }
  }
  line[n] = '\0';
  return n;
}

int SerialPort::readLine(std::string &line) {
  int n = 0;
  while (true) {
    char c;
    int ret = ::read(mFileDesc, &c, 1);
    if (ret > 0) {
      if (c == '\n' || c == '\r') {
        break;
      }
      line.push_back(c);
      n++;
    } else if (ret < 0) {
      return ret;
    }
  }
  return n;
}
