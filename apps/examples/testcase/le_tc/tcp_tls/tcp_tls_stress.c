/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>

#include <wifi_manager/wifi_manager.h>
#include <stress_tool/st_perf.h>

#include "mbedtls/config.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"
#include "tc_common.h"
#ifdef CONFIG_NETUTILS_NTPCLIENT
#include <protocols/ntpclient.h>
#include <unistd.h>
#endif

/*
 * Macro
 */
#define WM_AP_SSID         CONFIG_EXAMPLES_TESTCASE_TCP_TLS_SSID
#define WM_AP_PASSWORD     CONFIG_EXAMPLES_TESTCASE_TCP_TLS_PASSPHRASE
#define WM_AP_AUTH         CONFIG_EXAMPLES_TESTCASE_TCP_TLS_AUTHENTICATION
#define WM_AP_CRYPTO       CONFIG_EXAMPLES_TESTCASE_TCP_TLS_CRYPTO
#define TCP_TLS_TRIAL      CONFIG_EXAMPLES_TESTCASE_TCP_TLS_TRIAL

#define WM_TEST_SIGNAL										\
	do {													\
		pthread_mutex_lock(&g_wm_mutex);					\
		printf("T%d send signal\n", getpid());	\
		pthread_cond_signal(&g_wm_cond);					\
		pthread_mutex_unlock(&g_wm_mutex);					\
	} while (0)

#define WM_TEST_WAIT								\
	do {											\
		pthread_mutex_lock(&g_wm_mutex);			\
		printf(" T%d wait signal\n", getpid());		\
		pthread_cond_wait(&g_wm_cond, &g_wm_mutex);	\
		pthread_mutex_unlock(&g_wm_mutex);			\
	} while (0)

/*
 *
 */

typedef struct {
	char *ca_certificate;
	unsigned int ca_certificate_length;
	unsigned int ip4_addr;
	unsigned int port_num;
} tls_config;

typedef struct {
	mbedtls_ssl_context *tls_ctx;
	mbedtls_ssl_config *conf;
	mbedtls_net_context *server_fd;
	mbedtls_entropy_context *entropy;
	mbedtls_ctr_drbg_context *ctr_drbg;
	mbedtls_x509_crt *cacert;
	mbedtls_timing_delay_context *timer;
} mbedtls_config;

typedef enum {
	TCP_TLS_FAIL = -1,
	TCP_TLS_SUCCESS,
	TCP_TLS_INVALID_ARGS,
} tcp_tls_result_e;

/*
 * Callbacks
 */
static void wm_sta_connected(wifi_manager_result_e);
static void wm_sta_disconnected(wifi_manager_disconnect_e);

/*
 * Global
 */
 static const char google_ca_cert[] = "-----BEGIN CERTIFICATE-----\n"
            "MIIDVDCCAjygAwIBAgIDAjRWMA0GCSqGSIb3DQEBBQUAMEIxCzAJBgNVBAYTAlVT\n"
            "MRYwFAYDVQQKEw1HZW9UcnVzdCBJbmMuMRswGQYDVQQDExJHZW9UcnVzdCBHbG9i\n"
            "YWwgQ0EwHhcNMDIwNTIxMDQwMDAwWhcNMjIwNTIxMDQwMDAwWjBCMQswCQYDVQQG\n"
            "EwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjEbMBkGA1UEAxMSR2VvVHJ1c3Qg\n"
            "R2xvYmFsIENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2swYYzD9\n"
            "9BcjGlZ+W988bDjkcbd4kdS8odhM+KhDtgPpTSEHCIjaWC9mOSm9BXiLnTjoBbdq\n"
            "fnGk5sRgprDvgOSJKA+eJdbtg/OtppHHmMlCGDUUna2YRpIuT8rxh0PBFpVXLVDv\n"
            "iS2Aelet8u5fa9IAjbkU+BQVNdnARqN7csiRv8lVK83Qlz6cJmTM386DGXHKTubU\n"
            "1XupGc1V3sjs0l44U+VcT4wt/lAjNvxm5suOpDkZALeVAjmRCw7+OC7RHQWa9k0+\n"
            "bw8HHa8sHo9gOeL6NlMTOdReJivbPagUvTLrGAMoUgRx5aszPeE4uwc2hGKceeoW\n"
            "MPRfwCvocWvk+QIDAQABo1MwUTAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTA\n"
            "ephojYn7qwVkDBF9qn1luMrMTjAfBgNVHSMEGDAWgBTAephojYn7qwVkDBF9qn1l\n"
            "uMrMTjANBgkqhkiG9w0BAQUFAAOCAQEANeMpauUvXVSOKVCUn5kaFOSPeCpilKIn\n"
            "Z57QzxpeR+nBsqTP3UEaBU6bS+5Kb1VSsyShNwrrZHYqLizz/Tt1kL/6cdjHPTfS\n"
            "tQWVYrmm3ok9Nns4d0iXrKYgjy6myQzCsplFAMfOEVEiIuCl6rYVSAlk6l5PdPcF\n"
            "PseKUgzbFbS9bZvlxrFUaKnjaZC2mqUPuLk/IH2uSrW4nOQdtqvmlKXBx4Ot2/Un\n"
            "hw4EbNX/3aBd7YdStysVAq45pmp06drE57xNNB6pXE0zX5IJL4hmXXeXxx12E6nV\n"
            "5fEWCRE11azbJHFwLJhWC9kXtNHjUStedejV0NxPNO3CBWaAocvmMw==\n"
            "-----END CERTIFICATE-----";

static const char facebook_ca_cert[] =
		"-----BEGIN CERTIFICATE-----\n"
		"MIIEsTCCA5mgAwIBAgIQBOHnpNxc8vNtwCtCuF0VnzANBgkqhkiG9w0BAQsFADBs\n"
		"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
		"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
		"ZSBFViBSb290IENBMB4XDTEzMTAyMjEyMDAwMFoXDTI4MTAyMjEyMDAwMFowcDEL\n"
		"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
		"LmRpZ2ljZXJ0LmNvbTEvMC0GA1UEAxMmRGlnaUNlcnQgU0hBMiBIaWdoIEFzc3Vy\n"
		"YW5jZSBTZXJ2ZXIgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2\n"
		"4C/CJAbIbQRf1+8KZAayfSImZRauQkCbztyfn3YHPsMwVYcZuU+UDlqUH1VWtMIC\n"
		"Kq/QmO4LQNfE0DtyyBSe75CxEamu0si4QzrZCwvV1ZX1QK/IHe1NnF9Xt4ZQaJn1\n"
		"itrSxwUfqJfJ3KSxgoQtxq2lnMcZgqaFD15EWCo3j/018QsIJzJa9buLnqS9UdAn\n"
		"4t07QjOjBSjEuyjMmqwrIw14xnvmXnG3Sj4I+4G3FhahnSMSTeXXkgisdaScus0X\n"
		"sh5ENWV/UyU50RwKmmMbGZJ0aAo3wsJSSMs5WqK24V3B3aAguCGikyZvFEohQcft\n"
		"bZvySC/zA/WiaJJTL17jAgMBAAGjggFJMIIBRTASBgNVHRMBAf8ECDAGAQH/AgEA\n"
		"MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n"
		"NAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2Vy\n"
		"dC5jb20wSwYDVR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybDQuZGlnaWNlcnQuY29t\n"
		"L0RpZ2lDZXJ0SGlnaEFzc3VyYW5jZUVWUm9vdENBLmNybDA9BgNVHSAENjA0MDIG\n"
		"BFUdIAAwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQ\n"
		"UzAdBgNVHQ4EFgQUUWj/kK8CB3U8zNllZGKiErhZcjswHwYDVR0jBBgwFoAUsT7D\n"
		"aQP4v0cB1JgmGggC72NkK8MwDQYJKoZIhvcNAQELBQADggEBABiKlYkD5m3fXPwd\n"
		"aOpKj4PWUS+Na0QWnqxj9dJubISZi6qBcYRb7TROsLd5kinMLYBq8I4g4Xmk/gNH\n"
		"E+r1hspZcX30BJZr01lYPf7TMSVcGDiEo+afgv2MW5gxTs14nhr9hctJqvIni5ly\n"
		"/D6q1UEL2tU2ob8cbkdJf17ZSHwD2f2LSaCYJkJA69aSEaRkCldUxPUd1gJea6zu\n"
		"xICaEnL6VpPX/78whQYwvwt/Tv9XBZ0k7YXDK/umdaisLRbvfXknsuvCnQsH6qqF\n"
		"0wGjIChBWUMo0oHjqvbsezt3tkBigAVBRQHvFwY+3sAzm2fTYS5yh+Rp/BIAV0Ae\n"
		"cPUeybQ=\n"
		"-----END CERTIFICATE-----";

unsigned char SIMPLE_GET_REQUEST[] = "GET / HTTP/1.1\r\n"
									"Host: \r\n"
									"Connection: close\r\n"
									"\r\n";

static wifi_manager_cb_s g_wifi_callbacks = {
	wm_sta_connected,
	wm_sta_disconnected,
	NULL,
	NULL,
	NULL,
};

static pthread_mutex_t g_wm_mutex = PTHREAD_MUTEX_INITIALIZER;;
static pthread_cond_t g_wm_cond = PTHREAD_COND_INITIALIZER;

static mbedtls_config mbedtls_c;

/*
 * Callback
 */
void wm_sta_connected(wifi_manager_result_e res)
{
	printf(" T%d --> %s res(%d)\n", getpid(), __FUNCTION__, res);
	WM_TEST_SIGNAL;
}

void wm_sta_disconnected(wifi_manager_disconnect_e disconn)
{
	printf(" T%d --> %s\n", getpid(), __FUNCTION__);
	WM_TEST_SIGNAL;
}

#ifdef CONFIG_NETUTILS_NTPCLIENT
static void ntp_link_error(void)
{
	printf("ntp_link_error() callback is called\n");
}
#endif

/*
 * Internal APIs
 */
tcp_tls_result_e tcp_tls_hostname_to_ip4(char *hostname, unsigned int *ip4_address)
{
	if (hostname == NULL || ip4_address == NULL) {
		return TCP_TLS_INVALID_ARGS;
	}

	struct hostent *shost;

	shost = gethostbyname((const char *)hostname);
	if (shost == NULL) {
		ndbg("gethostbyname fail %d\n", shost);
		return TCP_TLS_FAIL;
	}

	memcpy(ip4_address, shost->h_addr, sizeof(in_addr_t));

	return TCP_TLS_SUCCESS;
}

tcp_tls_result_e tcp_tls_config_get(tls_config * config)
{
	tcp_tls_result_e ret = TCP_TLS_SUCCESS;
	unsigned int ip4_addr = 0;
#if 0
	ret = tcp_tls_hostname_to_ip4("www.facebook.com", &ip4_addr);
	if (ret != TCP_TLS_SUCCESS) {
		ndbg("tcp_tls_hostname_to_ip4 fails!\n");
		return ret;
	}
	config->ca_certificate = (char *) facebook_ca_cert;
	config->ca_certificate_length = sizeof(facebook_ca_cert);
	config->ip4_addr = ip4_addr;
	config->port_num = 443;
#else
	ret = tcp_tls_hostname_to_ip4("www.google.com", &ip4_addr);
	if (ret != TCP_TLS_SUCCESS) {
		ndbg("tcp_tls_hostname_to_ip4 fails!\n");
		return ret;
	}
	config->ca_certificate = (char *) google_ca_cert;
	config->ca_certificate_length = sizeof(google_ca_cert);
	config->ip4_addr = ip4_addr;
	config->port_num = 443;
#endif
	return ret;
}

static void wm_get_apinfo(wifi_manager_ap_config_s *apconfig)
{
	strncpy(apconfig->ssid, WM_AP_SSID, sizeof(WM_AP_SSID));
	apconfig->ssid_length = strlen(WM_AP_SSID);
	apconfig->ap_auth_type = WM_AP_AUTH;
	if (WM_AP_AUTH != WIFI_MANAGER_AUTH_OPEN) {
		strncpy(apconfig->passphrase, WM_AP_PASSWORD, sizeof(WM_AP_PASSWORD));
		apconfig->passphrase_length = strlen(WM_AP_PASSWORD);
		apconfig->ap_crypto_type = WM_AP_CRYPTO;
	}
}

static tcp_tls_result_e tcp_tls_create(mbedtls_config *tls)
{
	tcp_tls_result_e result = TCP_TLS_SUCCESS;
	int ret = 0;
	const char *pers = "tcptls_client";

	if (tls == NULL) {
		result = TCP_TLS_INVALID_ARGS;
		return result;
	}

#if defined(CONFIG_TLS_WITH_HW_ACCEL)
	ret = see_init();
	if (ret != 0) {
		printf("failed : see_init returned %d\n\n", ret);
		goto error;
	};
#endif

	tls->tls_ctx = malloc(sizeof(mbedtls_ssl_context));
	tls->ctr_drbg = malloc(sizeof(mbedtls_ctr_drbg_context));
	tls->entropy = malloc(sizeof(mbedtls_entropy_context));
	tls->server_fd = malloc(sizeof(mbedtls_net_context));
	tls->conf = malloc(sizeof(mbedtls_ssl_config));
	tls->cacert = malloc(sizeof(mbedtls_x509_crt));
	tls->timer = malloc(sizeof(mbedtls_timing_delay_context));

	mbedtls_ssl_init(tls->tls_ctx);
	mbedtls_ctr_drbg_init(tls->ctr_drbg);
	mbedtls_entropy_init(tls->entropy);
	mbedtls_net_init(tls->server_fd);
	mbedtls_ssl_config_init(tls->conf);
	mbedtls_x509_crt_init(tls->cacert);

	ret = mbedtls_ctr_drbg_seed(tls->ctr_drbg, mbedtls_entropy_func,
			tls->entropy, (const unsigned char *) pers,
			strlen(pers));
	if (ret != 0) {
		printf("failed : mbedtls_ctr_drbg_seed returned %d\n", ret);
		goto error;
	}

	ret = mbedtls_ssl_config_defaults(tls->conf,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		printf("failed : mbedtls_ssl_config_defaults returned %d\n\n", ret);
		goto error;
	}

	mbedtls_ssl_conf_rng(tls->conf, mbedtls_ctr_drbg_random, tls->ctr_drbg);
	// even though verifying certificate is fail. optional option proceed tls session
	mbedtls_ssl_conf_authmode(tls->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	//mbedtls_ssl_conf_authmode(tls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);

	mbedtls_ssl_set_timer_cb(tls->tls_ctx, tls->timer,
			mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	printf("TCP_TLS create succeeds: %d\n", result);

	return result;

error:
	result = TCP_TLS_FAIL;
	printf("TCP_TLS create fails\n");
	return result;
}

static tcp_tls_result_e tcp_tls_connect(mbedtls_config *tls, tls_config *tls_conf)
{
	tcp_tls_result_e result = TCP_TLS_SUCCESS;
	int ret = 0;
	char ip_addr[20] = { 0x00, };
	char port[6] = { 0x00, };

	if (tls == NULL || tls_conf == NULL) {
		result = TCP_TLS_INVALID_ARGS;
		return result;
	}

	ret = mbedtls_x509_crt_parse(tls->cacert,
			(const unsigned char *) tls_conf->ca_certificate,
			tls_conf->ca_certificate_length);
	if (ret != 0) {
		printf("failed : mbedtls_x509_crt_parse returned %d\n\n", ret);
		goto error;
	}

	sprintf(ip_addr, "%u.%u.%u.%u", (unsigned char)(tls_conf->ip4_addr >> 0),
			(unsigned char)(tls_conf->ip4_addr >> 8),
			(unsigned char)(tls_conf->ip4_addr >> 16),
			(unsigned char)(tls_conf->ip4_addr >> 24));
	sprintf(port, "%u", tls_conf->port_num);

	printf("  . Connecting to tcp/%s/%s\n", ip_addr, port);
	fflush(stdout);
	ret = mbedtls_net_connect(tls->server_fd, ip_addr, port,
			MBEDTLS_NET_PROTO_TCP);
	if (ret != 0) {
		printf("failed : mbedtls_net_connect returned %d\n\n", ret);
		goto error;
	}
	mbedtls_ssl_conf_ca_chain(tls->conf, tls->cacert, NULL);

	ret = mbedtls_ssl_setup(tls->tls_ctx, tls->conf);
	if (ret != 0) {
		printf("failed : mbedtls_ssl_setup returned %d\n\n", ret);
		goto error;
	}

	mbedtls_ssl_set_bio(tls->tls_ctx, tls->server_fd, mbedtls_net_send,
			NULL, mbedtls_net_recv_timeout);
	//mbedtls_ssl_conf_authmode(tls->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

	while ((ret = mbedtls_ssl_handshake(tls->tls_ctx)) != 0) {
		unsigned int flags;

		flags = mbedtls_ssl_get_verify_result(tls->tls_ctx);
		if (flags != 0) {
			char vrfy_buf[512];

			printf(" failed\n");

			mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

			printf("%s\n", vrfy_buf);
		} else
			printf(" ok\n");

		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			printf("failed : mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
			if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
				printf("	 Unable to verify the server's certificate. Either it is invalid,\n	 or you didn't set ca_file or ca_path to an appropriate value.\n	 Alternatively, you may want to use auth_mode=optional for testing purposes.\n\n");

			goto error;
		}
	}

	printf("TCP_TLS connect succeeds: %d\n", result);

	return result;

error:
	result = TCP_TLS_FAIL;
	printf("TCP_TLS connect fails\n");
	return result;
}

static tcp_tls_result_e tcp_tls_destroy(mbedtls_config *tls)
{
	tcp_tls_result_e result = TCP_TLS_SUCCESS;
	if (tls == NULL) {
		result = TCP_TLS_INVALID_ARGS;
		printf("TCP_TLS destroy fails: %d\n", result);
		return result;
	}

	if (tls->server_fd->fd > 0) {
		mbedtls_net_free(tls->server_fd);
	}
	mbedtls_x509_crt_free(tls->cacert);
	mbedtls_ssl_free(tls->tls_ctx);
	mbedtls_ssl_config_free(tls->conf);
	mbedtls_ctr_drbg_free(tls->ctr_drbg);
	mbedtls_entropy_free(tls->entropy);

	free(tls->server_fd);
	free(tls->cacert);
	free(tls->tls_ctx);
	free(tls->conf);
	free(tls->ctr_drbg);
	free(tls->entropy);
	free(tls->timer);

#if defined(CONFIG_TLS_WITH_HW_ACCEL)
	int ret = 0;

	ret = see_free();
	if (ret != 0) {
		printf("failed : see_free returned %d\n\n", ret);
		result = TCP_TLS_FAIL;
		return result;
	};
#endif
	printf("TCP_TLS destroy succeeds: %d\n", result);
	return result;
}

static tcp_tls_result_e tcp_tls_read(mbedtls_config *tls, unsigned char *buffer, unsigned int buffer_size, unsigned int *read_count, int time_out)
{
	tcp_tls_result_e result = TCP_TLS_FAIL;
	int ret = 0;

	if (tls == NULL || buffer == NULL || buffer_size <= 0 || read_count == NULL || time_out < -1) {
		result = TCP_TLS_INVALID_ARGS;
		printf("TCP_TLS read fails: %d\n", result);
		return result;
	}

	/* Set read_timeout */
	if (tls->tls_ctx->conf != NULL) {
		mbedtls_ssl_conf_read_timeout((mbedtls_ssl_config *)tls->tls_ctx->conf, time_out);
	}
	do {
		ret = mbedtls_ssl_read(tls->tls_ctx, buffer, buffer_size);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}
		break;
	} while (1);
	if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		printf("TCP_TLS read fails: %d\n", result);
		return result;
	}

	if (ret == MBEDTLS_ERR_SSL_TIMEOUT) {
		printf("TCP_TLS read fails: %d\n", result);
		return result;
	}

	if (ret < 0) {
		printf("failed : mbedtls_ssl_read returned %d\n\n", ret);
		return result;
	}

	if (ret == 0) {
		printf("\n\nEOF\n\n");
		*read_count = 0;
		result = TCP_TLS_SUCCESS;
		printf("TCP_TLS read succeeds: %d\n", result);
		return result;
	}
	*read_count = ret;
	printf("read contents: count %d text %s\n", *read_count, buffer);
	result = TCP_TLS_SUCCESS;
	printf("TCP_TLS read succeeds: %d\n", result);
	return result;
}

static tcp_tls_result_e tcp_tls_write(mbedtls_config *tls, unsigned char *data, unsigned int data_size, unsigned int *write_count)
{
	tcp_tls_result_e result = TCP_TLS_SUCCESS;
	int ret = 0;

	if (tls == NULL || data == NULL || data_size <= 0 || write_count == NULL) {
		result = TCP_TLS_INVALID_ARGS;
		printf("TCP_TLS write fails: %d\n", result);
		return result;
	}

	while ((ret = mbedtls_ssl_write(tls->tls_ctx, data, data_size)) <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			printf("failed : mbedtls_ssl_write returned %d\n\n", ret);
			result = TCP_TLS_FAIL;
			printf("TCP_TLS write fails: %d\n", result);
			return result;
		}
	}
	*write_count = ret;
	result = TCP_TLS_SUCCESS;
	printf("TCP_TLS write succeeds: %d\n", result);

	return result;
}

#ifdef CONFIG_NETUTILS_NTPCLIENT
void *tcp_tls_sync_time_ntp(void *data)
{
	if (ntpc_get_status() != NTP_RUNNING) {
		struct timespec pre_tp;
		clock_gettime(CLOCK_REALTIME, &pre_tp);

		struct timespec init_tp;
		init_tp.tv_sec = 0;
		init_tp.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &init_tp);

		if (ntpc_start(NULL, 0, 0, ntp_link_error) < 0) {
			printf("ntpc_start() failed\n");
		} else {
			printf("ntpc_start() OK\n");
			int time_sync = 0;

			while (1) {
				struct timespec sync_tp;
				clock_gettime(CLOCK_REALTIME, &sync_tp);
				if ((init_tp.tv_sec + 1000) < sync_tp.tv_sec) {
					time_sync = 1;
					break;
				} else if ((init_tp.tv_sec + *((int*) data)) < sync_tp.tv_sec) {
					break;
				}
				usleep(100000);
			}
			if (time_sync) {
				printf("ntpc_time sync done\n");
			} else {
				printf("ntpc_time sync fail\n");
				clock_settime(CLOCK_REALTIME, &pre_tp);
			}
		}
	} else {
		printf("ntpc already runnig\n");
	}

	if (ntpc_stop() < 0) {
		printf("ntcp_stop() failed.\n");
	}
	printf("ntpc_stop() OK.\n");

	return (void*)0;
}
#endif

static int tcp_tls_signal_init(void)
{
	int res = pthread_mutex_init(&g_wm_mutex, NULL);
	if (res != 0) {
		printf(" Pthread mutex init fail(%d) (%d)\n", res, errno);
		return -1;
	}
	res = pthread_cond_init(&g_wm_cond, NULL);
	if (res != 0) {
		printf(" Conditional mutex init fail(%d) (%d)\n", res, errno);
		return -1;
	}
	return 0;
}

static void tcp_tls_signal_deinit(void)
{
	pthread_mutex_destroy(&g_wm_mutex);
	pthread_cond_destroy(&g_wm_cond);
}
/*
 * tcp tls create
 */
TEST_SETUP(tls_create)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_TEARDOWN(tls_create)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_destroy(&mbedtls_c));
	ST_END_TEST;
}

TEST_F(tls_create)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_create(&mbedtls_c));
	ST_END_TEST;
}

/*
 * tcp tls destroy
 */
TEST_SETUP(tls_destroy)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_create(&mbedtls_c));
	ST_END_TEST;
}

TEST_TEARDOWN(tls_destroy)
{
	ST_START_TEST;
	ST_END_TEST;
}

TEST_F(tls_destroy)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_destroy(&mbedtls_c));
	ST_END_TEST;
}

/*
 * tcp tls connect
 */
TEST_SETUP(tls_connect)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_create(&mbedtls_c));
	ST_END_TEST;
}

TEST_TEARDOWN(tls_connect)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_destroy(&mbedtls_c));
	ST_END_TEST;
}

TEST_F(tls_connect)
{
	ST_START_TEST;
	tls_config tls_c;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_config_get(&tls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_connect(&mbedtls_c, &tls_c));
	ST_END_TEST;
}

/*
 * tcp tls send
 */
TEST_SETUP(tls_send)
{
	ST_START_TEST;
	tls_config tls_c;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_create(&mbedtls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_config_get(&tls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_connect(&mbedtls_c, &tls_c));
	ST_END_TEST;
}

TEST_TEARDOWN(tls_send)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_destroy(&mbedtls_c));
	ST_END_TEST;
}

TEST_F(tls_send)
{
	ST_START_TEST;
	unsigned int write_count = 0;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_write(&mbedtls_c, SIMPLE_GET_REQUEST, strlen((void*)SIMPLE_GET_REQUEST), &write_count));
	ST_END_TEST;
}

/*
 * tcp tls recv
 */
TEST_SETUP(tls_recv)
{
	ST_START_TEST;
	tls_config tls_c;
	unsigned int write_count = 0;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_create(&mbedtls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_config_get(&tls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_connect(&mbedtls_c, &tls_c));
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_write(&mbedtls_c, SIMPLE_GET_REQUEST, strlen((void*)SIMPLE_GET_REQUEST), &write_count));
	ST_END_TEST;
}

TEST_TEARDOWN(tls_recv)
{
	ST_START_TEST;
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_destroy(&mbedtls_c));
	ST_END_TEST;
}

TEST_F(tls_recv)
{
	ST_START_TEST;
	unsigned int read_count = 0;
	unsigned char read_buffer[320];
	ST_EXPECT_EQ(TCP_TLS_SUCCESS, tcp_tls_read(&mbedtls_c, read_buffer, sizeof(read_buffer), &read_count, 1000));
	ST_END_TEST;
}


/*
 * tcp tls run
 */


void tcp_tls_stress_test(int argc, FAR char *argv[])
{
	ST_SET_PACK(tcp_tls);

	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "create TCP-TLS", tls_create);
	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "recv TCP-TLS", tls_recv);
	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "send TCP-TLS", tls_send);
	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "connect TCP-TLS", tls_connect);
	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "destroy TCP-TLS", tls_destroy);
	ST_SET_SMOKE(tcp_tls, TCP_TLS_TRIAL, 0, "create TCP-TLS", tls_create);

	ST_RUN_TEST(tcp_tls);
	ST_RESULT_TEST(tcp_tls);
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int tc_tcp_tls_main(int argc, char *argv[])
#endif
{
	if (testcase_state_handler(TC_START, "TCPTLS STRESS TC") == ERROR) {
		return ERROR;
	}
	int res = tcp_tls_signal_init();
	if (res < 0) {
		return -1;
	}

	wifi_manager_result_e ret = WIFI_MANAGER_FAIL;
	ret = wifi_manager_init(&g_wifi_callbacks);
	if (ret == WIFI_MANAGER_FAIL) {
		printf("[WM] init failure\n");
		return -1;
	}
	wifi_manager_ap_config_s apconfig;
	wm_get_apinfo(&apconfig);
	ret = wifi_manager_connect_ap(&apconfig);
	if (ret == WIFI_MANAGER_FAIL) {
		printf("[WM] init failure\n");
		return -1;
	}
	WM_TEST_WAIT;

#ifdef CONFIG_NETUTILS_NTPCLIENT
	pthread_t p_ntp;
	int thr_id;
	int guarantee_time = 10;
	thr_id = pthread_create(&p_ntp, NULL, tcp_tls_sync_time_ntp, (void *)&guarantee_time);
	if (thr_id < 0) {
		printf("NTPclient thread open is failed\n");
		return -1;
	}
	pthread_join(p_ntp, (void *)&res);
	if (res < 0) {
		printf("NTPclient fails to make a synchronization\n");
		return -1;
	}
#endif
	tcp_tls_stress_test(argc, argv);

	ret = wifi_manager_disconnect_ap();
	if (ret == WIFI_MANAGER_FAIL) {
		printf("[WM] init failure\n");
		return -1;
	}
	WM_TEST_WAIT;

	ret = wifi_manager_deinit();
	if (ret == WIFI_MANAGER_FAIL) {
		printf("[WM] init failure\n");
		return -1;
	}

	tcp_tls_signal_deinit();
	(void)testcase_state_handler(TC_END, "TCPTLS STRESS TC");
	return 0;
}
