#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "socketHelper.h"

char* getSocketAddress(int sockfd) {
    // source: https://man7.org/linux/man-pages/man3/getifaddrs.3.html
    struct ifaddrs *ifaddr;
    struct sockaddr_in *sa;
    char *ipAddress;
    if (getifaddrs (&ifaddr) < 0) {
        return NULL;
    }
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        // check if IPv4 and not the loopback address
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            // can't use the loopback address
            ipAddress = inet_ntoa(sa->sin_addr);
            if (strcasecmp(ipAddress, "127.0.0.1") == 0) continue;
            printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, ipAddress);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}