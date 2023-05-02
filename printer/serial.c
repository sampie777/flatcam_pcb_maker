//
// Created by samuel on 1-5-23.
//

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "serial.h"

void serial_open(SerialDevice *device) {
    device->opened = false;
    device->has_error = false;
    device->port = open(device->port_name, O_RDWR);
    if (device->port < 0) {
        device->has_error = true;
        sprintf(device->error, "[Serial] Error %i from open: %s", errno, strerror(errno));
        return;
    }

    struct termios tty;
    if (tcgetattr(device->port, &tty) != 0) {
        device->has_error = true;
        sprintf(device->error, "[Serial] Error %i from tcgetattr: %s", errno, strerror(errno));
        return;
    }

    // disable generation and detection of the parity bit
    tty.c_cflag &= ~PARENB;
    // disable use two stop bits
    tty.c_cflag &= ~CSTOPB;
    // use 8 bits per byte
    tty.c_cflag |= CS8;
    // disable hardware RTS/CTS flow control
    tty.c_cflag &= ~CRTSCTS;
    // disables modem-specific signal lines and allow reading data
    tty.c_cflag |= CLOCAL | CREAD;

    // disable canonical mode
    tty.c_lflag &= ~ICANON;
    // These 3 probably don't do anything as canonical mode is disabled
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    // disable signal chars
    tty.c_lflag &= ~ISIG;

    // disable sofware flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    // disable special handling of received bytes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // disable any special handling of output chars/bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
    tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
    tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)
#endif

    tty.c_cc[VTIME] = 1;    // Wait for up to x deciseconds (10 decisecond = 1 second), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save settings
    if (tcsetattr(device->port, TCSANOW, &tty) != 0) {
        device->has_error = true;
        sprintf(device->error, "[Serial] Error %i from tcsetattr: %s", errno, strerror(errno));
        return;
    }

    device->opened = true;
}

void serial_close(SerialDevice *device) {
    close(device->port);
    device->opened = false;
}

void serial_read_line(SerialDevice *device, char *buffer, size_t max_length) {
    memset(buffer, '\0', max_length);

    while (strlen(buffer) < max_length - 1) {
        char data[1024];
        memset(data, '\0', sizeof(data));

        ssize_t data_length = read(device->port, &data, sizeof(data));
        if (data_length == 0) continue;
        if (data_length < 0) {
            device->has_error = true;
            sprintf(device->error, "[Serial] Error reading line: %s", strerror(errno));
            break;
        }

        strcat(buffer, data);
        if (data[data_length - 1] == '\n' || data[data_length - 1] == '\r') {
            // Remove line break at the end
            while (buffer[strlen(buffer) - 1] == '\n' || buffer[strlen(buffer) - 1] == '\r') {
                buffer[strlen(buffer) - 1] = '\0';
            }
            break;
        }
    }
}

void serial_read(SerialDevice *device) {
    char data[1024];
    memset(data, '\0', sizeof(data));

    ssize_t data_length = read(device->port, &data, sizeof(data));
    if (data_length == 0) return;
    if (data_length < 0) {
        device->has_error = true;
        sprintf(device->error, "[Serial] Error reading: %s", strerror(errno));
        return;
    }
    data[data_length] = '\0';

    while (data[strlen(data) - 1] == '\n' || data[strlen(data) - 1] == '\r') {
        data[strlen(data) - 1] = '\0';
    }
}

void serial_read_clear(const SerialDevice *device, int iterations) {
    char data[1024];
    for (int i = 0; i < iterations; i++) {
        memset(data, '\0', sizeof(data));
        read(device->port, &data, sizeof(data));
    }
}

void serial_println(SerialDevice *device, const char *data) {
    write(device->port, data, strlen(data));
    write(device->port, "\r", 1);
    strcpy(device->error, data);
}
