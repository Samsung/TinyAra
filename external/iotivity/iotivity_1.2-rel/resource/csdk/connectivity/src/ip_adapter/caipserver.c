/* ****************************************************************
 *
 * Copyright 2014 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#ifndef __APPLE_USE_RFC_3542
#define __APPLE_USE_RFC_3542 // for PKTINFO
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for in6_pktinfo
#endif

#ifdef __TIZENRT__
#include <tinyara/config.h>
#include <sys/uio.h>
#endif

#include "iotivity_config.h"
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#endif

#if defined(_WIN32)
#include <assert.h>
#include <winsock2.h>
#include <ws2def.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#endif

#include <stdio.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif //!defined(_MSC_VER)
#include <sys/types.h>
#include <fcntl.h>
#if !defined(_WIN32)
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#endif
#include <errno.h>
#ifdef __linux__
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#ifdef __TIZENRT__
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#endif
#include <coap/pdu.h>
#include "caipinterface.h"
#include "caipnwmonitor.h"
#include "caadapterutils.h"
#if defined(__WITH_DTLS__) || defined(__WITH_TLS__)
#include "ca_adapter_net_ssl.h"
#endif
#include "octhread.h"
#include "oic_malloc.h"
#include "oic_string.h"

#define USE_IP_MREQN
#if defined(_WIN32) || defined (__TIZENRT__)
#undef USE_IP_MREQN
#endif

/*
 * Logging tag for module name
 */
//#define TAG "OIC_CA_IP_SERVER"
#define TAG IP_SERVER_TAG

#ifdef __TIZENRT__
#ifdef CONFIG_NET_LWIP
#define SOCK_CLOEXEC 0
#else
#define SOCK_CLOEXEC 1
#endif
#endif
#define SELECT_TIMEOUT 1     // select() seconds (and termination latency)

#define IPv4_MULTICAST     "224.0.1.187"
static struct in_addr IPv4MulticastAddress = { 0 };

#define IPv6_DOMAINS       16
#define MOBILE_INTERFACES  2
#define IPv6_MULTICAST_INT "ff01::158"
static struct in6_addr IPv6MulticastAddressInt;
#define IPv6_MULTICAST_LNK "ff02::158"
static struct in6_addr IPv6MulticastAddressLnk;
#define IPv6_MULTICAST_RLM "ff03::158"
static struct in6_addr IPv6MulticastAddressRlm;
#define IPv6_MULTICAST_ADM "ff04::158"
static struct in6_addr IPv6MulticastAddressAdm;
#define IPv6_MULTICAST_SIT "ff05::158"
static struct in6_addr IPv6MulticastAddressSit;
#define IPv6_MULTICAST_ORG "ff08::158"
static struct in6_addr IPv6MulticastAddressOrg;
#define IPv6_MULTICAST_GLB "ff0e::158"
static struct in6_addr IPv6MulticastAddressGlb;

static char *ipv6mcnames[IPv6_DOMAINS] = {
    NULL,
    IPv6_MULTICAST_INT,
    IPv6_MULTICAST_LNK,
    IPv6_MULTICAST_RLM,
    IPv6_MULTICAST_ADM,
    IPv6_MULTICAST_SIT,
    NULL,
    NULL,
    IPv6_MULTICAST_ORG,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    IPv6_MULTICAST_GLB,
    NULL
};

// Samsung Mobile
static char *mobileinferfaces[MOBILE_INTERFACES] = {
    "rmnet", "pdp"
};

#ifdef __TIZENRT__
struct in6_pktinfo {
        struct in6_addr ipi6_addr;
        int             ipi6_ifindex;
};

struct in_pktinfo
{
  unsigned int   ipi_ifindex;  /* Interface index */
  struct in_addr ipi_spec_dst; /* Local address */
  struct in_addr ipi_addr;     /* Header Destination
                                    address */
};


#define RTMGRP_LINK 1
#define IP_PKTINFO         8
#define IPV6_PKTINFO            50
#define IPV6_MULTICAST_IF 9
#define IPV6_V6ONLY 27
#define IPV6_RECVPKTINFO       50
#define IPV6_JOIN_GROUP 12
#endif

/**
 * By default, IP multicast datagrams are sent with a time-to-live (TTL) of 1.
 * An application can choose an initial TTL.
 */
static size_t multicastTTL = 1;

#if defined (_WIN32)
#define IFF_UP_RUNNING_FLAGS  (IFF_UP)

    char* caips_get_error(){
        static char buffer[32];
        snprintf(buffer, 32, "%i", WSAGetLastError());
        return buffer;
    }
#define CAIPS_GET_ERROR \
    caips_get_error()
#else
#define IFF_UP_RUNNING_FLAGS  (IFF_UP|IFF_RUNNING)

#define CAIPS_GET_ERROR \
    strerror(errno)
#endif
static CAIPErrorHandleCallback g_ipErrorHandler = NULL;

static CAIPPacketReceivedCallback g_packetReceivedCallback = NULL;

static void CAFindReadyMessage();
#if !defined(WSA_WAIT_EVENT_0)
static void CASelectReturned(fd_set *readFds, int ret);
#else
static void CAEventReturned(CASocketFd_t socket);
#endif

static CAResult_t CAReceiveMessage(CASocketFd_t fd, CATransportFlags_t flags);

static void CAReceiveHandler(void *data)
{
    (void)data;
    OIC_LOG(DEBUG, TAG, "IN - CAReceiveHandler");

    while (!caglobals.ip.terminate)
    {
        CAFindReadyMessage();
    }
#ifndef __TIZENRT__
    if (caglobals.ip.shutdownFds[0] != OC_INVALID_SOCKET)
    {
        close(caglobals.ip.shutdownFds[0]);
        caglobals.ip.shutdownFds[0] = OC_INVALID_SOCKET;
    }
#endif
    OIC_LOG(DEBUG, TAG, "OUT - CAReceiveHandler");
}

#if !defined(WSA_WAIT_EVENT_0)

#define CLOSE_SOCKET(TYPE) \
    if (caglobals.ip.TYPE.fd != OC_INVALID_SOCKET) \
    { \
        close(caglobals.ip.TYPE.fd); \
        caglobals.ip.TYPE.fd = OC_INVALID_SOCKET; \
    }

#define SET(TYPE, FDS) \
    if (caglobals.ip.TYPE.fd != OC_INVALID_SOCKET) \
    { \
        FD_SET(caglobals.ip.TYPE.fd, FDS); \
    }

#define ISSET(TYPE, FDS, FLAGS) \
    if (caglobals.ip.TYPE.fd != OC_INVALID_SOCKET && FD_ISSET(caglobals.ip.TYPE.fd, FDS)) \
    { \
        fd = caglobals.ip.TYPE.fd; \
        flags = FLAGS; \
    }


static void CAFindReadyMessage()
{
    fd_set readFds;
    struct timeval timeout;

    timeout.tv_sec = caglobals.ip.selectTimeout;
    timeout.tv_usec = 0;
    struct timeval *tv = caglobals.ip.selectTimeout == -1 ? NULL : &timeout;

    FD_ZERO(&readFds);
    SET(u6,  &readFds)
    SET(u6s, &readFds)
    SET(u4,  &readFds)
    SET(u4s, &readFds)
    SET(m6,  &readFds)
    SET(m6s, &readFds)
    SET(m4,  &readFds)
    SET(m4s, &readFds)
#ifndef __TIZENRT__
    if (caglobals.ip.shutdownFds[0] != -1)
    {
        FD_SET(caglobals.ip.shutdownFds[0], &readFds);
    }
#endif
    if (caglobals.ip.netlinkFd != OC_INVALID_SOCKET)
    {
        FD_SET(caglobals.ip.netlinkFd, &readFds);
    }

    int ret = select(caglobals.ip.maxfd + 1, &readFds, NULL, NULL, tv);

    if (caglobals.ip.terminate)
    {
        OIC_LOG_V(INFO, TAG, "Packet receiver Stop request received.");
        return;
    }

    if (0 < ret)
    {
        CASelectReturned(&readFds, ret);
    }
    else if (0 > ret)
    {
        OIC_LOG_V(FATAL, TAG, "select error %s", CAIPS_GET_ERROR);
    }
}

static void CASelectReturned(fd_set *readFds, int ret)
{
    (void)ret;
    CASocketFd_t fd = OC_INVALID_SOCKET;
    CATransportFlags_t flags = CA_DEFAULT_FLAGS;

    while (!caglobals.ip.terminate)
    {
        ISSET(u6,  readFds, CA_IPV6)
        else ISSET(u6s, readFds, CA_IPV6 | CA_SECURE)
        else ISSET(u4,  readFds, CA_IPV4)
        else ISSET(u4s, readFds, CA_IPV4 | CA_SECURE)
        else ISSET(m6,  readFds, CA_MULTICAST | CA_IPV6)
        else ISSET(m6s, readFds, CA_MULTICAST | CA_IPV6 | CA_SECURE)
        else ISSET(m4,  readFds, CA_MULTICAST | CA_IPV4)
        else ISSET(m4s, readFds, CA_MULTICAST | CA_IPV4 | CA_SECURE)
        else if ((caglobals.ip.netlinkFd != OC_INVALID_SOCKET) && FD_ISSET(caglobals.ip.netlinkFd, readFds))
        {
            u_arraylist_t *iflist = CAFindInterfaceChange();
            if (iflist)
            {
                uint32_t listLength = u_arraylist_length(iflist);
                for (uint32_t i = 0; i < listLength; i++)
                {
                    CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);
                    if (ifitem)
                    {
                        CAProcessNewInterface(ifitem);
                    }
                }
                u_arraylist_destroy(iflist);
            }
            break;
        }
#ifndef __TIZENRT__
        else if (FD_ISSET(caglobals.ip.shutdownFds[0], readFds))
        {
            char buf[10] = {0};
            ssize_t len = read(caglobals.ip.shutdownFds[0], buf, sizeof (buf));
            if (-1 == len)
            {
                continue;
            }
            break;
        }
#endif
        else
        {
            break;
        }
        (void)CAReceiveMessage(fd, flags);
        FD_CLR(fd, readFds);
    }
}

#else // if defined(WSA_WAIT_EVENT_0)

#define CLOSE_SOCKET(TYPE) \
    if (caglobals.ip.TYPE.fd != OC_INVALID_SOCKET) \
    { \
        closesocket(caglobals.ip.TYPE.fd); \
        caglobals.ip.TYPE.fd = OC_INVALID_SOCKET; \
    }

#define PUSH_HANDLE(HANDLE, ARRAY, INDEX) \
{ \
    ARRAY[INDEX] = HANDLE; \
    INDEX++; \
}

// Turn handle into WSAEvent and push to ARRAY
#define PUSH_SOCKET(SOCKET, ARRAY, INDEX) \
    if (SOCKET != OC_INVALID_SOCKET) \
    { \
        WSAEVENT NewEvent; \
        NewEvent = WSACreateEvent(); \
        if (WSA_INVALID_EVENT != NewEvent) \
        { \
            if (0 != WSAEventSelect(SOCKET, NewEvent, FD_READ)) \
            { \
                OIC_LOG_V(ERROR, TAG, "WSAEventSelect failed 0x%08x ", WSAGetLastError()); \
                if (!WSACloseEvent(NewEvent)) \
                { \
                    OIC_LOG_V(ERROR, TAG, "WSACloseEvent(NewEvent) failed 0x%08x", WSAGetLastError()); \
                } \
            } \
            else \
            { \
                PUSH_HANDLE(NewEvent, ARRAY, INDEX); \
            } \
        } \
        else \
        { \
            OIC_LOG_V(ERROR, TAG, "WSACreateEvent(NewEvent) failed 0x%08x", WSAGetLastError()); \
        }\
    }

#define INSERT_SOCKET(FD, ARRAY, INDEX) \
    { \
        if (OC_INVALID_SOCKET != FD) \
        { \
            ARRAY[INDEX] = FD; \
        } \
    }


// Inserts the socket into the SOCKET_ARRAY and pushes the socket event into EVENT_ARRAY
#define PUSH_IP_SOCKET(TYPE, EVENT_ARRAY, SOCKET_ARRAY, INDEX) \
    { \
        if (OC_INVALID_SOCKET != caglobals.ip.TYPE.fd) \
        { \
            INSERT_SOCKET(caglobals.ip.TYPE.fd, SOCKET_ARRAY, INDEX); \
            PUSH_SOCKET(caglobals.ip.TYPE.fd, EVENT_ARRAY, INDEX); \
        } \
    }

#define IS_MATCHING_IP_SOCKET(TYPE, SOCKET, FLAGS) \
    if ((caglobals.ip.TYPE.fd != OC_INVALID_SOCKET) && (caglobals.ip.TYPE.fd == SOCKET)) \
    { \
        fd = caglobals.ip.TYPE.fd; \
        flags = FLAGS; \
    }

#define EVENT_ARRAY_SIZE  10

static void CAFindReadyMessage()
{
    CASocketFd_t socketArray[EVENT_ARRAY_SIZE];
    HANDLE eventArray[EVENT_ARRAY_SIZE];
    int arraySize = 0;
    int eventIndex;

    // socketArray and eventArray should have same number of elements
    OC_STATIC_ASSERT(_countof(socketArray) == _countof(eventArray), "Arrays should have same number of elements");

    PUSH_IP_SOCKET(u6,  eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(u6s, eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(u4,  eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(u4s, eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(m6,  eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(m6s, eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(m4,  eventArray, socketArray, arraySize);
    PUSH_IP_SOCKET(m4s, eventArray, socketArray, arraySize);

    if (WSA_INVALID_EVENT != caglobals.ip.shutdownEvent)
    {
        INSERT_SOCKET(OC_INVALID_SOCKET, socketArray, arraySize);
        PUSH_HANDLE(caglobals.ip.shutdownEvent, eventArray, arraySize);
    }

    /** @todo Support netlink events */

    // Should not have overflowed buffer
    assert(arraySize <= (_countof(socketArray)));

    // Timeout is unnecessary on Windows
    assert(-1 == caglobals.ip.selectTimeout);

    while (!caglobals.ip.terminate)
    {
        int ret = WSAWaitForMultipleEvents(arraySize, eventArray, FALSE, WSA_INFINITE, FALSE);

        switch (ret)
        {
            case WSA_WAIT_FAILED:
                OIC_LOG_V(ERROR, TAG, "WSAWaitForMultipleEvents returned WSA_WAIT_FAILED 0x%08x", WSAGetLastError());
                break;
            case WSA_WAIT_IO_COMPLETION:
                OIC_LOG_V(ERROR, TAG, "WSAWaitForMultipleEvents returned WSA_WAIT_IO_COMPLETION 0x%08x", WSAGetLastError());
                break;
            case WSA_WAIT_TIMEOUT:
                OIC_LOG_V(ERROR, TAG, "WSAWaitForMultipleEvents returned WSA_WAIT_TIMEOUT 0x%08x", WSAGetLastError());
                break;
            default:
                eventIndex = ret - WSA_WAIT_EVENT_0;
                if ((eventIndex >= 0) && (eventIndex < arraySize))
                {
                    if (false == WSAResetEvent(eventArray[eventIndex]))
                    {
                        OIC_LOG_V(ERROR, TAG, "WSAResetEvent failed 0x%08x", WSAGetLastError());
                    }

                    // Break out if shutdownEvent is triggered.
                    if ((caglobals.ip.shutdownEvent != WSA_INVALID_EVENT) &&
                        (caglobals.ip.shutdownEvent == eventArray[eventIndex]))
                    {
                        break;
                    }
                    CAEventReturned(socketArray[eventIndex]);
                }
                else
                {
                    OIC_LOG_V(ERROR, TAG, "WSAWaitForMultipleEvents failed 0x%08x", WSAGetLastError());
                }
                break;
        }

    }

    while (arraySize > 0)
    {
        arraySize--;
        if (!WSACloseEvent(eventArray[arraySize]))
        {
            OIC_LOG_V(ERROR, TAG, "WSACloseEvent (Index %i) failed 0x%08x", arraySize, WSAGetLastError());
        }
    }

    if (caglobals.ip.terminate)
    {
        caglobals.ip.shutdownEvent = WSA_INVALID_EVENT;
        WSACleanup();
    }
}

static void CAEventReturned(CASocketFd_t socket)
{
    CASocketFd_t fd = OC_INVALID_SOCKET;
    CATransportFlags_t flags = CA_DEFAULT_FLAGS;

    while (!caglobals.ip.terminate)
    {
        IS_MATCHING_IP_SOCKET(u6,  socket, CA_IPV6)
        else IS_MATCHING_IP_SOCKET(u6s, socket, CA_IPV6 | CA_SECURE)
        else IS_MATCHING_IP_SOCKET(u4,  socket, CA_IPV4)
        else IS_MATCHING_IP_SOCKET(u4s, socket, CA_IPV4 | CA_SECURE)
        else IS_MATCHING_IP_SOCKET(m6,  socket, CA_MULTICAST | CA_IPV6)
        else IS_MATCHING_IP_SOCKET(m6s, socket, CA_MULTICAST | CA_IPV6 | CA_SECURE)
        else IS_MATCHING_IP_SOCKET(m4,  socket, CA_MULTICAST | CA_IPV4)
        else IS_MATCHING_IP_SOCKET(m4s, socket, CA_MULTICAST | CA_IPV4 | CA_SECURE)
        else
        {
            break;
        }
        (void)CAReceiveMessage(socket, flags);
        // We will never get more than one match per socket, so always break.
        break;
    }
}

#endif

void CADeInitializeIPGlobals()
{
    CLOSE_SOCKET(u6);
    CLOSE_SOCKET(u6s);
    CLOSE_SOCKET(u4);
    CLOSE_SOCKET(u4s);
    CLOSE_SOCKET(m6);
    CLOSE_SOCKET(m6s);
    CLOSE_SOCKET(m4);
    CLOSE_SOCKET(m4s);

    if (caglobals.ip.netlinkFd != OC_INVALID_SOCKET)
    {
#ifdef _WIN32
        closesocket(caglobals.ip.netlinkFd);
#else
        close(caglobals.ip.netlinkFd);
#endif
        caglobals.ip.netlinkFd = OC_INVALID_SOCKET;
    }
}

static CAResult_t CAReceiveMessage(CASocketFd_t fd, CATransportFlags_t flags)
{
    OIC_LOG(DEBUG, TAG, "IN - CAReceiveMessage");
    char recvBuffer[COAP_MAX_PDU_SIZE] = {0};

    size_t len = 0;
    int level = 0;
    int type = 0;
    int namelen = 0;
    struct sockaddr_storage srcAddr = { .ss_family = 0 };
    unsigned char *pktinfo = NULL;
#if !defined(WSA_CMSG_DATA)
    struct cmsghdr *cmp = NULL;
    struct iovec iov = { .iov_base = recvBuffer, .iov_len = sizeof (recvBuffer) };
    union control
    {
        struct cmsghdr cmsg;
        unsigned char data[CMSG_SPACE(sizeof (struct in6_pktinfo))];
    } cmsg;
#if defined (CONFIG_NET_IPv6)
    if (flags & CA_IPV6)
    {
        namelen = sizeof (struct sockaddr_in6);
        level = IPPROTO_IPV6;
        type = IPV6_PKTINFO;
        len = sizeof (struct in6_pktinfo);
    }
    else
#endif
    {
        namelen = sizeof (struct sockaddr_in);
        level = IPPROTO_IP;
        type = IP_PKTINFO;
        len = sizeof (struct in6_pktinfo);
    }

    struct msghdr msg = { .msg_name = &srcAddr,
                          .msg_namelen = namelen,
                          .msg_iov = &iov,
                          .msg_iovlen = 1,
                          .msg_control = &cmsg,
                          .msg_controllen = CMSG_SPACE(len) };

    ssize_t recvLen = recvmsg(fd, &msg, flags);
    if (OC_SOCKET_ERROR == recvLen)
    {
        OIC_LOG_V(ERROR, TAG, "Recvfrom failed %s", strerror(errno));
        return CA_STATUS_FAILED;
    }
    OIC_LOG_V(DEBUG, TAG, "recvd %u bytes from recvmsg", recvLen);

    if (flags & CA_MULTICAST)
    {
        for (cmp = CMSG_FIRSTHDR(&msg); cmp != NULL; cmp = CMSG_NXTHDR(&msg, cmp))
        {
            if (cmp->cmsg_level == level && cmp->cmsg_type == type)
            {
                pktinfo = CMSG_DATA(cmp);
            }
        }
    }
#else // if defined(WSA_CMSG_DATA)
    union control
    {
        WSACMSGHDR cmsg;
        uint8_t data[WSA_CMSG_SPACE(sizeof (IN6_PKTINFO))];
    } cmsg;
    memset(&cmsg, 0, sizeof(cmsg));

#if defined (CONFIG_NET_IPv6)
    if (flags & CA_IPV6)
    {
        namelen  = sizeof (struct sockaddr_in6);
        level = IPPROTO_IPV6;
        type = IPV6_PKTINFO;
    }
    else
#endif
    {
        namelen = sizeof (struct sockaddr_in);
        level = IPPROTO_IP;
        type = IP_PKTINFO;
    }

    WSABUF iov = {.len = sizeof (recvBuffer), .buf = recvBuffer};
    WSAMSG msg = {.name = (PSOCKADDR)&srcAddr,
                  .namelen = namelen,
                  .lpBuffers = &iov,
                  .dwBufferCount = 1,
                  .Control = {.buf = cmsg.data, .len = sizeof (cmsg)}
                 };

    uint32_t recvLen = 0;
    uint32_t ret = caglobals.ip.wsaRecvMsg(fd, &msg, &recvLen, 0,0);
    OIC_LOG_V(DEBUG, TAG, "WSARecvMsg recvd %u bytes", recvLen);
    if (OC_SOCKET_ERROR == ret)
    {
        OIC_LOG_V(ERROR, TAG, "WSARecvMsg failed %i", WSAGetLastError());
    }

    if (flags & CA_MULTICAST)
    {
        for (WSACMSGHDR *cmp = WSA_CMSG_FIRSTHDR(&msg); cmp != NULL;
             cmp = WSA_CMSG_NXTHDR(&msg, cmp))
        {
            if (cmp->cmsg_level == level && cmp->cmsg_type == type)
            {
                pktinfo = WSA_CMSG_DATA(cmp);
            }
        }
    }
#endif // !defined(WSA_CMSG_DATA)
    CASecureEndpoint_t sep = {.endpoint = {.adapter = CA_ADAPTER_IP, .flags = flags}};

#ifndef __TIZENRT__
    if (flags & CA_IPV6)
    {
        /** @todo figure out correct usage for ifindex, and sin6_scope_id.*/
        if ((flags & CA_MULTICAST) && pktinfo)
        {
            struct in6_addr *addr = &(((struct in6_pktinfo *)pktinfo)->ipi6_addr);
            unsigned char topbits = ((unsigned char *)addr)[0];
            if (topbits != 0xff)
            {
                sep.endpoint.flags &= ~CA_MULTICAST;
            }
        }
    }
    else
#endif
    {
        if ((flags & CA_MULTICAST) && pktinfo)
        {
            struct in_addr *addr = &((struct in_pktinfo *)pktinfo)->ipi_addr;
            uint32_t host = ntohl(addr->s_addr);
            unsigned char topbits = ((unsigned char *)&host)[3];
            if (topbits < 224 || topbits > 239)
            {
                sep.endpoint.flags &= ~CA_MULTICAST;
            }
        }
    }

    CAConvertAddrToName(&srcAddr, namelen, sep.endpoint.addr, &sep.endpoint.port);

    if (flags & CA_SECURE)
    {
#ifdef __WITH_DTLS__
        int ret = CAdecryptSsl(&sep, (uint8_t *)recvBuffer, recvLen);
        OIC_LOG_V(INFO, TAG, "CAdecryptSsl returns [%d]", ret);
#else
        OIC_LOG(ERROR, TAG, "Encrypted message but no DTLS");
#endif
    }
    else
    {
        if (g_packetReceivedCallback)
        {
            OIC_LOG(DEBUG, TAG, "call receivedCB");
            g_packetReceivedCallback(&sep, recvBuffer, recvLen);
        }
    }

    OIC_LOG(DEBUG, TAG, "OUT - CAReceiveMessage");
    return CA_STATUS_OK;

}

void CAIPPullData()
{
    OIC_LOG(DEBUG, TAG, "IN");
    OIC_LOG(DEBUG, TAG, "OUT");
}

static CASocketFd_t CACreateSocket(int family, uint16_t *port, bool isMulticast)
{
    int socktype = SOCK_DGRAM;
#ifdef SOCK_CLOEXEC
    socktype |= SOCK_CLOEXEC;
#endif
    CASocketFd_t fd = socket(family, socktype, IPPROTO_UDP);
    if (OC_INVALID_SOCKET == fd)
    {
        OIC_LOG_V(ERROR, TAG, "create socket failed: %s", CAIPS_GET_ERROR);
        return OC_INVALID_SOCKET;
    }

#if !defined(SOCK_CLOEXEC) && defined(FD_CLOEXEC)
    int fl = fcntl(fd, F_GETFD);
    if (-1 == fl || -1 == fcntl(fd, F_SETFD, fl|FD_CLOEXEC))
    {
        OIC_LOG_V(ERROR, TAG, "set FD_CLOEXEC failed: %s", strerror(errno));
        close(fd);
        return OC_INVALID_SOCKET;
    }
#endif
    struct sockaddr_storage sa = { .ss_family = family };
    socklen_t socklen = 0;

#if defined (CONFIG_NET_IPv6)
    if (family == AF_INET6)
    {
        int on = 1;

        if (OC_SOCKET_ERROR == setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, OPTVAL_T(&on), sizeof (on)))
        {
            OIC_LOG_V(ERROR, TAG, "IPV6_V6ONLY failed: %s", CAIPS_GET_ERROR);
        }

        if (isMulticast && *port) // only do this for multicast ports
        {
#if defined(IPV6_RECVPKTINFO)
            if (OC_SOCKET_ERROR == setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof (on)))
#else
            if (OC_SOCKET_ERROR == setsockopt(fd, IPPROTO_IPV6, IPV6_PKTINFO, OPTVAL_T(&on), sizeof (on)))
#endif
            {
                OIC_LOG_V(ERROR, TAG, "IPV6_RECVPKTINFO failed: %s",CAIPS_GET_ERROR);
            }
        }

        ((struct sockaddr_in6 *)&sa)->sin6_port = htons(*port);
        socklen = sizeof (struct sockaddr_in6);
    }
    else
#endif /* CONFIG_NET_IPv6 */
    {
        if (isMulticast && *port) // only do this for multicast ports
        {
            int on = 1;
            if (OC_SOCKET_ERROR == setsockopt(fd, IPPROTO_IP, IP_PKTINFO, OPTVAL_T(&on), sizeof (on)))
            {
                OIC_LOG_V(ERROR, TAG, "IP_PKTINFO failed: %s", CAIPS_GET_ERROR);
            }
        }

        ((struct sockaddr_in *)&sa)->sin_port = htons(*port);
        socklen = sizeof (struct sockaddr_in);
    }

    if (isMulticast && *port) // use the given port
    {
        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, OPTVAL_T(&on), sizeof (on)))
        {
            OIC_LOG_V(ERROR, TAG, "SO_REUSEADDR failed: %s", CAIPS_GET_ERROR);
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            return OC_INVALID_SOCKET;
        }
    }

    if (OC_SOCKET_ERROR == bind(fd, (struct sockaddr *)&sa, socklen))
    {
        OIC_LOG_V(ERROR, TAG, "bind socket failed: %s", CAIPS_GET_ERROR);
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        return OC_INVALID_SOCKET;
    }

    if (!*port) // return the assigned port
    {
        if (OC_SOCKET_ERROR == getsockname(fd, (struct sockaddr *)&sa, &socklen))
        {
            OIC_LOG_V(ERROR, TAG, "getsockname failed: %s", CAIPS_GET_ERROR);
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            return OC_INVALID_SOCKET;
        }
        *port = ntohs(family == AF_INET6 ?
                      ((struct sockaddr_in6 *)&sa)->sin6_port :
                      ((struct sockaddr_in *)&sa)->sin_port);
    }

    return fd;
}

#define CHECKFD(FD) \
    if (FD > caglobals.ip.maxfd) \
        caglobals.ip.maxfd = FD;
#define NEWSOCKET(FAMILY, NAME, MULTICAST) \
    caglobals.ip.NAME.fd = CACreateSocket(FAMILY, &caglobals.ip.NAME.port, MULTICAST); \
    if (caglobals.ip.NAME.fd == OC_INVALID_SOCKET) \
    {   \
        caglobals.ip.NAME.port = 0; \
        caglobals.ip.NAME.fd = CACreateSocket(FAMILY, &caglobals.ip.NAME.port, MULTICAST); \
    }   \
    CHECKFD(caglobals.ip.NAME.fd)

static void CAInitializeNetlink()
{
    caglobals.ip.netlinkFd = OC_INVALID_SOCKET;
#ifdef __linux__
    // create NETLINK fd for interface change notifications
    struct sockaddr_nl sa = { AF_NETLINK, 0, 0,
                              RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR };

    caglobals.ip.netlinkFd = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, NETLINK_ROUTE);
    if (caglobals.ip.netlinkFd == OC_INVALID_SOCKET)
    {
        OIC_LOG_V(ERROR, TAG, "netlink socket failed: %s", strerror(errno));
    }
    else
    {
        int r = bind(caglobals.ip.netlinkFd, (struct sockaddr *)&sa, sizeof (sa));
        if (r)
        {
            OIC_LOG_V(ERROR, TAG, "netlink bind failed: %s", strerror(errno));
            close(caglobals.ip.netlinkFd);
            caglobals.ip.netlinkFd = OC_INVALID_SOCKET;
        }
        else
        {
            CHECKFD(caglobals.ip.netlinkFd);
        }
    }
#elif defined (__TIZENRT__)
	caglobals.ip.netlinkFd = socket(AF_LWNL, SOCK_RAW, LWNL_ROUTE);
	if (caglobals.ip.netlinkFd < 0) {
		OIC_LOG_V(ERROR, TAG, "netlink socket failed: %s", strerror(errno));
		return;
	}

	struct sockaddr_lwnl addr = {LWNL_DEV_WIFI};
	int r = bind(caglobals.ip.netlinkFd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_lwnl));
	if (r){
        OIC_LOG_V(ERROR, TAG, "netlink bind failed\n");
	}
#endif
}

static void CAInitializeFastShutdownMechanism()
{
    caglobals.ip.selectTimeout = -1; // don't poll for shutdown
    int ret = -1;
#if defined(WSA_WAIT_EVENT_0)
    caglobals.ip.shutdownEvent = WSACreateEvent();
    if (WSA_INVALID_EVENT != caglobals.ip.shutdownEvent)
    {
        ret = 0;
    }
#elif defined(HAVE_PIPE2)
#ifndef __TIZENRT__
    ret = pipe2(caglobals.ip.shutdownFds, O_CLOEXEC);
    CHECKFD(caglobals.ip.shutdownFds[0]);
    CHECKFD(caglobals.ip.shutdownFds[1]);
#endif
#else
#ifndef __TIZENRT__
    ret = pipe(caglobals.ip.shutdownFds);
    if (-1 != ret)
    {
        ret = fcntl(caglobals.ip.shutdownFds[0], F_GETFD);
        if (-1 != ret)
        {
            ret = fcntl(caglobals.ip.shutdownFds[0], F_SETFD, ret|FD_CLOEXEC);
        }
        if (-1 != ret)
        {
            ret = fcntl(caglobals.ip.shutdownFds[1], F_GETFD);
        }
        if (-1 != ret)
        {
            ret = fcntl(caglobals.ip.shutdownFds[1], F_SETFD, ret|FD_CLOEXEC);
        }
        if (-1 == ret)
        {
            close(caglobals.ip.shutdownFds[1]);
            close(caglobals.ip.shutdownFds[0]);
            caglobals.ip.shutdownFds[0] = -1;
            caglobals.ip.shutdownFds[1] = -1;
        }
    }
    CHECKFD(caglobals.ip.shutdownFds[0]);
    CHECKFD(caglobals.ip.shutdownFds[1]);
#endif
#endif
    if (-1 == ret)
    {
        OIC_LOG_V(ERROR, TAG, "fast shutdown mechanism init failed: %s", CAIPS_GET_ERROR);
        caglobals.ip.selectTimeout = SELECT_TIMEOUT; //poll needed for shutdown
    }
}

CAResult_t CAIPStartServer(const ca_thread_pool_t threadPool)
{
    CAResult_t res = CA_STATUS_OK;

    if (caglobals.ip.started)
    {
        return res;
    }
#if defined (_WIN32)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData ={.wVersion = 0};
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        OIC_LOG_V(ERROR, TAG, "WSAStartup failed: %i", err);
        return CA_STATUS_FAILED;
    }
    OIC_LOG(DEBUG, TAG, "WSAStartup Succeeded");
#endif
    if (!IPv4MulticastAddress.s_addr)
    {
        (void)inet_pton(AF_INET, IPv4_MULTICAST, &IPv4MulticastAddress);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_INT, &IPv6MulticastAddressInt);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_LNK, &IPv6MulticastAddressLnk);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_RLM, &IPv6MulticastAddressRlm);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_ADM, &IPv6MulticastAddressAdm);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_SIT, &IPv6MulticastAddressSit);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_ORG, &IPv6MulticastAddressOrg);
        (void)inet_pton(AF_INET6, IPv6_MULTICAST_GLB, &IPv6MulticastAddressGlb);
    }

    if (!caglobals.ip.ipv6enabled && !caglobals.ip.ipv4enabled)
    {
        caglobals.ip.ipv4enabled = true;  // only needed to run CA tests
    }

    if (caglobals.ip.ipv6enabled)
    {
        NEWSOCKET(AF_INET6, u6, false)
        NEWSOCKET(AF_INET6, u6s, false)
        NEWSOCKET(AF_INET6, m6, true)
        NEWSOCKET(AF_INET6, m6s, true)
        OIC_LOG_V(INFO, TAG, "IPv6 unicast port: %u", caglobals.ip.u6.port);
    }
    if (caglobals.ip.ipv4enabled)
    {
        NEWSOCKET(AF_INET, u4, false)
        NEWSOCKET(AF_INET, u4s, false)
        NEWSOCKET(AF_INET, m4, true)
        NEWSOCKET(AF_INET, m4s, true)
        OIC_LOG_V(INFO, TAG, "IPv4 unicast port: %u", caglobals.ip.u4.port);
    }

    OIC_LOG_V(INFO, TAG,
              "socket summary: u6=%d, u6s=%d, u4=%d, u4s=%d, m6=%d, m6s=%d, m4=%d, m4s=%d",
              caglobals.ip.u6.fd, caglobals.ip.u6s.fd, caglobals.ip.u4.fd, caglobals.ip.u4s.fd,
              caglobals.ip.m6.fd, caglobals.ip.m6s.fd, caglobals.ip.m4.fd, caglobals.ip.m4s.fd);

    OIC_LOG_V(INFO, TAG,
              "port summary: u6 port=%d, u6s port=%d, u4 port=%d, u4s port=%d, m6 port=%d,"
              "m6s port=%d, m4 port=%d, m4s port=%d",
              caglobals.ip.u6.port, caglobals.ip.u6s.port, caglobals.ip.u4.port,
              caglobals.ip.u4s.port, caglobals.ip.m6.port, caglobals.ip.m6s.port,
              caglobals.ip.m4.port, caglobals.ip.m4s.port);
#if defined (SIO_GET_EXTENSION_FUNCTION_POINTER)
    caglobals.ip.wsaRecvMsg = NULL;
    GUID GuidWSARecvMsg = WSAID_WSARECVMSG;
    DWORD copied = 0;
    err = WSAIoctl(caglobals.ip.u4.fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidWSARecvMsg, sizeof(GuidWSARecvMsg), &(caglobals.ip.wsaRecvMsg), sizeof(caglobals.ip.wsaRecvMsg), &copied, 0, 0);
    if (0 != err)
    {
        OIC_LOG_V(ERROR, TAG, "WSAIoctl failed %i", WSAGetLastError());
        return CA_STATUS_FAILED;
    }
#endif
    // set up appropriate FD mechanism for fast shutdown
    CAInitializeFastShutdownMechanism();

    // create source of network interface change notifications
    CAInitializeNetlink();

    caglobals.ip.selectTimeout = CAGetPollingInterval(caglobals.ip.selectTimeout);

    res = CAIPStartListenServer();
    if (CA_STATUS_OK != res)
    {
        OIC_LOG_V(ERROR, TAG, "Failed to start listening server![%d]", res);
        return res;
    }

    caglobals.ip.terminate = false;
#ifndef __TIZENRT__
    res = ca_thread_pool_add_task(threadPool, CAReceiveHandler, NULL, NULL);
#else
    res = ca_thread_pool_add_task(threadPool, CAReceiveHandler, NULL, NULL, "IoT_ReceiveHandler",
                                  CONFIG_IOTIVITY_RECEIVEHANDLER_PTHREAD_STACKSIZE);
#endif
    if (CA_STATUS_OK != res)
    {
        OIC_LOG(ERROR, TAG, "thread_pool_add_task failed");
        CAIPStopServer();
        return res;
    }
    OIC_LOG(INFO, TAG, "CAReceiveHandler thread started successfully.");

    caglobals.ip.started = true;
    return CA_STATUS_OK;
}

void CAIPStopServer()
{
    caglobals.ip.started = false;
    caglobals.ip.terminate = true;

    CADeInitializeIPGlobals();

#if !defined(WSA_WAIT_EVENT_0)
#ifndef __TIZENRT__
    if (caglobals.ip.shutdownFds[1] != -1)
    {
        close(caglobals.ip.shutdownFds[1]);
        // receive thread will stop immediately
    }
    else
    {
        // receive thread will stop in SELECT_TIMEOUT seconds.
    }
#endif
#else
    // receive thread will stop immediately.
    if (!WSASetEvent(caglobals.ip.shutdownEvent))
    {
        OIC_LOG_V(DEBUG, TAG, "set shutdown event failed: %#08X", GetLastError());
    }
#endif

    OIC_LOG(INFO, TAG, "Adapter terminated successfully");
}

void CAWakeUpForChange()
{
#if !defined(WSA_WAIT_EVENT_0)
#ifndef __TIZENRT__
    if (caglobals.ip.shutdownFds[1] != -1)
    {
        ssize_t len = 0;
        do
        {
            len = write(caglobals.ip.shutdownFds[1], "w", 1);
        } while ((len == -1) && (errno == EINTR));
        if ((len == -1) && (errno != EINTR) && (errno != EPIPE))
        {
            OIC_LOG_V(DEBUG, TAG, "write failed: %s", strerror(errno));
        }
    }
#endif
#else
    if (!WSASetEvent(caglobals.ip.shutdownEvent))
    {
        OIC_LOG_V(DEBUG, TAG, "set shutdown event failed: %#08X", GetLastError());
    }
#endif
}

static void applyMulticastToInterface4(uint32_t ifindex)
{
    if (!caglobals.ip.ipv4enabled)
    {
        return;
    }

#if defined(USE_IP_MREQN)
    struct ip_mreqn mreq = { .imr_multiaddr = IPv4MulticastAddress,
                             .imr_address.s_addr = htonl(INADDR_ANY),
                             .imr_ifindex = ifindex };
#else
    struct ip_mreq mreq  = { .imr_multiaddr.s_addr = IPv4MulticastAddress.s_addr,
                             .imr_interface.s_addr = htonl(ifindex) };
#endif

    int ret = setsockopt(caglobals.ip.m4.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, OPTVAL_T(&mreq), sizeof (mreq));
    if (OC_SOCKET_ERROR == ret)
    {
#if !defined(WSAEINVAL)
        if (EADDRINUSE != errno)
#else
        if (WSAEINVAL != WSAGetLastError()) // Joining multicast group more than once (IPv4 Flavor)
#endif
        {
            OIC_LOG_V(ERROR, TAG, "       IPv4 IP_ADD_MEMBERSHIP failed: %s", CAIPS_GET_ERROR);
        }
    }
    ret = setsockopt(caglobals.ip.m4s.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, OPTVAL_T(&mreq), sizeof (mreq));
    if (OC_SOCKET_ERROR == ret)
    {
#if !defined(WSAEINVAL)
        if (EADDRINUSE != errno)
#else
        if (WSAEINVAL != WSAGetLastError()) // Joining multicast group more than once (IPv4 Flavor)
#endif
        {
            OIC_LOG_V(ERROR, TAG, "SECURE IPv4 IP_ADD_MEMBERSHIP failed: %s", CAIPS_GET_ERROR);
        }
    }
}

static void applyMulticast6(int fd, struct in6_addr *addr, uint32_t ifindex)
{
#ifndef __TIZENRT__
    struct ipv6_mreq mreq = {.ipv6mr_multiaddr = {0},
                             .ipv6mr_interface = ifindex };

    // VS2013 has problems with struct copies inside struct initializers, so copy separately.
    mreq.ipv6mr_multiaddr = *addr;

    int ret = setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, OPTVAL_T(&mreq), sizeof (mreq));
    if (OC_SOCKET_ERROR == ret)
    {
#if !defined(_WIN32)
                if (EADDRINUSE != errno)
#else
                if (WSAEINVAL != WSAGetLastError()) // Joining multicast group more than once (IPv6 Flavor)
#endif
        {
            OIC_LOG_V(ERROR, TAG, "IPv6 IPV6_JOIN_GROUP failed: %s", CAIPS_GET_ERROR);
        }
    }
#endif /* ! __TIZENRT__ */
}

static void applyMulticastToInterface6(uint32_t ifindex)
{
    if (!caglobals.ip.ipv6enabled)
    {
        return;
    }
    //applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressInt, ifindex);
    applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressLnk, ifindex);
    applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressRlm, ifindex);
    //applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressAdm, ifindex);
    applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressSit, ifindex);
    //applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressOrg, ifindex);
    //applyMulticast6(caglobals.ip.m6.fd, &IPv6MulticastAddressGlb, ifindex);

    //applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressInt, ifindex);
    applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressLnk, ifindex);
    applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressRlm, ifindex);
    //applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressAdm, ifindex);
    applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressSit, ifindex);
    //applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressOrg, ifindex);
    //applyMulticast6(caglobals.ip.m6s.fd, &IPv6MulticastAddressGlb, ifindex);
}

CAResult_t CAIPStartListenServer()
{
    u_arraylist_t *iflist = CAIPGetInterfaceInformation(0);
    if (!iflist)
    {
        OIC_LOG_V(ERROR, TAG, "CAIPGetInterfaceInformation() failed: %s", strerror(errno));
        return CA_STATUS_FAILED;
    }

    uint32_t len = u_arraylist_length(iflist);
    OIC_LOG_V(DEBUG, TAG, "IP network interfaces found: %d", len);

    for (uint32_t i = 0; i < len; i++)
    {
        CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);

        if (!ifitem)
        {
            continue;
        }
        if ((ifitem->flags & IFF_UP_RUNNING_FLAGS) != IFF_UP_RUNNING_FLAGS)
        {
            continue;
        }
        if (ifitem->family == AF_INET)
        {
            OIC_LOG_V(DEBUG, TAG, "Adding IPv4 interface(%d) to multicast group", ifitem->index);
            applyMulticastToInterface4(ifitem->index);
        }
        if (ifitem->family == AF_INET6)
        {
            OIC_LOG_V(DEBUG, TAG, "Adding IPv6 interface(%d) to multicast group", ifitem->index);
            applyMulticastToInterface6(ifitem->index);
        }
    }

    u_arraylist_destroy(iflist);
    return CA_STATUS_OK;
}

CAResult_t CAIPStopListenServer()
{
    u_arraylist_t *iflist = CAIPGetInterfaceInformation(0);
    if (!iflist)
    {
        OIC_LOG_V(ERROR, TAG, "Get interface info failed: %s", strerror(errno));
        return CA_STATUS_FAILED;
    }

    uint32_t len = u_arraylist_length(iflist);
    OIC_LOG_V(DEBUG, TAG, "IP network interfaces found: %d", len);

    for (uint32_t i = 0; i < len; i++)
    {
        CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);

        if (!ifitem)
        {
            continue;
        }
        if ((ifitem->flags & IFF_UP_RUNNING_FLAGS) != IFF_UP_RUNNING_FLAGS)
        {
            continue;
        }
        if (ifitem->family == AF_INET)
        {
            CLOSE_SOCKET(m4);
            CLOSE_SOCKET(m4s);
            OIC_LOG_V(DEBUG, TAG, "IPv4 network interface: %s cloed", ifitem->name);
        }
        if (ifitem->family == AF_INET6)
        {
            CLOSE_SOCKET(m6);
            CLOSE_SOCKET(m6s);
            OIC_LOG_V(DEBUG, TAG, "IPv6 network interface: %s", ifitem->name);
        }
    }
    u_arraylist_destroy(iflist);
    return CA_STATUS_OK;
}

void CAProcessNewInterface(CAInterface_t *ifitem)
{
    if (!ifitem)
    {
        OIC_LOG(DEBUG, TAG, "ifitem is null");
        return;
    }

    if (ifitem->family == AF_INET6)
    {
        OIC_LOG_V(DEBUG, TAG, "Adding a new IPv6 interface(%d) to multicast group", ifitem->index);
        applyMulticastToInterface6(ifitem->index);
    }
    if (ifitem->family == AF_INET)
    {
        OIC_LOG_V(DEBUG, TAG, "Adding a new IPv4 interface(%d) to multicast group", ifitem->index);
        applyMulticastToInterface4(ifitem->index);
    }
}

void CAIPSetPacketReceiveCallback(CAIPPacketReceivedCallback callback)
{
    g_packetReceivedCallback = callback;
}

static void sendData(int fd, const CAEndpoint_t *endpoint,
                     const void *data, uint32_t dlen,
                     const char *cast, const char *fam)
{
    OIC_LOG(DEBUG, TAG, "IN");

    if (!endpoint)
    {
        OIC_LOG(DEBUG, TAG, "endpoint is null");
        if (g_ipErrorHandler)
        {
            g_ipErrorHandler(endpoint, data, dlen, CA_STATUS_INVALID_PARAM);
        }
        return;
    }

    (void)cast;  // eliminates release warning
    (void)fam;

    struct sockaddr_storage sock = { .ss_family = 0 };
    CAConvertNameToAddr(endpoint->addr, endpoint->port, &sock);

    socklen_t socklen = 0;
    if (sock.ss_family == AF_INET6)
    {
        /** @todo figure out correct usage for ifindex, and sin6_scope_id */
        socklen = sizeof(struct sockaddr_in6);
    }
    else
    {
        socklen = sizeof(struct sockaddr_in);
    }

#ifdef TB_LOG
    const char *secure = (endpoint->flags & CA_SECURE) ? "secure " : "";
#endif
#if !defined(_WIN32)
    ssize_t len = sendto(fd, data, dlen, 0, (struct sockaddr *)&sock, socklen);
    if (OC_SOCKET_ERROR == len)
    {
         // If logging is not defined/enabled.
        if (g_ipErrorHandler)
        {
            g_ipErrorHandler(endpoint, data, dlen, CA_SEND_FAILED);
        }
#ifdef TB_LOG
        OIC_LOG_V(ERROR, TAG, "%s%s %s sendTo failed: %s", secure, cast, fam, strerror(errno));
#endif
        CALogSendStateInfo(endpoint->adapter, endpoint->addr, endpoint->port,
                           len, false, strerror(errno));
    }
    else
    {
#ifdef TB_LOG
        OIC_LOG_V(INFO, TAG, "%s%s %s sendTo is successful: %zd bytes", secure, cast, fam, len);
#endif
        CALogSendStateInfo(endpoint->adapter, endpoint->addr, endpoint->port,
                           len, true, NULL);
    }
#else
    int err = 0;
    int len = 0;
    int sent = 0;
    do {
        len = sendto(fd, ((char*)data) + sent, dlen - sent, 0, (struct sockaddr *)&sock, socklen);
        if (OC_SOCKET_ERROR == len)
        {
            err = WSAGetLastError();
            if ((WSAEWOULDBLOCK != err) && (WSAENOBUFS != err))
            {
                 // If logging is not defined/enabled.
                if (g_ipErrorHandler)
                {
                    g_ipErrorHandler(endpoint, data, dlen, CA_SEND_FAILED);
                }

                OIC_LOG_V(ERROR, TAG, "%s%s %s sendTo failed: %i", secure, cast, fam, err);
            }
        }
        else
        {
            sent += len;
            if (sent != len)
            {
                OIC_LOG_V(INFO, TAG, "%s%s %s sendTo (Partial Send) is successful: "
                          "currently sent: %ld bytes, "
                          "total sent: %ld bytes, "
                          "remaining: %ld bytes",
                          secure, cast, fam, len, sent, dlen-sent);
            }
            else
            {
                OIC_LOG_V(INFO, TAG, "%s%s %s sendTo is successful: %ld bytes",
                                     secure, cast, fam, len);
            }
        }
    } while ((OC_SOCKET_ERROR == len) && ((WSAEWOULDBLOCK == err) || (WSAENOBUFS == err)) || (sent < dlen));
#endif
}

static void sendMulticastData6(const u_arraylist_t *iflist,
                               CAEndpoint_t *endpoint,
                               const void *data, uint32_t datalen)
{
#ifndef __TIZENRT__
    if (!endpoint)
    {
        OIC_LOG(DEBUG, TAG, "endpoint is null");
        return;
    }

    int scope = endpoint->flags & CA_SCOPE_MASK;
    char *ipv6mcname = ipv6mcnames[scope];
    if (!ipv6mcname)
    {
        OIC_LOG_V(INFO, TAG, "IPv6 multicast scope invalid: %d", scope);
        return;
    }
    OICStrcpy(endpoint->addr, sizeof(endpoint->addr), ipv6mcname);
    int fd = caglobals.ip.u6.fd;

    uint32_t len = u_arraylist_length(iflist);
    for (uint32_t i = 0; i < len; i++)
    {
        CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);
        if (!ifitem)
        {
            continue;
        }
        if ((ifitem->flags & IFF_UP_RUNNING_FLAGS) != IFF_UP_RUNNING_FLAGS)
        {
            continue;
        }
        if (ifitem->family != AF_INET6)
        {
            continue;
        }

        bool isMobile = false;
        for (uint32_t j = 0; j < MOBILE_INTERFACES; j++)
        {
            if (strstr(ifitem->name, mobileinferfaces[j]))
            {
                isMobile = true;
                break;
            }
        }
        if (isMobile)
        {
            continue;
        }

        int index = ifitem->index;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, OPTVAL_T(&index), sizeof (index)))
        {
            OIC_LOG_V(ERROR, TAG, "setsockopt6 failed: %s", CAIPS_GET_ERROR);
            return;
        }

        // Set multicast packet TTL; default TTL is 1
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &multicastTTL, sizeof(multicastTTL)))
        {
            OIC_LOG_V(ERROR, TAG, "IPV6_MULTICAST_HOPS failed: %s", CAIPS_GET_ERROR);
        }
        sendData(fd, endpoint, data, datalen, "multicast", "ipv6");
    }
#endif /* ! __TIZENRT__*/
}

static void sendMulticastData4(const u_arraylist_t *iflist,
                               CAEndpoint_t *endpoint,
                               const void *data, uint32_t datalen)
{
    VERIFY_NON_NULL_VOID(endpoint, TAG, "endpoint is NULL");

#if defined(USE_IP_MREQN)
    struct ip_mreqn mreq = { .imr_multiaddr = IPv4MulticastAddress,
                             .imr_address.s_addr = htonl(INADDR_ANY),
                             .imr_ifindex = 0};
#else
    struct ip_mreq mreq  = { .imr_multiaddr.s_addr = IPv4MulticastAddress.s_addr,
                             .imr_interface = {0}};
#endif

    OICStrcpy(endpoint->addr, sizeof(endpoint->addr), IPv4_MULTICAST);
    int fd = caglobals.ip.u4.fd;

    uint32_t len = u_arraylist_length(iflist);
    for (uint32_t i = 0; i < len; i++)
    {
        CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);
        if (!ifitem)
        {
            continue;
        }
        if ((ifitem->flags & IFF_UP_RUNNING_FLAGS) != IFF_UP_RUNNING_FLAGS)
        {
            continue;
        }
        if (ifitem->family != AF_INET)
        {
            continue;
        }

        bool isMobile = false;
        for (uint32_t j = 0; j < MOBILE_INTERFACES; j++)
        {
            if (strstr(ifitem->name, mobileinferfaces[j]))
            {
                isMobile = true;
                break;
            }
        }
        if (isMobile)
        {
            continue;
        }

#if defined(USE_IP_MREQN)
        mreq.imr_ifindex = ifitem->index;
#else
        mreq.imr_interface.s_addr = htonl(ifitem->index);
#endif
        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, OPTVAL_T(&mreq), sizeof (mreq)))
        {
            OIC_LOG_V(ERROR, TAG, "send IP_MULTICAST_IF failed: %s (using defualt)",
                    CAIPS_GET_ERROR);
        }

        // Set multicast packet TTL; default TTL is 1
        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &multicastTTL, sizeof(multicastTTL)))
        {
            OIC_LOG_V(ERROR, TAG, "IP_MULTICAST_TTL failed: %s", CAIPS_GET_ERROR);
        }
        sendData(fd, endpoint, data, datalen, "multicast", "ipv4");
    }
}

void CAIPSendData(CAEndpoint_t *endpoint, const void *data, uint32_t datalen,
                  bool isMulticast)
{
    VERIFY_NON_NULL_VOID(endpoint, TAG, "endpoint is NULL");
    VERIFY_NON_NULL_VOID(data, TAG, "data is NULL");

    bool isSecure = (endpoint->flags & CA_SECURE) != 0;

    if (isMulticast)
    {
        endpoint->port = isSecure ? CA_SECURE_COAP : CA_COAP;

        u_arraylist_t *iflist = CAIPGetInterfaceInformation(0);
        if (!iflist)
        {
            OIC_LOG_V(ERROR, TAG, "get interface info failed: %s", strerror(errno));
            return;
        }

        if ((endpoint->flags & CA_IPV6) && caglobals.ip.ipv6enabled)
        {
            sendMulticastData6(iflist, endpoint, data, datalen);
        }
        if ((endpoint->flags & CA_IPV4) && caglobals.ip.ipv4enabled)
        {
            sendMulticastData4(iflist, endpoint, data, datalen);
        }

        u_arraylist_destroy(iflist);
    }
    else
    {
        if (!endpoint->port)    // unicast discovery
        {
            endpoint->port = isSecure ? CA_SECURE_COAP : CA_COAP;
        }

        CASocketFd_t fd;
        if (caglobals.ip.ipv6enabled && (endpoint->flags & CA_IPV6))
        {
            fd = isSecure ? caglobals.ip.u6s.fd : caglobals.ip.u6.fd;
#ifndef __WITH_DTLS__
            fd = caglobals.ip.u6.fd;
#endif
            sendData(fd, endpoint, data, datalen, "unicast", "ipv6");
        }
        if (caglobals.ip.ipv4enabled && (endpoint->flags & CA_IPV4))
        {
            fd = isSecure ? caglobals.ip.u4s.fd : caglobals.ip.u4.fd;
#ifndef __WITH_DTLS__
            fd = caglobals.ip.u4.fd;
#endif
            sendData(fd, endpoint, data, datalen, "unicast", "ipv4");
        }
    }
}

CAResult_t CAGetIPInterfaceInformation(CAEndpoint_t **info, uint32_t *size)
{
    VERIFY_NON_NULL(info, TAG, "info is NULL");
    VERIFY_NON_NULL(size, TAG, "size is NULL");

    u_arraylist_t *iflist = CAIPGetInterfaceInformation(0);
    if (!iflist)
    {
        OIC_LOG_V(ERROR, TAG, "get interface info failed: %s", strerror(errno));
        return CA_STATUS_FAILED;
    }

    uint32_t len = u_arraylist_length(iflist);
    uint32_t length = len;

#ifdef __WITH_DTLS__
    //If DTLS is supported, each interface can support secure port as well
    length = len * 2;
#endif

    CAEndpoint_t *eps = (CAEndpoint_t *)OICCalloc(length, sizeof (CAEndpoint_t));
    if (!eps)
    {
        OIC_LOG(ERROR, TAG, "Malloc Failed");
        u_arraylist_destroy(iflist);
        return CA_MEMORY_ALLOC_FAILED;
    }

    for (uint32_t i = 0, j = 0; i < len; i++)
    {
        CAInterface_t *ifitem = (CAInterface_t *)u_arraylist_get(iflist, i);
        if(!ifitem)
        {
            continue;
        }

        eps[j].adapter = CA_ADAPTER_IP;
        eps[j].ifindex = 0;

        if (ifitem->family == AF_INET6)
        {
            eps[j].flags = CA_IPV6;
            eps[j].port = caglobals.ip.u6.port;
        }
        else
        {
            eps[j].flags = CA_IPV4;
            eps[j].port = caglobals.ip.u4.port;
        }
        OICStrcpy(eps[j].addr, sizeof(eps[j].addr), ifitem->addr);

#ifdef __WITH_DTLS__
        j++;

        eps[j].adapter = CA_ADAPTER_IP;
        eps[j].ifindex = 0;

        if (ifitem->family == AF_INET6)
        {
            eps[j].flags = CA_IPV6 | CA_SECURE;
            eps[j].port = caglobals.ip.u6s.port;
        }
        else
        {
            eps[j].flags = CA_IPV4 | CA_SECURE;
            eps[j].port = caglobals.ip.u4s.port;
        }
        OICStrcpy(eps[j].addr, sizeof(eps[j].addr), ifitem->addr);
#endif
        j++;
    }

    *info = eps;
    *size = length;

    u_arraylist_destroy(iflist);

    return CA_STATUS_OK;
}

void CAIPSetErrorHandler(CAIPErrorHandleCallback errorHandleCallback)
{
    g_ipErrorHandler = errorHandleCallback;
}

CAResult_t CAIPSetMulticastTTL(size_t ttl)
{
    multicastTTL = ttl;
    return CA_STATUS_OK;
}

CAResult_t CAIPGetMulticastTTL(size_t *ttl)
{
    VERIFY_NON_NULL(ttl, TAG, "ttl is NULL");

    *ttl = multicastTTL;
    return CA_STATUS_OK;
}
