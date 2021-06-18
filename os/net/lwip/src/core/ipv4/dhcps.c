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

/**
 * The MIT License (MIT)

 * Copyright (c) 2014 zeroday nodemcu.com

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

/**
 * @file dhcps.c
 * @brief DHCP Server Program
 **/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_DHCP /* don't build if not configured for use in lwipopts.h */
#if LWIP_DHCPS
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/def.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/netif/etharp.h"
#include "lwip/prot/dhcp.h"

#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if LWIP_IPV6
#define IN_ADDR_T(ipaddr)   (((ipaddr).u_addr.ip4))
#else
#define IN_ADDR_T(ipaddr)   (((ipaddr)))
#endif

#define DHCPS_LEASE_TIME_DEF    CONFIG_LWIP_DHCPS_LEASE_DEF
#define DHCPS_LEASE_TIMER       DHCPS_LEASE_TIME_DEF
#define BOOTP_BROADCAST         0x8000
#define BOOTP_UNICAST           0X0000

#define DHCPS_STATE_OFFER   1
#define DHCPS_STATE_DECLINE 2
#define DHCPS_STATE_ACK     3
#define DHCPS_STATE_NAK     4
#define DHCPS_STATE_IDLE    5
#define DHCPS_STATE_RELEASE 6

#define DHCP_OPTION_PERFORM_ROUTER_DISCOVERY  31
#define DHCP_OPTION_END                       255

#define DHCP_MAX_MSG_LEN_MIN_REQUIRED         576

#define LWIP_DHCPS_SERVER_IP CONFIG_LWIP_DHCPS_SERVER_IP
#define LWIP_DHCPS_SERVER_NETMASK CONFIG_LWIP_DHCPS_SERVER_NETMASK
#define LWIP_DHCPS_MAX_STATION_NUM CONFIG_LWIP_DHCPS_MAX_STATION_NUM

#define MAX_STATION_NUM LWIP_DHCPS_MAX_STATION_NUM

/****************************************************************************
 * Private Data
 ****************************************************************************/
_uint32_t dhcps_lease_time = DHCPS_LEASE_TIME_DEF;	//minute

struct dhcps_msg {
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint8_t xid[4];
	uint16_t secs;
	uint16_t flags;
	uint8_t ciaddr[4];
	uint8_t yiaddr[4];
	uint8_t siaddr[4];
	uint8_t giaddr[4];
	uint8_t chaddr[16];
	uint8_t sname[64];
	uint8_t file[128];
	uint8_t options[312];
};

struct _dhcps_lease {
	bool enable;
	ip_addr_t start_ip;
	ip_addr_t end_ip;
};

struct dhcps_state {
	int state;
};

struct dhcps_pool {
	ip_addr_t ip;
	_uint8_t mac[6];
	_uint32_t lease_timer;
};

struct list_node {
	void *pnode;
	struct list_node *pnext;
};

static struct _dhcps_lease dhcps_lease;
static struct udp_pcb *pcb_dhcps;

static ip_addr_t broadcast_dhcps;
static ip_addr_t server_address;
static ip_addr_t client_address;
static ip_addr_t client_address_plus;

static struct list_node *plist;

//static _uint8_t offer = 0xFF;
static bool renew;
static const _uint8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 };	//0x63825363; // 99.130.83.99

static dhcp_sta_joined_cb g_dhcp_sta_joined = NULL;
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void dhcps_node_insert_to_list(struct list_node **phead, struct list_node *pinsert);
static void dhcps_node_remove_from_list(struct list_node **phead, struct list_node *pdelete);
static uint8_t *dhcps_add_msg_type(uint8_t *optptr, uint8_t type);
static uint8_t *dhcps_add_offer_options(uint8_t *optptr);
static uint8_t *dhcps_add_end(uint8_t *optptr);
static void dhcps_create_msg(struct dhcps_msg *m);
static void dhcps_send_msg(struct dhcps_msg *m, u8_t msg_type);
static uint8_t dhcps_parse_options(uint8_t *optptr, int16_t len);
static int16_t dhcps_parse_msg(struct dhcps_msg *m, u16_t len);
static void dhcps_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

#if 0							/* Temporary disabled : will be used */
static void kill_oldest_dhcps_pool(void);
#endif

/****************************************************************************
 * Static Functions
 ****************************************************************************/

/**
 * @fn dhcps_node_insert_to_list
 * @brief insert the node to the list
 * @param phead
 * @param pinsert
 * @return void
 * @section
 * **/

static void dhcps_node_insert_to_list(struct list_node **phead, struct list_node *pinsert)
{
	struct list_node *list = NULL;
	struct dhcps_pool *pdhcps_pool = NULL;
	struct dhcps_pool *pdhcps_node = NULL;

	if (*phead == NULL) {
		*phead = pinsert;
	} else {
		list = *phead;
		pdhcps_node = pinsert->pnode;
		pdhcps_pool = list->pnode;

		if (IN_ADDR_T(pdhcps_node->ip).addr < IN_ADDR_T(pdhcps_pool->ip).addr) {
			pinsert->pnext = list;
			*phead = pinsert;
		} else {
			while (list->pnext != NULL) {
				pdhcps_pool = list->pnext->pnode;
				if (IN_ADDR_T(pdhcps_node->ip).addr < IN_ADDR_T(pdhcps_pool->ip).addr) {
					pinsert->pnext = list->pnext;
					list->pnext = pinsert;
					break;
				}
				list = list->pnext;
			}

			if (list->pnext == NULL) {
				list->pnext = pinsert;
			}
		}
	}
}

/**
 * @fn dhcps_node_delete_from_list
 * @brief remove the node from list
 * @param phead
 * @param pdelete
 * @return void
 * @section
 * **/
static void dhcps_node_remove_from_list(struct list_node **phead, struct list_node *pdelete)
{
	struct list_node *list = NULL;

	list = *phead;
	if (list == NULL) {
		*phead = NULL;
	} else {
		if (list == pdelete) {
			*phead = list->pnext;
		} else {
			while (list != NULL) {
				if (list->pnext == pdelete) {
					list->pnext = pdelete->pnext;
				}
				list = list->pnext;
			}
		}
	}
}

/**
 * @fn dhcps_add_msg_type
 * @brief add DHCP msg type
 * @param optptr
 * @param type
 * @return optptr address
 * @section
 * **/
static uint8_t *dhcps_add_msg_type(uint8_t *optptr, uint8_t type)
{
	*optptr++ = DHCP_OPTION_MESSAGE_TYPE;
	*optptr++ = 1;
	*optptr++ = type;
	return optptr;
}

/**
 * @fn add_offer_options
 * @brief add DHCP OFFER options
 * @param optptr
 * @return optptr address
 * @section
 * **/
static uint8_t *dhcps_add_offer_options(uint8_t *optptr)
{
	ip4_addr_t ipaddr;

	ipaddr.addr = IN_ADDR_T(server_address).addr;

	*optptr++ = DHCP_OPTION_SUBNET_MASK;
	*optptr++ = 4;
	*optptr++ = 255;
	*optptr++ = 255;
	*optptr++ = 255;
	*optptr++ = 0;

	*optptr++ = DHCP_OPTION_ROUTER;
	*optptr++ = 4;
	*optptr++ = ip4_addr1(&ipaddr);
	*optptr++ = ip4_addr2(&ipaddr);
	*optptr++ = ip4_addr3(&ipaddr);
	*optptr++ = ip4_addr4(&ipaddr);

	*optptr++ = DHCP_OPTION_LEASE_TIME;
	*optptr++ = 4;
	*optptr++ = ((DHCPS_LEASE_TIMER * 60) >> 24) & 0xFF;
	*optptr++ = ((DHCPS_LEASE_TIMER * 60) >> 16) & 0xFF;
	*optptr++ = ((DHCPS_LEASE_TIMER * 60) >> 8) & 0xFF;
	*optptr++ = ((DHCPS_LEASE_TIMER * 60) >> 0) & 0xFF;

	*optptr++ = DHCP_OPTION_SERVER_ID;
	*optptr++ = 4;
	*optptr++ = ip4_addr1(&ipaddr);
	*optptr++ = ip4_addr2(&ipaddr);
	*optptr++ = ip4_addr3(&ipaddr);
	*optptr++ = ip4_addr4(&ipaddr);

	*optptr++ = DHCP_OPTION_BROADCAST;
	*optptr++ = 4;
	*optptr++ = ip4_addr1(&ipaddr);
	*optptr++ = ip4_addr2(&ipaddr);
	*optptr++ = ip4_addr3(&ipaddr);
	*optptr++ = 255;

	*optptr++ = DHCP_OPTION_DNS_SERVER;
	*optptr++ = 4;
	*optptr++ = 8;
	*optptr++ = 8;
	*optptr++ = 8;
	*optptr++ = 8;

	return optptr;
}

/**
 * @fn dhcps_add_end
 * @brief add DHCP_OPTION_END msg
 * @param optptr
 * @return optptr address
 * @section
 * **/
static uint8_t *dhcps_add_end(uint8_t *optptr)
{
	*optptr++ = DHCP_OPTION_END;
	return optptr;
}

/**
 * @fn dhcps_create_msg
 * @brief create DHCP messages
 * @param m
 * @return void
 * @section
 * **/
static void dhcps_create_msg(struct dhcps_msg *m)
{
	ip_addr_t client;

	IN_ADDR_T(client).addr = IN_ADDR_T(client_address).addr;

	m->op = DHCP_OFFER;
	m->htype = DHCP_HTYPE_ETH;
	m->hlen = 6;
	m->hops = 0;
	m->secs = 0;
#ifdef CONFIG_LWIP_DHCPS_UNICAST
	m->flags = htons(BOOTP_UNICAST);
#else
	m->flags = htons(BOOTP_BROADCAST);
#endif

	memcpy(m->yiaddr, &IN_ADDR_T(client).addr, sizeof(m->yiaddr));
	memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}

/**
 * @fn dhcps_send_msg
 * @brief send DHCP messages
 * @param m dhcp message format
 * @param type
 * @return DHCP state
 * @section
 * **/
static void dhcps_send_msg(struct dhcps_msg *m, u8_t msg_type)
{
	u8_t *end;
	u8_t *data;
	u16_t cnt = 0;
	u16_t i;

	struct pbuf *p, *q;

	dhcps_create_msg(m);

	end = dhcps_add_msg_type(&m->options[4], msg_type);

	if (msg_type != DHCP_NAK) {
		end = dhcps_add_offer_options(end);
	}

	end = dhcps_add_end(end);

	p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);

	if (p != NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_send_msg(): p->ref = %d\n", p->ref));
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_send_msg(): p->tot_len = %d\n", p->tot_len));
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_send_msg(): p->len = %d\n", p->len));

		q = p;
		while (q != NULL) {
			data = (u8_t *) q->payload;
			for (i = 0; i < (q->len); i++) {
				data[i] = ((u8_t *) m)[cnt++];
			}
			q = q->next;
		}
	} else {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_send_msg(): pbuf_alloc failed\n"));
		return;
	}

	ip_addr_t ip_temp = IPADDR4_INIT(0x0);
#ifdef CONFIG_LWIP_DHCPS_UNICAST
	memcpy(&(IN_ADDR_T(ip_temp).addr), m->yiaddr, sizeof(m->yiaddr));
#else
	ip4_addr_set(ip_2_ip4(&ip_temp), &(IN_ADDR_T(broadcast_dhcps)));
#endif

	if (udp_sendto(pcb_dhcps, p, &ip_temp, DHCP_CLIENT_PORT) != ERR_OK) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_send_msg(): udp_sendto failed\n"));
	}

	if (p->ref != 0) {
		pbuf_free(p);
	}
}

/**
 * @fn dhcps_parse_options
 * @brief parsing DHCP options
 * @param optptr dhcp message format
 * @param len
 * @return DHCP state
 * @section
 * **/
static uint8_t dhcps_parse_options(uint8_t *optptr, int16_t len)
{
	bool is_dhcp_parse_end = false;
	uint8_t *end = NULL;
	u16_t type = 0;
	ip_addr_t client;

	struct dhcps_state s;

	IN_ADDR_T(client).addr = IN_ADDR_T(client_address).addr;
	end = optptr + len;
	s.state = DHCPS_STATE_IDLE;

	while (optptr < end) {
		LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): *optptr = %d\n", *optptr));
		switch ((int16_t) *optptr) {
		case DHCP_OPTION_MESSAGE_TYPE:
			type = *(optptr + 2);
			break;
		case DHCP_OPTION_REQUESTED_IP:
			if (memcmp((char *)&IN_ADDR_T(client).addr, (char *)optptr + 2, 4) == 0) {
				LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): DHCP_OPTION_REQUESTED_IP = 0 ok\n"));
				s.state = DHCPS_STATE_ACK;
			} else {
				LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): DHCP_OPTION_REQUESTED_IP != 0 err\n"));
				s.state = DHCPS_STATE_NAK;
			}
			break;
		case DHCP_OPTION_END:
			is_dhcp_parse_end = true;
			break;
		}

		if (is_dhcp_parse_end) {
			break;
		}

		optptr += optptr[1] + 2;
	}

	switch (type) {
	case DHCP_DISCOVER:
		s.state = DHCPS_STATE_OFFER;
		LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): DHCPD_STATE_OFFER\n"));
		break;

	case DHCP_REQUEST:
		if (!(s.state == DHCPS_STATE_ACK || s.state == DHCPS_STATE_NAK)) {
			if (renew == true) {
				s.state = DHCPS_STATE_ACK;
			} else {
				s.state = DHCPS_STATE_NAK;
			}
			LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): DHCPD_STATE_NAK\n"));
		}
		break;

	case DHCP_DECLINE:
		s.state = DHCPS_STATE_IDLE;
		LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): DHCPD_STATE_IDLE\n"));
		break;

	case DHCP_RELEASE:
		s.state = DHCPS_STATE_RELEASE;
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps: DHCPD_STATE_IDLE\n"));
		break;
	}
	LWIP_DEBUGF(DHCP_DEBUG, ("parse_options(): return s.state = %d\n", s.state));
	return s.state;
}

/**
 * @fn dhcps_parse_msg
 * @brief parsing DHCP message
 * @param m dhcp message format
 * @param len
 * @return DHCP state
 * @section
 * **/
static int16_t dhcps_parse_msg(struct dhcps_msg *m, u16_t len)
{
	bool flag = false;
	int ret = 0;

	ip_addr_t addr_tmp;
	ip_addr_t first_address;

	struct dhcps_pool *pdhcps_pool = NULL;

	struct list_node *pnode = NULL;
	struct list_node *pback_node = NULL;

	if (memcmp(m->options, magic_cookie, sizeof(magic_cookie)) != 0) {
		LWIP_DEBUGF(DHCP_DEBUG, ("parse_msg() : cound not parse magic_cookie (ret %d)\n", ret));
		return ret;
	}

	LWIP_DEBUGF(DHCP_DEBUG, ("parse_msg(): dhcp msg len=%d\n", len));

	IN_ADDR_T(first_address).addr = IN_ADDR_T(dhcps_lease.start_ip).addr;
	IN_ADDR_T(client_address).addr = IN_ADDR_T(client_address_plus).addr;
	renew = false;

	for (pback_node = plist; pback_node != NULL; pback_node = pback_node->pnext) {
		pdhcps_pool = pback_node->pnode;
		if (memcmp(pdhcps_pool->mac, m->chaddr, sizeof(pdhcps_pool->mac)) == 0) {
			if (memcmp(&IN_ADDR_T(pdhcps_pool->ip).addr, m->ciaddr, sizeof(IN_ADDR_T(pdhcps_pool->ip).addr)) == 0) {
				renew = true;
			}
			IN_ADDR_T(client_address).addr = IN_ADDR_T(pdhcps_pool->ip).addr;
			pdhcps_pool->lease_timer = DHCPS_LEASE_TIMER;
			pnode = pback_node;

			goto POOL_CHECK;
		} else if (IN_ADDR_T(pdhcps_pool->ip).addr == IN_ADDR_T(client_address_plus).addr) {
			IN_ADDR_T(addr_tmp).addr = htonl(IN_ADDR_T(client_address_plus).addr);
			IN_ADDR_T(addr_tmp).addr++;
			IN_ADDR_T(client_address_plus).addr = htonl(IN_ADDR_T(addr_tmp).addr);
			IN_ADDR_T(client_address).addr = IN_ADDR_T(client_address_plus).addr;
		}

		if (flag == false) {	// search the fisrt unused ip
			if (IN_ADDR_T(first_address).addr < IN_ADDR_T(pdhcps_pool->ip).addr) {
				flag = true;
			} else {
				IN_ADDR_T(addr_tmp).addr = htonl(IN_ADDR_T(first_address).addr);
				IN_ADDR_T(addr_tmp).addr++;
				IN_ADDR_T(first_address).addr = htonl(IN_ADDR_T(addr_tmp).addr);
			}
		}
	}

	if (IN_ADDR_T(client_address_plus).addr > IN_ADDR_T(dhcps_lease.end_ip).addr) {
		IN_ADDR_T(client_address).addr = IN_ADDR_T(first_address).addr;
	}

	if (IN_ADDR_T(client_address).addr > IN_ADDR_T(dhcps_lease.end_ip).addr) {
		IN_ADDR_T(client_address_plus).addr = IN_ADDR_T(dhcps_lease.start_ip).addr;
		pdhcps_pool = NULL;
		pnode = NULL;
	} else {
		pdhcps_pool = (struct dhcps_pool *)mem_malloc(sizeof(struct dhcps_pool));
		if (pdhcps_pool == NULL) {
			return 0;
		}
		IN_ADDR_T(pdhcps_pool->ip).addr = IN_ADDR_T(client_address).addr;
		memcpy(pdhcps_pool->mac, m->chaddr, sizeof(pdhcps_pool->mac));
		pdhcps_pool->lease_timer = DHCPS_LEASE_TIMER;

#ifdef CONFIG_LWIP_DHCPS_UNICAST
		// update ARP table to send unicast
		struct eth_addr mac;
		memcpy(&mac.addr, pdhcps_pool->mac, ETH_HWADDR_LEN);
		etharp_add_static_entry(&(IN_ADDR_T(pdhcps_pool->ip)), &mac);
#endif

		pnode = (struct list_node *)mem_malloc(sizeof(struct list_node));
		if (pnode == NULL) {
			return 0;
		}
		pnode->pnode = pdhcps_pool;
		pnode->pnext = NULL;
		dhcps_node_insert_to_list(&plist, pnode);

		if (IN_ADDR_T(client_address).addr == IN_ADDR_T(dhcps_lease.end_ip).addr) {
			IN_ADDR_T(client_address_plus).addr = IN_ADDR_T(dhcps_lease.start_ip).addr;
		} else {
			IN_ADDR_T(addr_tmp).addr = htonl(IN_ADDR_T(client_address).addr);
			IN_ADDR_T(addr_tmp).addr++;
			IN_ADDR_T(client_address_plus).addr = htonl(IN_ADDR_T(addr_tmp).addr);
		}
	}

POOL_CHECK:
	if ((IN_ADDR_T(client_address).addr > IN_ADDR_T(dhcps_lease.end_ip).addr) || (ip_addr_isany(&client_address))) {
		if (pnode != NULL) {
			dhcps_node_remove_from_list(&plist, pnode);
			mem_free(pnode);
			pnode = NULL;
		}

		if (pdhcps_pool != NULL) {
			mem_free(pdhcps_pool);
			pdhcps_pool = NULL;
		}
		return DHCPS_STATE_NAK;
	}

	LWIP_DEBUGF(DHCP_DEBUG, ("Assigned client IP address %u.%u.%u.%u\n",
							 (unsigned char)((htonl(IN_ADDR_T(client_address)) >> 24) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(client_address)) >> 16) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(client_address)) >> 8) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(client_address)) >> 0) & 0xff)));

	ret = dhcps_parse_options(&m->options[4], len);

	if (ret == DHCPS_STATE_RELEASE) {
		if (pnode != NULL) {
			dhcps_node_remove_from_list(&plist, pnode);
			mem_free(pnode);
			pnode = NULL;
		}

		if (pdhcps_pool != NULL) {
			mem_free(pdhcps_pool);
			pdhcps_pool = NULL;
		}
		memset(&client_address, 0x0, sizeof(client_address));
	}

	return ret;
}

/**
 * @fn dhcps_handle_dhcp
 * @brief handle received DHCP message
 * @param arg
 * @param pcb
 * @param p
 * @param addr
 * @param port
 * @return DHCP state
 * @section
 * **/
static void dhcps_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	int16_t tlen = 0;
	u16_t dhcps_msg_cnt = 0;
	u8_t *p_dhcps_msg = NULL;
	u8_t *data = NULL;

	//struct netif *netif = (struct netif *)arg;
	struct dhcps_msg *pmsg_dhcps = NULL;
//ip4_addr4_16
	LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(pbuf = %p) from DHCP client %" U16_F ".%" U16_F ".%" U16_F ".%" U16_F " port %" U16_F "\n",
							 (void *)p,
							 ((char *)(&((IN_ADDR_T(*addr)).addr)))[0],
							 ((char *)(&((IN_ADDR_T(*addr)).addr)))[1],
							 ((char *)(&((IN_ADDR_T(*addr)).addr)))[2],
							 ((char *)(&((IN_ADDR_T(*addr)).addr)))[3],
							 port));
	LWIP_DEBUGF(DHCP_DEBUG, ("pbuf->len = %" U16_F "\n", p->len));
	LWIP_DEBUGF(DHCP_DEBUG, ("pbuf->tot_len = %" U16_F "\n", p->tot_len));

	pmsg_dhcps = (struct dhcps_msg *)mem_malloc(sizeof(struct dhcps_msg));

	if (pmsg_dhcps == NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp() : counld not allocate dhcp msg\n"));
		goto free_pbuf_and_return;
	}

	p_dhcps_msg = (u8_t *) pmsg_dhcps;
	tlen = p->tot_len;
	data = p->payload;

	LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): msg tot_len=%d, len=%d\n", tlen, p->len));

	memcpy(&p_dhcps_msg[dhcps_msg_cnt], data, p->len);
	dhcps_msg_cnt = p->len;

	if (p->next != NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): next msg tot_len=%d, len=%d\n", p->next->tot_len, p->next->len));

		data = p->next->payload;

		memcpy(&p_dhcps_msg[dhcps_msg_cnt], data, p->len);
		dhcps_msg_cnt += p->len;
	}

	LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): parse_msg\n"));

	switch (dhcps_parse_msg(pmsg_dhcps, tlen - 240)) {
	case DHCPS_STATE_OFFER:	//1
		LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): DHCPD_STATE_OFFER\n"));
		dhcps_send_msg(pmsg_dhcps, DHCP_OFFER);
		break;
	case DHCPS_STATE_ACK:		//3
		LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): DHCPD_STATE_ACK\n"));
		dhcps_send_msg(pmsg_dhcps, DHCP_ACK);

		if (g_dhcp_sta_joined) {
			LWIP_DEBUGF(DHCP_DEBUG, ("station join done: callback\n"));
			dhcp_node_s node;
			memcpy(&node.ipaddr, pmsg_dhcps->yiaddr, sizeof(pmsg_dhcps->yiaddr));
			memcpy(&node.macaddr, pmsg_dhcps->chaddr, 6);

			g_dhcp_sta_joined(DHCP_ACK_EVT, (void *)&node);
		}
		break;
	case DHCPS_STATE_NAK:		//4
		LWIP_DEBUGF(DHCP_DEBUG, ("handle_dhcp(): DHCPD_STATE_NAK\n"));
		dhcps_send_msg(pmsg_dhcps, DHCP_NAK);
		break;
	default:
		break;
	}

	mem_free(pmsg_dhcps);
	pmsg_dhcps = NULL;

free_pbuf_and_return:
	if (p != NULL) {
		pbuf_free(p);
	}
	return;
}

/**
 * @fn kill_oldest_dhcps_pool
 * @brief delete DHCP POOL
 * @param void
 * @return void
 * @section
 * **/
#if 0							/* Temporary disabled : Reason - Not used */
static void kill_oldest_dhcps_pool(void)
{
	struct list_node *pre = NULL;
	struct list_node *p = NULL;
	struct list_node *minpre = NULL;
	struct list_node *minp = NULL;

	struct dhcps_pool *pdhcps_pool = NULL;
	struct dhcps_pool *pmin_pool = NULL;

	pre = plist;
	p = pre->pnext;
	minpre = pre;
	minp = p;

	while (p != NULL) {
		pdhcps_pool = p->pnode;
		pmin_pool = minp->pnode;
		if (pdhcps_pool->lease_timer < pmin_pool->lease_timer) {
			minp = p;
			minpre = pre;
		}
		pre = p;
		p = p->pnext;
	}
	minpre->pnext = minp->pnext;
	free(minp->pnode);
	minp->pnode = NULL;
	free(minp);
	minp = NULL;
}
#endif

/**
 * @fn dhcps_register_cb
 * @brief register dhcp server connection callback
 * @param netif, callback
 * @return err_t
 * @section
 * **/
err_t dhcps_register_cb(dhcp_sta_joined_cb dhcp_join_cb)
{
	g_dhcp_sta_joined = NULL;
	if (dhcp_join_cb) {
		g_dhcp_sta_joined = dhcp_join_cb;
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_register_cb(): link callback\n"));
		return ERR_OK;
	}
	return ERR_ARG;
}

/**
 * @fn dhcps_start
 * @brief start DHCP server
 * @param info, callback
 * @return void
 * @section
 * **/
err_t dhcps_start(struct netif *netif)
{
	struct udp_pcb *dhcps;
	err_t result = ERR_OK;
	u32_t ipaddr_tmp;

	if (netif == NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("error: netif is null\n"));
		return ERR_MEM;
	}

	dhcps = netif->dhcps_pcb;
	LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start (netif=%p) %c%c%" U16_F "\n",
							 (void *)netif,
							 netif->name[0],
							 netif->name[1],
							 (u16_t) netif->num));

	/* check hwtype of the netif */
	if ((netif->flags & NETIF_FLAG_ETHARP) == 0) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start(): No ETHARP netif\n"));
		return ERR_ARG;
	}

	/* check MTU of the netif */
	if (netif->mtu < DHCP_MAX_MSG_LEN_MIN_REQUIRED) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start(): Cannot use this netif with DHCP: MTU is too small\n"));
		return ERR_MEM;
	}

	/* no DHCP server attached yet? */
	if (dhcps == NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start() : starting new DHCP server\n"));
	} else {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start() : re-starting DHCP server configuration\n"));
		if (netif->dhcps_pcb != NULL) {
			udp_remove(netif->dhcps_pcb);
		}
	}

	dhcps = udp_new();
	if (dhcps == NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start() : could not allocate DHCP server\n"));
		return ERR_MEM;
	}

	netif->dhcps_pcb = dhcps;

	pcb_dhcps = dhcps;

	if (IN_ADDR_T(netif->ip_addr).addr == 0) {
		IN_ADDR_T(netif->ip_addr).addr = inet_addr(LWIP_DHCPS_SERVER_IP);
	}

	if (IN_ADDR_T(netif->gw).addr == 0) {
		IN_ADDR_T(netif->gw).addr = inet_addr(LWIP_DHCPS_SERVER_IP);
	}

	server_address = netif->ip_addr;

	if (IN_ADDR_T(netif->netmask).addr == 0) {
		IN_ADDR_T(netif->netmask).addr = inet_addr(LWIP_DHCPS_SERVER_NETMASK);
	}

	ipaddr_tmp = htonl(IN_ADDR_T(netif->ip_addr).addr);
	IN_ADDR_T(broadcast_dhcps).addr = htonl(ipaddr_tmp | 0xff);

	IN_ADDR_T(dhcps_lease.start_ip).addr = htonl(ipaddr_tmp + 1);
	IN_ADDR_T(dhcps_lease.end_ip).addr = htonl(ipaddr_tmp + MAX_STATION_NUM);

	IN_ADDR_T(client_address_plus).addr = IN_ADDR_T(dhcps_lease.start_ip).addr;

	LWIP_DEBUGF(DHCP_DEBUG, ("Server IP address set %u.%u.%u.%u\n",
							 (unsigned char)((htonl(IN_ADDR_T(server_address).addr) >> 24) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(server_address).addr) >> 16) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(server_address).addr) >> 8) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(server_address).addr) >> 0) & 0xff)));

	LWIP_DEBUGF(DHCP_DEBUG, ("Broadcast IP address set %u.%u.%u.%u\n",
							 (unsigned char)((htonl(IN_ADDR_T(broadcast_dhcps).addr) >> 24) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(broadcast_dhcps).addr) >> 16) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(broadcast_dhcps).addr) >> 8) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(broadcast_dhcps).addr) >> 0) & 0xff)));

	LWIP_DEBUGF(DHCP_DEBUG, ("Start IP address set %u.%u.%u.%u\n",
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.start_ip).addr) >> 24) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.start_ip).addr) >> 16) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.start_ip).addr) >> 8) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.start_ip).addr) >> 0) & 0xff)));

	LWIP_DEBUGF(DHCP_DEBUG, ("End IP address set %u.%u.%u.%u\n",
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.end_ip).addr) >> 24) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.end_ip).addr) >> 16) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.end_ip).addr) >> 8) & 0xff),
							 (unsigned char)((htonl(IN_ADDR_T(dhcps_lease.end_ip).addr) >> 0) & 0xff)));

	ip_set_option(pcb_dhcps, SOF_BROADCAST);

	result = udp_bind(pcb_dhcps, IP_ADDR_ANY, DHCP_SERVER_PORT);
	LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start(): udp_bind ret %d\n", result));

	udp_recv(pcb_dhcps, dhcps_recv, NULL);

	if (result != ERR_OK) {
		LWIP_DEBUGF(DHCP_DEBUG, ("dhcps_start(): could not start dhcp server\n"));
	}

	return result;

}

/**
 * @fn dhcps_stop
 * @brief stop DHCP server
 * @param void
 * @return void
 * @section
 * **/
void dhcps_stop(struct netif *netif)
{
	struct list_node *pnode = NULL;
	struct list_node *pback_node = NULL;

	if (netif == NULL || pcb_dhcps == NULL) {
		LWIP_DEBUGF(DHCP_DEBUG, ("error: netif null\n"));
		return;
	}

	udp_disconnect(pcb_dhcps);

	if (netif->dhcps_pcb != NULL) {
		udp_remove(netif->dhcps_pcb);
		netif->dhcps_pcb = NULL;
	}

	pnode = plist;

	while (pnode != NULL) {
		pback_node = pnode;
		pnode = pback_node->pnext;
		dhcps_node_remove_from_list(&plist, pback_node);
		mem_free(pback_node->pnode);
		pback_node->pnode = NULL;
		mem_free(pback_node);
		pback_node = NULL;
	}
}

#endif							/* LWIP_DHCPS */
#endif							/* LWIP_DHCP */
