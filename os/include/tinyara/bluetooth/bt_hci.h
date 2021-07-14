/****************************************************************************
 * include/tinyara/bluetooth/bt_hci.h
 * Bluetooth Host Control Interface definitions.
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Ported from the Intel/Zephyr arduino101_firmware_source-v1.tar package
 * where the code was released with a compatible 3-clause BSD license:
 *
 *   Copyright (c) 2016, Intel Corporation
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_TINYARA_BLUETOOTH_BT_HCI_H
#define __INCLUDE_TINYARA_BLUETOOTH_BT_HCI_H 1

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BT_ADDR_LE_PUBLIC    0x00
#define BT_ADDR_LE_RANDOM    0x01
#define BT_ADDR_LE_PUBLIC_ID 0x02
#define BT_ADDR_LE_RANDOM_ID 0x03

#define BT_ADDR_ANY    (&(bt_addr_t) { { 0, 0, 0, 0, 0, 0 } })
#define BT_ADDR_LE_ANY (&(bt_addr_le_t) { 0, { 0, 0, 0, 0, 0, 0 } })
#define BT_ADDR_NONE   (&(bt_addr_t) { \
			{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } })

/* HCI Error Codes */
#define BT_HCI_ERR_SUCCESS                      0x00
#define BT_HCI_ERR_UNKNOWN_CMD                  0x01
#define BT_HCI_ERR_UNKNOWN_CONN_ID              0x02
#define BT_HCI_ERR_HW_FAILURE                   0x03
#define BT_HCI_ERR_PAGE_TIMEOUT                 0x04
#define BT_HCI_ERR_AUTHENTICATION_FAIL          0x05
#define BT_HCI_ERR_PIN_OR_KEY_MISSING           0x06
#define BT_HCI_ERR_MEM_CAPACITY_EXCEEDED        0x07
#define BT_HCI_ERR_CONN_TIMEOUT                 0x08
#define BT_HCI_ERR_CONN_LIMIT_EXCEEDED          0x09
#define BT_HCI_ERR_SYNC_CONN_LIMIT_EXCEEDED     0x0a
#define BT_HCI_ERR_CONN_ALREADY_EXISTS          0x0b
#define BT_HCI_ERR_CMD_DISALLOWED               0x0c
#define BT_HCI_ERR_INSUFFICIENT_RESOURCES       0x0d
#define BT_HCI_ERR_INSUFFICIENT_SECURITY        0x0e
#define BT_HCI_ERR_BD_ADDR_UNACCEPTABLE         0x0f
#define BT_HCI_ERR_CONN_ACCEPT_TIMEOUT          0x10
#define BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL     0x11
#define BT_HCI_ERR_INVALID_PARAM                0x12
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN        0x13
#define BT_HCI_ERR_REMOTE_LOW_RESOURCES         0x14
#define BT_HCI_ERR_REMOTE_POWER_OFF             0x15
#define BT_HCI_ERR_LOCALHOST_TERM_CONN          0x16
#define BT_HCI_ERR_PAIRING_NOT_ALLOWED          0x18
#define BT_HCI_ERR_UNSUPP_REMOTE_FEATURE        0x1a
#define BT_HCI_ERR_INVALID_LL_PARAM             0x1e
#define BT_HCI_ERR_UNSPECIFIED                  0x1f
#define BT_HCI_ERR_UNSUPP_LL_PARAM_VAL          0x20
#define BT_HCI_ERR_LL_RESP_TIMEOUT              0x22
#define BT_HCI_ERR_LL_PROC_COLLISION            0x23
#define BT_HCI_ERR_INSTANT_PASSED               0x28
#define BT_HCI_ERR_PAIRING_NOT_SUPPORTED        0x29
#define BT_HCI_ERR_DIFF_TRANS_COLLISION         0x2a
#define BT_HCI_ERR_UNACCEPT_CONN_PARAMS         0x3b
#define BT_HCI_ERR_ADV_TIMEOUT                  0x3c
#define BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL         0x3d
#define BT_HCI_ERR_CONN_FAIL_TO_ESTAB           0x3e

/* EIR/AD definitions */

#define BT_EIR_FLAGS             0x01	/* AD flags */
#define BT_EIR_UUID16_SOME       0x02	/* 16-bit UUID, more available */
#define BT_EIR_UUID16_ALL        0x03	/* 16-bit UUID, all listed */
#define BT_EIR_UUID32_SOME       0x04	/* 32-bit UUID, more available */
#define BT_EIR_UUID32_ALL        0x05	/* 32-bit UUID, all listed */
#define BT_EIR_UUID128_SOME      0x06	/* 128-bit UUID, more available */
#define BT_EIR_UUID128_ALL       0x07	/* 128-bit UUID, all listed */
#define BT_EIR_NAME_COMPLETE     0x09	/* Complete name */
#define BT_EIR_TX_POWER          0x0a	/* Tx Power */
#define BT_EIR_SOLICIT16         0x14	/* Solicit UUIDs, 16-bit */
#define BT_EIR_SOLICIT128        0x15	/* Solicit UUIDs, 128-bit */
#define BT_EIR_SVC_DATA16        0x16	/* Service data, 16-bit UUID */
#define BT_EIR_GAP_APPEARANCE    0x19	/* GAP appearance */
#define BT_EIR_SOLICIT32         0x1f	/* Solicit UUIDs, 32-bit */
#define BT_EIR_SVC_DATA32        0x20	/* Service data, 32-bit UUID */
#define BT_EIR_SVC_DATA128       0x21	/* Service data, 128-bit UUID */
#define BT_EIR_MANUFACTURER_DATA 0xff	/* Manufacturer Specific Data */

#define BT_LE_AD_GENERAL         0x02	/* General Discoverable */
#define BT_LE_AD_NO_BREDR        0x04	/* BR/EDR not supported */

#define bt_acl_handle(h)         ((h) & 0x0fff)

/* LMP features */

#define BT_LMP_NO_BREDR          0x20
#define BT_LMP_LE                0x40

/* LE features */

#define BT_HCI_LE_ENCRYPTION     0x01

/* OpCode Group Fields */

#define BT_OGF_LINK_CTRL         0x01
#define BT_OGF_BASEBAND          0x03
#define BT_OGF_INFO              0x04
#define BT_OGF_LE                0x08

/* Construct OpCode from OGF and OCF */

#define BT_OP(ogf, ocf)          ((ocf) | ((ogf) << 10))

#define BT_HCI_OP_DISCONNECT                  BT_OP(BT_OGF_LINK_CTRL, 0x0006)
#define BT_HCI_OP_SET_EVENT_MASK              BT_OP(BT_OGF_BASEBAND, 0x0001)
#define BT_HCI_OP_RESET                       BT_OP(BT_OGF_BASEBAND, 0x0003)
#define BT_HCI_OP_SET_CTL_TO_HOST_FLOW        BT_OP(BT_OGF_BASEBAND, 0x0031)
#define BT_HCI_OP_HOST_BUFFER_SIZE            BT_OP(BT_OGF_BASEBAND, 0x0033)
#define BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS  BT_OP(BT_OGF_BASEBAND, 0x0035)
#define BT_HCI_OP_LE_WRITE_LE_HOST_SUPP       BT_OP(BT_OGF_BASEBAND, 0x006d)
#define BT_HCI_OP_READ_LOCAL_VERSION_INFO     BT_OP(BT_OGF_INFO, 0x0001)
#define BT_HCI_OP_READ_LOCAL_FEATURES         BT_OP(BT_OGF_INFO, 0x0003)
#define BT_HCI_OP_READ_BUFFER_SIZE            BT_OP(BT_OGF_INFO, 0x0005)
#define BT_HCI_OP_READ_BD_ADDR                BT_OP(BT_OGF_INFO, 0x0009)
#define BT_HCI_OP_LE_READ_BUFFER_SIZE         BT_OP(BT_OGF_LE, 0x0002)
#define BT_HCI_OP_LE_READ_LOCAL_FEATURES      BT_OP(BT_OGF_LE, 0x0003)
#define BT_HCI_OP_LE_SET_RANDOM_ADDRESS       BT_OP(BT_OGF_LE, 0x0005)

/* Local Name */
#define BT_HCI_OP_WRITE_LOCAL_NAME            BT_OP(BT_OGF_BASEBAND, 0x0013)
#define BT_HCI_OP_READ_LOCAL_NAME             BT_OP(BT_OGF_BASEBAND, 0x0014)

/* Advertising types */

#define BT_LE_ADV_IND                         0x00
#define BT_LE_ADV_DIRECT_IND                  0x01
#define BT_LE_ADV_SCAN_IND                    0x02
#define BT_LE_ADV_NONCONN_IND                 0x03
#define BT_LE_ADV_DIRECT_IND_LOW_DUTY         0x04
/* Needed in advertising reports when getting info about */
#define BT_LE_ADV_SCAN_RSP                    0x04

#define BT_HCI_OP_LE_SET_ADV_PARAMETERS       BT_OP(BT_OGF_LE, 0x0006)
#define BT_HCI_OP_LE_SET_ADV_DATA             BT_OP(BT_OGF_LE, 0x0008)
#define BT_HCI_OP_LE_SET_SCAN_RSP_DATA        BT_OP(BT_OGF_LE, 0x0009)
#define BT_HCI_OP_LE_SET_ADV_ENABLE           BT_OP(BT_OGF_LE, 0x000a)

/* Scan types */

#define BT_HCI_OP_LE_SET_SCAN_PARAMS          BT_OP(BT_OGF_LE, 0x000b)
#define BT_LE_SCAN_PASSIVE                  0x00
#define BT_LE_SCAN_ACTIVE                   0x01
#define BT_HCI_OP_LE_SET_SCAN_ENABLE          BT_OP(BT_OGF_LE, 0x000c)
#define BT_LE_SCAN_DISABLE                  0x00
#define BT_LE_SCAN_ENABLE                   0x01
#define BT_LE_SCAN_FILTER_DUP_DISABLE       0x00
#define BT_LE_SCAN_FILTER_DUP_ENABLE        0x01
#define BT_HCI_OP_LE_CREATE_CONN              BT_OP(BT_OGF_LE, 0x000d)
#define BT_HCI_OP_LE_CREATE_CONN_CANCEL       BT_OP(BT_OGF_LE, 0x000e)
#define BT_HCI_OP_LE_CONN_UPDATE              BT_OP(BT_OGF_LE, 0x0013)
#define BT_HCI_OP_LE_ENCRYPT                  BT_OP(BT_OGF_LE, 0x0017)
#define BT_HCI_OP_LE_RAND                     BT_OP(BT_OGF_LE, 0x0018)
#define BT_HCI_OP_LE_START_ENCRYPTION         BT_OP(BT_OGF_LE, 0x0019)
#define BT_HCI_OP_LE_LTK_REQ_REPLY            BT_OP(BT_OGF_LE, 0x001a)
#define BT_HCI_OP_LE_LTK_REQ_NEG_REPLY        BT_OP(BT_OGF_LE, 0x001b)

/* Event definitions */

#define BT_HCI_EVT_DISCONN_COMPLETE           0x05
#define BT_HCI_EVT_ENCRYPT_CHANGE             0x08
#define BT_HCI_EVT_CMD_COMPLETE               0x0e
#define BT_HCI_EVT_CMD_STATUS                 0x0f
#define BT_HCI_EVT_NUM_COMPLETED_PACKETS      0x13
#define BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE 0x30
#define BT_HCI_EVT_LE_META_EVENT              0x3e
#define BT_HCI_EVT_LE_CONN_COMPLETE           0x01
#define BT_HCI_EVT_LE_ENH_CONN_COMPLETE       0x0a
#define BT_HCI_ROLE_MASTER                  0x00
#define BT_HCI_ROLE_SLAVE                   0x01
#define BT_HCI_EVT_LE_ADVERTISING_REPORT      0x02
#define BT_HCI_EVT_LE_LTK_REQUEST             0x05

#define CONFIG_BT_DEVICE_NAME_MAX		65
#define CONFIG_BT_DEVICE_NAME			"TizenRT"
#define BT_HCI_LE_ADV_DISABLE                   0x00
#define BT_HCI_LE_ADV_ENABLE                    0x01
#define CONFIG_BT_ID_MAX                        2
#define HCI_MAX_NAME_LENGTH                     65

#define BT_ADDR_IS_RPA(a)     (((a)->val[5] & 0xc0) == 0x40)
#define BT_ADDR_IS_NRPA(a)    (((a)->val[5] & 0xc0) == 0x00)
#define BT_ADDR_IS_STATIC(a)  (((a)->val[5] & 0xc0) == 0xc0)

#define BT_ADDR_SET_RPA(a)    ((a)->val[5] = (((a)->val[5] & 0x3f) | 0x40))

#define BT_ADDR_SET_NRPA(a)   ((a)->val[5] &= 0x3f)
#define BT_ADDR_SET_STATIC(a) ((a)->val[5] |= 0xc0)

/* EIR/AD data type definitions */
#define BT_DATA_FLAGS                   0x01	/* AD flags */
#define BT_DATA_UUID16_SOME             0x02	/* 16-bit UUID, more available */
#define BT_DATA_UUID16_ALL              0x03	/* 16-bit UUID, all listed */
#define BT_DATA_UUID32_SOME             0x04	/* 32-bit UUID, more available */
#define BT_DATA_UUID32_ALL              0x05	/* 32-bit UUID, all listed */
#define BT_DATA_UUID128_SOME            0x06	/* 128-bit UUID, more available */
#define BT_DATA_UUID128_ALL             0x07	/* 128-bit UUID, all listed */
#define BT_DATA_NAME_SHORTENED          0x08	/* Shortened name */
#define BT_DATA_NAME_COMPLETE           0x09	/* Complete name */
#define BT_DATA_TX_POWER                0x0a	/* Tx Power */
#define BT_DATA_SM_TK_VALUE             0x10	/* Security Manager TK Value */
#define BT_DATA_SM_OOB_FLAGS            0x11	/* Security Manager OOB Flags */
#define BT_DATA_SOLICIT16               0x14	/* Solicit UUIDs, 16-bit */
#define BT_DATA_SOLICIT128              0x15	/* Solicit UUIDs, 128-bit */
#define BT_DATA_SVC_DATA16              0x16	/* Service data, 16-bit UUID */
#define BT_DATA_GAP_APPEARANCE          0x19	/* GAP appearance */
#define BT_DATA_LE_BT_DEVICE_ADDRESS    0x1b	/* LE Bluetooth Device Address */
#define BT_DATA_LE_ROLE                 0x1c	/* LE Role */
#define BT_DATA_SOLICIT32               0x1f	/* Solicit UUIDs, 32-bit */
#define BT_DATA_SVC_DATA32              0x20	/* Service data, 32-bit UUID */
#define BT_DATA_SVC_DATA128             0x21	/* Service data, 128-bit UUID */
#define BT_DATA_LE_SC_CONFIRM_VALUE     0x22	/* LE SC Confirmation Value */
#define BT_DATA_LE_SC_RANDOM_VALUE      0x23	/* LE SC Random Value */
#define BT_DATA_URI                     0x24	/* URI */
#define BT_DATA_MESH_PROV               0x29	/* Mesh Provisioning PDU */
#define BT_DATA_MESH_MESSAGE            0x2a	/* Mesh Networking PDU */
#define BT_DATA_MESH_BEACON             0x2b	/* Mesh Beacon */

#define BT_DATA_MANUFACTURER_DATA       0xff	/* Manufacturer Specific Data */

#ifdef BIT
#undef BIT
#endif
#define BIT(x) ((uint32_t)1 << (x))

/* HCI version from Assigned Numbers */
#define BT_HCI_VERSION_1_0B                     0
#define BT_HCI_VERSION_1_1                      1
#define BT_HCI_VERSION_1_2                      2
#define BT_HCI_VERSION_2_0                      3
#define BT_HCI_VERSION_2_1                      4
#define BT_HCI_VERSION_3_0                      5
#define BT_HCI_VERSION_4_0                      6
#define BT_HCI_VERSION_4_1                      7
#define BT_HCI_VERSION_4_2                      8
#define BT_HCI_VERSION_5_0                      9
#define BT_HCI_VERSION_5_1                      10

/* Defined GAP timers */
#define BT_GAP_SCAN_FAST_INTERVAL               0x0060	/* 60 ms    */
#define BT_GAP_SCAN_FAST_WINDOW                 0x0030	/* 30 ms    */
#define BT_GAP_SCAN_SLOW_INTERVAL_1             0x0800	/* 1.28 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_1               0x0012	/* 11.25 ms */
#define BT_GAP_SCAN_SLOW_INTERVAL_2             0x1000	/* 2.56 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_2               0x0012	/* 11.25 ms */
#define BT_GAP_ADV_FAST_INT_MIN_1               0x0030	/* 30 ms    */
#define BT_GAP_ADV_FAST_INT_MAX_1               0x0060	/* 60 ms    */
#define BT_GAP_ADV_FAST_INT_MIN_2               0x00a0	/* 100 ms   */
#define BT_GAP_ADV_FAST_INT_MAX_2               0x00f0	/* 150 ms   */
#define BT_GAP_ADV_SLOW_INT_MIN                 0x0640	/* 1 s      */
#define BT_GAP_ADV_SLOW_INT_MAX                 0x0780	/* 1.2 s    */
#define BT_GAP_INIT_CONN_INT_MIN                0x0018	/* 30 ms    */
#define BT_GAP_INIT_CONN_INT_MAX                0x0028	/* 50 ms    */

#define CONFIG_BT_BACKGROUND_SCAN_INTERVAL      0x0010	/*defaulut value */
#define CONFIG_BT_BACKGROUND_SCAN_WINDOW        0x0010	/*default value */

#define BT_HCI_LE_SCAN_PASSIVE					0x00
#define BT_HCI_LE_SCAN_ACTIVE					0x01

#define BT_HCI_LE_SCAN_FILTER_DUP_DISABLE		0x00
#define BT_HCI_LE_SCAN_FILTER_DUP_ENABLE		0x01

/* LE States */
#define BT_LE_STATES_SLAVE_CONN_ADV(states)     (states & 0x0000004000000000)

/* LE features */
#define BT_LE_FEAT_BIT_ENC                      0
#define BT_LE_FEAT_BIT_CONN_PARAM_REQ           1
#define BT_LE_FEAT_BIT_EXT_REJ_IND              2
#define BT_LE_FEAT_BIT_SLAVE_FEAT_REQ           3
#define BT_LE_FEAT_BIT_PING                     4
#define BT_LE_FEAT_BIT_DLE                      5
#define BT_LE_FEAT_BIT_PRIVACY                  6
#define BT_LE_FEAT_BIT_EXT_SCAN                 7
#define BT_LE_FEAT_BIT_PHY_2M                   8
#define BT_LE_FEAT_BIT_SMI_TX                   9
#define BT_LE_FEAT_BIT_SMI_RX                   10
#define BT_LE_FEAT_BIT_PHY_CODED                11
#define BT_LE_FEAT_BIT_ADV_EXT                  12
#define BT_LE_FEAT_BIT_ADV_PER                  13
#define BT_LE_FEAT_BIT_CHAN_SEL_ALGO_2          14
#define BT_LE_FEAT_BIT_PWR_CLASS_1              15
#define BT_LE_FEAT_BIT_MIN_USED_CHAN_PROC       16

#define BT_LE_FEAT_TEST(feat, n)                (feat[(n) >> 3] & \
						BIT((n) & 7))
#define BT_FEAT_LE_ENCR(feat)                   BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_ENC)
#define BT_FEAT_LE_CONN_PARAM_REQ_PROC(feat)    BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_CONN_PARAM_REQ)
#define BT_FEAT_LE_SLAVE_FEATURE_XCHG(feat)     BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_SLAVE_FEAT_REQ)
#define BT_FEAT_LE_DLE(feat)                    BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_DLE)
#define BT_FEAT_LE_PHY_2M(feat)                 BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_PHY_2M)
#define BT_FEAT_LE_PHY_CODED(feat)              BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_PHY_CODED)
#define BT_FEAT_LE_PRIVACY(feat)                BT_LE_FEAT_TEST(feat, \
						BT_LE_FEAT_BIT_PRIVACY)

#define LE_CONN_MIN_INTERVAL         0x0028
#define LE_CONN_MAX_INTERVAL         0x0038
#define LE_CONN_LATENCY              0x0000
#define LE_CONN_TIMEOUT              0x002a

/* Supported Commands */
#define BT_CMD_TEST(cmd, octet, bit)            (cmd[octet] & BIT(bit))
#define BT_CMD_LE_STATES(cmd)                   BT_CMD_TEST(cmd, 28, 3)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct bt_addr_s {
	uint8_t val[6];
} bt_addr_t;

typedef struct bt_addr_le_s {
	uint8_t type;
	uint8_t val[6];
} bt_addr_le_t;

struct bt_hci_evt_hdr_s {
	uint8_t evt;
	uint8_t len;
} packed_struct;

struct bt_hci_acl_hdr_s {
	uint16_t handle;
	uint16_t len;
} packed_struct;

struct bt_hci_cmd_hdr_s {
	uint16_t opcode;
	uint8_t param_len;
} packed_struct;

struct bt_hci_cp_disconnect_s {
	uint16_t handle;
	uint8_t reason;
} packed_struct;

struct bt_hci_cp_set_event_mask_s {
	uint8_t events[8];
} packed_struct;

struct bt_hci_cp_host_buffer_size_s {
	uint16_t acl_mtu;
	uint8_t sco_mtu;
	uint16_t acl_pkts;
	uint16_t sco_pkts;
} packed_struct;

struct bt_hci_handle_count_s {
	uint16_t handle;
	uint16_t count;
} packed_struct;

struct bt_hci_cp_host_num_completed_packets_s {
	uint8_t num_handles;
	struct bt_hci_handle_count_s h[0];
} packed_struct;

struct bt_hci_cp_write_le_host_supp_s {
	uint8_t le;
	uint8_t simul;
} packed_struct;

struct bt_hci_rp_read_local_version_info_s {
	uint8_t status;
	uint8_t hci_version;
	uint16_t hci_revision;
	uint8_t lmp_version;
	uint16_t manufacturer;
	uint16_t lmp_subversion;
} packed_struct;

struct bt_hci_rp_read_local_features_s {
	uint8_t status;
	uint8_t features[8];
} packed_struct;

struct bt_hci_rp_read_buffer_size_s {
	uint8_t status;
	uint16_t acl_max_len;
	uint8_t sco_max_len;
	uint16_t acl_max_num;
	uint16_t sco_max_num;
} packed_struct;

struct bt_hci_rp_read_bd_addr_s {
	uint8_t status;
	bt_addr_t bdaddr;
} packed_struct;

struct bt_hci_rp_le_read_buffer_size_s {
	uint8_t status;
	uint16_t le_max_len;
	uint8_t le_max_num;
} packed_struct;

struct bt_hci_rp_le_read_local_features_s {
	uint8_t status;
	uint8_t features[8];
} packed_struct;

struct bt_hci_cp_le_set_adv_parameters_s {
	uint16_t min_interval;
	uint16_t max_interval;
	uint8_t type;
	uint8_t own_addr_type;
	bt_addr_le_t direct_addr;
	uint8_t channel_map;
	uint8_t filter_policy;
} packed_struct;

struct bt_hci_cp_le_set_adv_data_s {
	uint8_t len;
	uint8_t data[31];
} packed_struct;

struct bt_hci_cp_le_set_scan_rsp_data_s {
	uint8_t len;
	uint8_t data[31];
} packed_struct;

struct bt_hci_cp_le_set_adv_enable_s {
	uint8_t enable;
} packed_struct;

struct bt_hci_cp_le_set_scan_params_s {
	uint8_t scan_type;
	uint16_t interval;
	uint16_t window;
	uint8_t addr_type;
	uint8_t filter_policy;
} packed_struct;

struct bt_hci_cp_le_set_scan_enable_s {
	uint8_t enable;
	uint8_t filter_dup;
} packed_struct;

struct bt_hci_cp_le_create_conn_s {
	uint16_t scan_interval;
	uint16_t scan_window;
	uint8_t filter_policy;
	bt_addr_le_t peer_addr;
	uint8_t own_addr_type;
	uint16_t conn_interval_min;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint16_t supervision_timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} packed_struct;

struct hci_cp_le_conn_update_s {
	uint16_t handle;
	uint16_t conn_interval_min;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint16_t supervision_timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} packed_struct;

struct bt_hci_cp_le_encrypt_s {
	uint8_t key[16];
	uint8_t plaintext[16];
} packed_struct;

struct bt_hci_rp_le_encrypt_s {
	uint8_t status;
	uint8_t enc_data[16];
} packed_struct;

struct bt_hci_rp_le_rand_s {
	uint8_t status;
	uint8_t rand[8];
} packed_struct;

struct bt_hci_cp_le_start_encryption_s {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
	uint8_t ltk[16];
} packed_struct;

struct bt_hci_cp_le_ltk_req_reply_s {
	uint16_t handle;
	uint8_t ltk[16];
} packed_struct;

struct bt_hci_cp_le_ltk_req_neg_reply_s {
	uint16_t handle;
} packed_struct;

struct bt_hci_cp_le_set_random_address_s {
	bt_addr_t bdaddr;
} packed_struct;

struct hci_cp_write_local_name_s {
	uint8_t local_name[HCI_MAX_NAME_LENGTH];
} packed_struct;

struct hci_rp_read_local_name_s {
	uint8_t status;
	uint8_t local_name[HCI_MAX_NAME_LENGTH];
} packed_struct;

#define BT_HCI_OP_LE_READ_REMOTE_FEATURES     BT_OP(BT_OGF_LE, 0x0016)
struct bt_hci_cp_le_read_remote_features_s {
	uint16_t handle;
} packed_struct;

#define BT_HCI_OP_READ_SUPPORTED_COMMANDS     BT_OP(BT_OGF_INFO, 0x0002)
struct bt_hci_rp_read_supported_commands_s {
	uint8_t status;
	uint8_t commands[64];
} packed_struct;

#define BT_HCI_OP_LE_READ_SUPP_STATES         BT_OP(BT_OGF_LE, 0x001c)
struct bt_hci_rp_le_read_supp_states_s {
	uint8_t status;
	uint8_t le_states[8];
} packed_struct;

#define BT_HCI_OP_LE_SET_DATA_LEN             BT_OP(BT_OGF_LE, 0x0022)
struct bt_hci_cp_le_set_data_len_s {
	uint16_t handle;
	uint16_t tx_octets;
	uint16_t tx_time;
} packed_struct;

struct bt_hci_rp_le_set_data_len_s {
	uint8_t status;
	uint16_t handle;
} packed_struct;

#define BT_HCI_OP_LE_READ_MAX_DATA_LEN        BT_OP(BT_OGF_LE, 0x002f)
struct bt_hci_rp_le_read_max_data_len_s {
	uint8_t status;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
} packed_struct;

#define BT_HCI_LE_PHY_1M                        0x01
#define BT_HCI_LE_PHY_2M                        0x02
#define BT_HCI_LE_PHY_CODED                     0x03

#define BT_HCI_OP_LE_READ_PHY                   BT_OP(BT_OGF_LE, 0x0030)
struct bt_hci_cp_le_read_phy_s {
	uint16_t handle;
} packed_struct;

struct bt_hci_rp_le_read_phy_s {
	uint8_t status;
	uint16_t handle;
	uint8_t tx_phy;
	uint8_t rx_phy;
} packed_struct;

#define BT_HCI_LE_PHY_TX_ANY                    BIT(0)
#define BT_HCI_LE_PHY_RX_ANY                    BIT(1)

#define BT_HCI_LE_PHY_PREFER_1M                 BIT(0)
#define BT_HCI_LE_PHY_PREFER_2M                 BIT(1)
#define BT_HCI_LE_PHY_PREFER_CODED              BIT(2)

#define BT_HCI_OP_LE_SET_DEFAULT_PHY            BT_OP(BT_OGF_LE, 0x0031)
struct bt_hci_cp_le_set_default_phy_s {
	uint8_t all_phys;
	uint8_t tx_phys;
	uint8_t rx_phys;
} packed_struct;

#define BT_HCI_LE_PHY_CODED_ANY                 0x00
#define BT_HCI_LE_PHY_CODED_S2                  0x01
#define BT_HCI_LE_PHY_CODED_S8                  0x02

#define BT_HCI_OP_LE_SET_PHY                    BT_OP(BT_OGF_LE, 0x0032)
struct bt_hci_cp_le_set_phy_s {
	uint16_t handle;
	uint8_t all_phys;
	uint8_t tx_phys;
	uint8_t rx_phys;
	uint16_t phy_opts;
} packed_struct;

/* Event definitions */

struct bt_hci_evt_disconn_complete_s {
	uint8_t status;
	uint16_t handle;
	uint8_t reason;
} packed_struct;

struct bt_hci_evt_encrypt_change_s {
	uint8_t status;
	uint16_t handle;
	uint8_t encrypt;
} packed_struct;

struct hci_evt_cmd_complete_s {
	uint8_t ncmd;
	uint16_t opcode;
} packed_struct;

struct bt_hci_evt_cmd_status_s {
	uint8_t status;
	uint8_t ncmd;
	uint16_t opcode;
} packed_struct;

struct bt_hci_evt_num_completed_packets_s {
	uint8_t num_handles;
	struct bt_hci_handle_count_s h[0];
} packed_struct;

struct bt_hci_evt_encrypt_key_refresh_complete_s {
	uint8_t status;
	uint16_t handle;
} packed_struct;

struct bt_hci_evt_le_meta_event_s {
	uint8_t subevent;
} packed_struct;

struct bt_hci_evt_le_conn_complete_s {
	uint8_t status;
	uint16_t handle;
	uint8_t role;
	bt_addr_le_t peer_addr;
	uint16_t interval;
	uint16_t latency;
	uint16_t supv_timeout;
	uint8_t clock_accuracy;
} packed_struct;

struct bt_hci_ev_le_advertising_info_s {
	uint8_t evt_type;
	bt_addr_le_t addr;
	uint8_t length;
	uint8_t data[0];
} packed_struct;

struct bt_hci_evt_le_ltk_request_s {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
} packed_struct;

struct bt_hci_evt_le_enh_conn_complete_s {
	uint8_t status;
	uint16_t handle;
	uint8_t role;
	bt_addr_le_t peer_addr;
	bt_addr_t local_rpa;
	bt_addr_t peer_rpa;
	uint16_t interval;
	uint16_t latency;
	uint16_t supv_timeout;
	uint8_t clock_accuracy;
} packed_struct;

#define BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE       0x04
struct bt_hci_evt_le_remote_feat_complete_s {
	uint8_t status;
	uint16_t handle;
	uint8_t features[8];
} packed_struct;

#endif							/* __INCLUDE_TINYARA_BLUETOOTH_BT_HCI_H */
