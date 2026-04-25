#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define BAUD_RATE B115200

int iflag = 0;
int oflag = 0;
int cflag = CREAD | CS8 | CLOCAL;
int lflag = 0;

#define BUF_SIZE 128
#define TIMEOUT_MSEC 5000

const char *usagestr = "Usage: %s tty_filename\n";
const char *teststr = "hello world";

int set_attrs(int fd, struct termios *t);
int check_attrs(int fd, struct termios *t);
int wait_for_input(int fd);

int main(int argc, char *argv[])
{
        struct termios t;
        
        int n;  // For function call error checking
        int fd; // File descriptor for the UART
        char buf[BUF_SIZE]; // Read buffer

        if (argc != 2) {
                printf(usagestr, argv[0]);
                return EXIT_SUCCESS;
        }

        // Open TTY
        fd = open(argv[1], O_RDWR);
        if (fd == -1) {
                perror("[ERROR] Failed to open requested TTY");
                return EXIT_FAILURE;
        }

        // Apply flags
        n = set_attrs(fd, &t);
        if (n != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }

        // tcsetattr() returns 0 if ANY flags are successfully set, so check
        // for inconsistencies
        n = check_attrs(fd, &t);
        if (n != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }

        // Write the test string
        printf("Sending test string: %s\n", teststr);
        write(fd, teststr, strlen(teststr));

        n = wait_for_input(fd);
        if (n != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }

        // Read and store
        n = read(fd, buf, BUF_SIZE - 1);
        if (n == -1) {
                perror("[ERROR] read() failed");
                close(fd);
                return EXIT_FAILURE;
        }

        buf[n] = '\0'; // Null terminate whatever we just read

        if (n >= BUF_SIZE) {
                printf("[WARNING] Reached the end of the input buffer while "
                                "reading. Data may be truncated\n");
        }

        printf("UART device responds:\n%s\n", buf);

        // Close port and error check
        n = close(fd);
        if (n != 0) {
                perror("[WARNING] close() syscall failed. Check for data loss");
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

int set_attrs(int fd, struct termios *t)
{
        int n;

        n = tcgetattr(fd, t);  // Get attributes, just to ensure the device
                                // is working properly
        if (n == -1) {
                perror("[ERROR] Failed to get TTY attributes");
                return EXIT_FAILURE;
        }

        // Set flags
        t->c_iflag = iflag;
        t->c_oflag = oflag;
        t->c_cflag = cflag;
        t->c_lflag = lflag;
        n = cfsetspeed(t, BAUD_RATE);
        if (n == -1) {
                perror("[ERROR] Failed to set baud rate");
                return EXIT_FAILURE;
        }

        // Send flags to the device
        n = tcsetattr(fd, TCSANOW, t);
        if (n == -1) {
                perror("[ERROR] Failed to set TTY attributes");
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

int check_attrs(int fd, struct termios *t)
{
        struct termios t_chk;
        int n;

        // Get the currently active attributes
        n = tcgetattr(fd, &t_chk);
        if (n == -1) {
                perror("[ERROR] Failed to get TTY attributes");
                return EXIT_FAILURE;
        }

        // Check that flags match before and after setting
        if (t->c_iflag != t_chk.c_iflag) {
                printf("[WARNING] Input flags do not match; tcsetattr() "
                                "likely failed.\n");
        }
        if (t->c_oflag != t_chk.c_oflag) {
                printf("[WARNING] Output flags do not match; tcsetattr() "
                                "likely failed.\n");
        }
        if (t->c_cflag != t_chk.c_cflag) {
                printf("[WARNING] Control flags do not match; tcsetattr() "
                                "likely failed.\n");
        }
        if (t->c_lflag != t_chk.c_lflag) {
                printf("[WARNING] Local flags do not match; tcsetattr() "
                                "likely failed.\n");
        }

        return EXIT_SUCCESS;
}

int wait_for_input(int fd)
{
        int n;
        struct pollfd pfd;

        pfd.fd = fd;
        pfd.events = POLLIN; // Only trigger when data is available for reading

        // Block until there is data to read, or timeout
        n = poll(&pfd, 1, TIMEOUT_MSEC);
        if (n == -1) {
                perror("[ERROR] poll() system call failed");
                close(fd);
                return EXIT_FAILURE;
        } else if (n == 0) {
                printf("Timed out while waiting for data\n");
                return EXIT_SUCCESS;
        }
        
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("[WARNING] Unexpected TTY event detected during poll()"
                                "\n");
        }

        return EXIT_SUCCESS;
}
