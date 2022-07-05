#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "check.h"

int connect_host(const char *hostname, const char *service);
const char *sockaddr_to_string(const struct sockaddr *sa, socklen_t salen);
int write_all(int fd, char *buffer, size_t len, size_t *written);

int main(int argc, char *const argv[]) {
    if (argc != 3) {
        printf("Usage: %s HOST PORT\n", program_invocation_short_name);
        return EXIT_FAILURE;
    }

    const char *hostname = argv[1];
    const char *service = argv[2];
    char buffer[8192];
    ssize_t n;
    int sock;
    int sleep_time = 1;
    size_t remaining_data_in_buffer = 0;

    while (1) {
    reconnect:
        while ((sock = connect_host(hostname, service)) < 0) {
            fprintf(stderr, "Cannot connect to %s:%s: %s\n", hostname, service, strerror(errno));
            sleep(sleep_time);
            if (sleep_time < 1024) {
                sleep_time *= 2;
            }
        }
        sleep_time = 1;

        if (remaining_data_in_buffer > 0) {
            assert(remaining_data_in_buffer < sizeof(buffer));
            size_t r;
            if (write_all(sock, &buffer[remaining_data_in_buffer],
                          sizeof(buffer) - remaining_data_in_buffer, &r) < 0) {
                remaining_data_in_buffer += r;
                goto reconnect;
            }
        }

        struct pollfd pfds[] = {
            {.fd = sock, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0},
        };

        while (1) {
            CHK_NEG(poll(pfds, 2, -1) < 0);

            if (pfds[0].revents & POLLERR) {
                fprintf(stderr, "Error, with socket. Reconnecting\n");
                goto reconnect;
            }
            if (pfds[0].revents & POLLIN) {
                n = read(sock, buffer, sizeof(buffer));
                if (n < 0) {
                    fprintf(stderr, "Error, with socket (%s). Reconnecting\n", strerror(errno));
                    goto reconnect;
                } else if (n == 0) {
                    fprintf(stderr, "Socket is closed. Reconnecting\n");
                    goto reconnect;
                }
                CHK_NEG(write_all(STDOUT_FILENO, buffer, (size_t)n, NULL));
            }

            if (pfds[1].revents & POLLERR) {
                fprintf(stderr, "Error with STDIN. Quitting\n");
                return EXIT_FAILURE;
            }
            if (pfds[1].revents & POLLIN) {
                CHK_NEG(n = read(STDIN_FILENO, buffer, sizeof(buffer)));
                if (n == 0) {
                    fprintf(stderr, "STDIN is closed, quitting\n");
                    return EXIT_SUCCESS;
                }
                if (write_all(sock, buffer, (size_t)n, &remaining_data_in_buffer) < 0) {
                    fprintf(stderr, "Error while writing into socket (%s), reconnecting.\n",
                            strerror(errno));
                    goto reconnect;
                }
                remaining_data_in_buffer = 0;
            }
        }
    }
    return EXIT_SUCCESS;
}

int connect_host(const char *hostname, const char *service) {
    struct addrinfo *results = NULL;
    struct addrinfo hints;
    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    CHK(getaddrinfo(hostname, service, &hints, &results), != 0);

    for (struct addrinfo *ai = results; ai != NULL; ai = ai->ai_next) {
        CHK_NEG(sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == 0) {
            fprintf(stderr, "Connected to %s\n", sockaddr_to_string(ai->ai_addr, ai->ai_addrlen));
            return sock;
        }
        close(sock);
        sock = -1;
    }

    return -1;
}

const char *sockaddr_to_string(const struct sockaddr *sa, socklen_t salen) {
    static char buffer[256];

    switch (sa->sa_family) {
    case AF_INET: {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        assert(salen >= sizeof(struct sockaddr_in));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, (const void *)&sin->sin_addr, ip, sizeof(ip));
        snprintf(buffer, sizeof(buffer), "%s:%hu", ip, htons(sin->sin_port));
        break;
    }
    case AF_INET6: {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        assert(salen >= sizeof(struct sockaddr_in6));
        char ip6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, (const void *)&sin6->sin6_addr, ip6, sizeof(ip6));
        snprintf(buffer, sizeof(buffer), "[%s]:%hu", ip6, htons(sin6->sin6_port));
        break;
    }
    default:
        fprintf(stderr, "Unsupported socket family: %hu\n", sa->sa_family);
        exit(EXIT_FAILURE);
    }
    return buffer;
}

int write_all(int fd, char *buffer, size_t len, size_t *written) {
    ssize_t n;
    size_t total = 0;
    while (len > 0) {
        n = write(fd, buffer, len);
        if (n == 0) {
            break;
        } else if (n < 0) {
            if (written) {
                *written = total;
            }
            return n;
        } else {
            total += n;
            buffer += n;
            len -= n;
        }
    }
    if (written) {
        *written = total;
    }
    return 0;
}
