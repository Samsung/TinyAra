/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/// @file tc_net_setsockopt.c
/// @brief Test Case Example for setsockopt() API
#include <tinyara/config.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netutils/netlib.h>

#include <sys/socket.h>

#include "tc_internal.h"

#if CONFIG_NET_LWIP_IGMP && CONFIG_NET_UDP
/**
* @fn                   :tc_net_setsockopt_multicast_ttl_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_multicast_ttl_p(int s)
{
	int ret = -1;
	u8_t optval = 1;
	ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}
/**
* @fn                   :tc_net_setsockopt_multicast_ttl_loop_own_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_multicast_ttl_loop_own_p(int s)
{
	int ret = -1;
	u8_t loop = 1;
	ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}
/**
* @fn                   :tc_net_setsockopt_multicast_ttl_loop_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_multicast_ttl_loop_p(int s)
{
	int ret = -1;
	u8_t loop = 250;
	ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_net_setsockopt_multicast_if_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor, ipv4 addr
* Postconditions        :
* @return               :void
*/

static void tc_net_setsockopt_multicast_if_p(int s, const char *my_ipv4addr)
{
	int ret = -1;
	struct sockaddr_in interface_addr;
	interface_addr.sin_addr.s_addr = inet_addr(my_ipv4addr);

	ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &interface_addr, sizeof(interface_addr));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_net_setsockopt_multicast_add_group_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor, group ipv4 addr
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_multicast_add_group_p(int udp_fd)
{
	int ret = -1;
	struct ip_mreq mreq;
	char *group_ipv4addr = "239.255.255.255";

	mreq.imr_multiaddr.s_addr = inet_addr(group_ipv4addr);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ret = setsockopt(udp_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @fn                   :tc_net_setsockopt_multicast_drop_group_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor, group ipv4 addr
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_multicast_drop_group_p(int udp_fd)
{
	int ret = -1;
	struct ip_mreq mreq;
	char *group_ipv4addr = "239.255.255.255";

	mreq.imr_multiaddr.s_addr = inet_addr(group_ipv4addr);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	ret = setsockopt(udp_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}
#endif

/**
   * @testcase		   :tc_net_setsockopt_multicast_tcp_nodelay_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_multicast_tcp_nodelay_p(int s)
{
	int ret = -1;
	int option = 1;

	ret = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}

/**
   * @testcase		   :tc_net_setsockopt_multicast_tcp_keepalive_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_multicast_tcp_keepalive_p(int s)
{
	int ret = -1;
	int option = 1;

	ret = setsockopt(s, IPPROTO_TCP, TCP_KEEPALIVE, &option, sizeof(option));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}

#if CONFIG_NET_TCP_KEEPALIVE
/**
   * @testcase		   :tc_net_setsockopt_multicast_tcp_keepidle_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_multicast_tcp_keepidle_p(int s)
{
	int ret = -1;
	int keepidle = 30;

	ret = setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}

/**
   * @testcase		   :tc_net_setsockopt_multicast_tcp_keepintvl_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_multicast_tcp_keepintvl_p(int s)
{
	int ret = -1;
	int keepintvl = 120;

	ret = setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}

/**
   * @testcase		   :tc_net_setsockopt_multicast_tcp_keepcnt_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_multicast_tcp_keepcnt_p(int s)
{
	int ret = -1;
	int keepcnt = 5;

	ret = setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}
#endif
/**
   * @testcase		   :tc_net_setsockopt_ip_tos_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_ip_tos_p(int s)
{
	int ret = -1;
	int optval = 1;

	ret = setsockopt(s, IPPROTO_IP, IP_TOS, &optval, sizeof(optval));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}

/**
* @fn                   :tc_net_setsockopt_ip_ttl_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_ip_ttl_p(int s)
{
	int ret = -1;
	int optval = 1;

	ret = setsockopt(s, IPPROTO_IP, IP_TTL, &optval, sizeof(optval));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}



/**
   * @testcase		   :tc_net_setsockopt_broadcast_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_broadcast_p(int s)
{
	int ret = 0;
	int optval = 1;				//broadcast value

	ret = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof optval);

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}

#if CONFIG_NET_SO_SNDTIMEO
/**
* @fn                   :tc_net_setsockopt_sndtimo_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_sndtimo_p(int s)
{
	int ret = -1;
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}
#endif

#if CONFIG_NET_UDP
/**
* @fn                   :tc_net_setsockopt_no_check_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_no_check_p(void)
{
	int ret = -1;
	int optval = 1;
	int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_fd < 0) {
		printf("socket creation error (%s) line:%d\n", __FUNCTION__, __LINE__);
		return;
	}

	ret = setsockopt(udp_fd, SOL_SOCKET, SO_NO_CHECK, &optval, sizeof(optval));
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
	close(udp_fd);
}
#endif

#if CONFIG_NET_SO_RCVTIMEO
/**
   * @testcase		   :tc_net_setsockopt_rcvtimo_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_rcvtimo_p(int s)
{
	int ret = -1;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}
#endif

#if CONFIG_NET_SO_REUSE
/**
   * @testcase		   :tc_net_setsockopt_reuseaddr_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_reuseaddr_p(int s)
{
	int ret = -1;
	int option = 1;

	ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof option);

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}
#endif
#if CONFIG_NET_SO_RCVBUF
/**
* @fn                   :tc_net_setsockopt_rcvbuf_p
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_rcvbuf_p(int s)
{
	int ret = -1;
	int size = 1000;

	ret = setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, size);
	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();
}
#endif

/**
   * @testcase		   :tc_net_setsockopt_keepalive_p
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_keepalive_p(int s)
{
	int ret = -1;
	int optval = 1;

	ret = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);

	TC_ASSERT_GEQ("setsockopt", ret, 0);
	TC_SUCCESS_RESULT();

}



/**
   * @testcase		   :tc_net_setsockopt_bad_filedesc_n
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :
   * @postcondition	   :
   */
static void tc_net_setsockopt_bad_filedesc_n(void)
{
	int ret = -1;
	ret = setsockopt(-1, SOL_SOCKET, 0, 0, 0);

	TC_ASSERT_EQ("setsockopt", ret, -1);
	TC_SUCCESS_RESULT();

}

/**
   * @testcase		   :tc_net_setsockopt_invalid_filedesc_n
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :
   * @postcondition	   :
   */
static void tc_net_setsockopt_invalid_filedesc_n(void)
{
	int ret = -1;

	ret = setsockopt(0, SOL_SOCKET, 0, 0, 0);

	TC_ASSERT_EQ("setsockopt", ret, -1);
	TC_SUCCESS_RESULT();

}

/**
   * @testcase		   :tc_net_setsockopt_invalid_level_n
   * @brief		   :
   * @scenario		   :
   * @apicovered	   :setsockopt()
   * @precondition	   :socket file descriptor
   * @postcondition	   :
   */
static void tc_net_setsockopt_invalid_level_n(int s)
{
	int ret = -1;

	ret = setsockopt(s, -1, 0, 0, 0);

	TC_ASSERT_EQ("setsockopt", ret, -1);
	TC_SUCCESS_RESULT();

}

/**
* @fn                   :tc_net_setsockopt_invalid_opt_name_n
* @brief                :
* @Scenario             :
* @API's covered        :setsockopt()
* Preconditions         :socket file descriptor
* Postconditions        :
* @return               :void
*/
static void tc_net_setsockopt_invalid_opt_name_n(int s)
{
	int ret = -1;

	ret = setsockopt(s, SOL_SOCKET, TCP_NODELAY, 0, 0);
	TC_ASSERT_EQ("setsockopt", ret, -1);
	TC_SUCCESS_RESULT();
}

static void tc_net_setsockopt_solsocket_p(int fd)
{
	tc_net_setsockopt_keepalive_p(fd);
	tc_net_setsockopt_broadcast_p(fd);
#if CONFIG_NET_SO_RCVBUF
	tc_net_setsockopt_rcvbuf_p(fd);
#endif
#if CONFIG_NET_SO_REUSE
	tc_net_setsockopt_reuseaddr_p(fd);
#endif
#if CONFIG_NET_SO_RCVTIMEO
	tc_net_setsockopt_rcvtimo_p(fd);
#endif
#if CONFIG_NET_SO_SNDTIMEO
	tc_net_setsockopt_sndtimo_p(fd);
#endif
#if CONFIG_NET_UDP
	tc_net_setsockopt_no_check_p();
#endif
}

#if CONFIG_NET_LWIP_IGMP && CONFIG_NET_UDP
static void tc_net_setsockopt_multicast_p(void)
{
	char *ip = "127.0.0.1";
	int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_fd < 0) {
		printf("socket creation error (%s) line:%d\n", __FUNCTION__, __LINE__);
		return;
	}
	tc_net_setsockopt_multicast_add_group_p(udp_fd);
	tc_net_setsockopt_multicast_drop_group_p(udp_fd);

	tc_net_setsockopt_multicast_if_p(udp_fd, ip);
	tc_net_setsockopt_multicast_ttl_loop_p(udp_fd);
	tc_net_setsockopt_multicast_ttl_loop_own_p(udp_fd);
	tc_net_setsockopt_multicast_ttl_p(udp_fd);
	close(udp_fd);
}
#endif

static void tc_net_setsockopt_ip_p(int fd)
{
	tc_net_setsockopt_ip_ttl_p(fd);
	tc_net_setsockopt_ip_tos_p(fd);
#if CONFIG_NET_LWIP_IGMP && CONFIG_NET_UDP
	tc_net_setsockopt_multicast_p();
#endif
}

static void tc_net_setsockopt_tcp_p(int fd)
{
#if CONFIG_NET_TCP_KEEPALIVE
	tc_net_setsockopt_multicast_tcp_keepcnt_p(fd);
	tc_net_setsockopt_multicast_tcp_keepintvl_p(fd);
	tc_net_setsockopt_multicast_tcp_keepidle_p(fd);
#endif
	tc_net_setsockopt_multicast_tcp_keepalive_p(fd);
	tc_net_setsockopt_multicast_tcp_nodelay_p(fd);
}

static void tc_net_setsockopt_n(int fd)
{
	tc_net_setsockopt_invalid_filedesc_n();
	tc_net_setsockopt_bad_filedesc_n();
	tc_net_setsockopt_invalid_level_n(fd);
	tc_net_setsockopt_invalid_opt_name_n(fd);
}

static void tc_net_setsockopt_p(int fd)
{
	tc_net_setsockopt_solsocket_p(fd);
	tc_net_setsockopt_ip_p(fd);
	tc_net_setsockopt_tcp_p(fd);
}

/****************************************************************************
 * Name: setsockopt()
 ****************************************************************************/

void net_setsockopt_main(void)
{
	int fd = -1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("socket creation error (%s) line:%d\n", __FUNCTION__, __LINE__);
		return;
	}
	tc_net_setsockopt_n(fd);
	tc_net_setsockopt_p(fd);
	close(fd);
	return;
}
