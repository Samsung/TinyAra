#ifndef _TIZENRT_WIRELESS_WIFI_H__
#define _TIZENRT_WIRELESS_WIFI_H__

#define TRWIFI_MACADDR_LEN			  6
#define TRWIFI_MACADDR_STR_LEN		  17
#define TRWIFI_SSID_LEN				  32
#define TRWIFI_PASSPHRASE_LEN		  64

/*  if serialization is failed then driver should let pass the fail event to applications*/
#define TRWIFI_POST_SCANEVENT(dev, evt, scanlist)						\
	do {																\
		if (evt == LWNL_EVT_SCAN_DONE) {									\
			uint8_t *buffer = NULL;										\
			int lwnl_res = trwifi_serialize_scaninfo(&buffer, scanlist); \
			if (lwnl_res < 0) {											\
				trwifi_post_event(dev, LWNL_EVT_SCAN_FAILED, NULL, 0);		\
			} else {													\
				trwifi_post_event(dev, LWNL_EVT_SCAN_DONE, buffer, lwnl_res); \
				kmm_free(buffer);										\
			}															\
		}																\
	} while (0)

typedef enum {
	TRWIFI_FAIL = -1,
	TRWIFI_SUCCESS,
	TRWIFI_INVALID_ARGS,
	TRWIFI_TIMEOUT,
	TRWIFI_BUSY,
	TRWIFI_FILE_ERROR,
	TRWIFI_ALREADY_CONNECTED,
	TRWIFI_NOT_SUPPORTED,
} trwifi_result_e;

typedef enum {
	TRWIFI_IEEE_80211_LEGACY,			  /**<	IEEE 802.11a/g/b		  */
	TRWIFI_IEEE_80211_A,				  /**<	IEEE 802.11a			  */
	TRWIFI_IEEE_80211_B,				  /**<	IEEE 802.11b			  */
	TRWIFI_IEEE_80211_G,				  /**<	IEEE 802.11g			  */
	TRWIFI_IEEE_80211_N,				  /**<	IEEE 802.11n			  */
	TRWIFI_IEEE_80211_AC,				  /**<	IEEE 802.11ac			  */
	TRWIFI_MODE_NOT_SUPPORTED,			  /**<	Driver does not report	  */
} trwifi_standard_type_e;

typedef enum {
	TRWIFI_AUTH_OPEN,					 /**<  open mode								 */
	TRWIFI_AUTH_WEP_SHARED,				 /**<  use shared key (wep key)					 */
	TRWIFI_AUTH_WPA_PSK,				 /**<  WPA_PSK mode								 */
	TRWIFI_AUTH_WPA2_PSK,				 /**<  WPA2_PSK mode							 */
	TRWIFI_AUTH_WPA3_PSK,				 /**<  WPA3_PSK mode							 */
	TRWIFI_AUTH_WPA_AND_WPA2_PSK,		 /**<  WPA_PSK and WPA_PSK mixed mode			 */
	TRWIFI_AUTH_WPA_PSK_ENT,			 /**<  Enterprise WPA_PSK mode					 */
	TRWIFI_AUTH_WPA2_PSK_ENT,			 /**<  Enterprise WPA2_PSK mode					 */
	TRWIFI_AUTH_WPA_AND_WPA2_PSK_ENT,	 /**<  Enterprise WPA_PSK and WPA_PSK mixed mode */
	TRWIFI_AUTH_IBSS_OPEN,				 /**<  IBSS ad-hoc mode						     */
	TRWIFI_AUTH_WPS,					 /**<  WPS mode									 */
	TRWIFI_AUTH_UNKNOWN,				 /**<  unknown type								 */
} trwifi_ap_auth_type_e;

typedef enum {
	TRWIFI_CRYPTO_NONE,					 /**<  none encryption							 */
	TRWIFI_CRYPTO_WEP_64,				 /**<  WEP encryption wep-40					 */
	TRWIFI_CRYPTO_WEP_128,				 /**<  WEP encryption wep-104					 */
	TRWIFI_CRYPTO_AES,					 /**<  AES encryption							 */
	TRWIFI_CRYPTO_TKIP,					 /**<  TKIP encryption							 */
	TRWIFI_CRYPTO_TKIP_AND_AES,			 /**<  TKIP and AES mixed encryption			 */
	TRWIFI_CRYPTO_AES_ENT,				 /**<  Enterprise AES encryption				 */
	TRWIFI_CRYPTO_TKIP_ENT,				 /**<  Enterprise TKIP encryption				 */
	TRWIFI_CRYPTO_TKIP_AND_AES_ENT,		 /**<  Enterprise TKIP and AES mixed encryption	 */
	TRWIFI_CRYPTO_UNKNOWN,				 /**<  unknown encryption						 */
} trwifi_ap_crypto_type_e;

typedef enum {
	TRWIFI_DISCONNECTED,			/**<  wifi is disconnected	*/
	TRWIFI_CONNECTED,			/**<  connected				*/
	TRWIFI_SOFTAP_MODE,			/**<  soft ap mode			*/
} trwifi_status_e;

typedef enum {
	LWNL_REQ_WIFI_INIT,
	LWNL_REQ_WIFI_DEINIT,
	LWNL_REQ_WIFI_SCANAP,
	LWNL_REQ_WIFI_GETINFO,
	LWNL_REQ_WIFI_SETAUTOCONNECT,
	LWNL_REQ_WIFI_STARTSTA,
	LWNL_REQ_WIFI_CONNECTAP,
	LWNL_REQ_WIFI_DISCONNECTAP,
	LWNL_REQ_WIFI_STARTSOFTAP,
	LWNL_REQ_WIFI_STOPSOFTAP,
	LWNL_REQ_WIFI_IOCTL,
	LWNL_REQ_WIFI_UNKNOWN
} lwnl_req_wifi;

typedef enum {
	LWNL_EVT_STA_CONNECTED,
	LWNL_EVT_STA_CONNECT_FAILED,
	LWNL_EVT_STA_DISCONNECTED,
	LWNL_EVT_SOFTAP_STA_JOINED,
	LWNL_EVT_SOFTAP_STA_LEFT,
	LWNL_EVT_SCAN_DONE,
	LWNL_EVT_SCAN_FAILED,
	LWNL_EVT_EXIT,
	LWNL_EVT_UNKNOWN,
} lwnl_cb_wifi;

typedef struct {
	char ssid[TRWIFI_SSID_LEN + 1];				 /**<  Service Set Identification		  */
	unsigned int ssid_length;						/**<  Service Set Identification Length	 */
	char passphrase[TRWIFI_PASSPHRASE_LEN + 1];	 /**<  ap passphrase(password)			  */
	unsigned int passphrase_length;					/**<  ap passphrase length				 */
	trwifi_ap_auth_type_e ap_auth_type;			 /**<  @ref trwifi_ap_auth_type		   */
	trwifi_ap_crypto_type_e ap_crypto_type;		 /**<  @ref trwifi_ap_crypto_type	   */
} trwifi_ap_config_s;

typedef struct {
	unsigned int channel;								   /**<	 Radio channel that the AP beacon was received on		*/
	char ssid[TRWIFI_SSID_LEN + 1];						/**<  Service Set Identification (i.e. Name of Access Point) */
	unsigned int ssid_length;							   /**<	 The length of Service Set Identification				*/
	unsigned char bssid[TRWIFI_MACADDR_STR_LEN + 1];	/**<  MAC address (xx:xx:xx:xx:xx:xx) of Access Point		 */
	unsigned int max_rate;								   /**<	 Maximum data rate in kilobits/s						*/
	int rssi;											   /**<	 Receive Signal Strength Indication in dBm				*/
	trwifi_standard_type_e phy_mode;					/**< Supported MAC/PHY standard								 */
	trwifi_ap_auth_type_e ap_auth_type;					/**<  @ref trwifi_ap_auth_type							  */
	trwifi_ap_crypto_type_e ap_crypto_type;				/**<  @ref trwifi_ap_crypto_type						  */
} trwifi_ap_scan_info_s;

typedef struct {
	unsigned int channel;                             /**<  wifi channel, =0 is full channel scan */
	char ssid[TRWIFI_SSID_LEN + 1];                  /**<  Service Set Identification            */
	unsigned int ssid_length;                         /**<  Service Set Identification Length     */
} trwifi_scan_config_s;

typedef struct {
	unsigned int channel;								   /**<	 soft ap wifi channel				*/
	char ssid[TRWIFI_SSID_LEN + 1];						/**<  Service Set Identification		 */
	unsigned int ssid_length;							   /**<	 Service Set Identification Length	*/
	char passphrase[TRWIFI_PASSPHRASE_LEN + 1];			/**<  ap passphrase(password)			 */
	unsigned int passphrase_length;						   /**<	 ap passphrase length				*/
	trwifi_ap_auth_type_e ap_auth_type;					/**<  @ref trwifi_ap_auth_type		  */
	trwifi_ap_crypto_type_e ap_crypto_type;				/**<  @ref trwifi_ap_crypto_type	  */
	void (*inform_new_sta_join)(void);						/**< @ref new station joining softAP	*/
} trwifi_softap_config_s;

typedef struct trwifi_scan_list {
	trwifi_ap_scan_info_s ap_info;
	struct trwifi_scan_list *next;
} trwifi_scan_list_s;

typedef struct {
	int rssi;                       /**<     Received Signal Strength Indication in dBm */
	trwifi_status_e wifi_status;    /**<  @ref trwifi_status                     */
	struct timeval disconn_time;    // optional: store the last time when the connection is disconnected
	int32_t reason_code;            // optional: the reason why connection is disconnected. value is vendor specific
} trwifi_info;

typedef uint16_t trwifi_cmd;
/*  Set/Get Power save mode */
#define TRWIFI_MSG_GET_POWERMODE 1
#define TRWIFI_MSG_SET_POWERMODE 2
/*  Get driver statistics */
#define TRWIFI_MSG_GET_STATS 3

/*  power mode(command) value */
#define TRWIFI_POWERMODE_ON 1
#define TRWIFI_POWERMODE_OFF 2

/*  TizenRT Wi-Fi Message */
typedef struct {
	trwifi_cmd cmd;
	void *data;
} trwifi_msg_s;

/*  Statistics message */
typedef struct {
	trwifi_cmd cmd;
	struct timeval start;
	struct timeval end;
	uint32_t tx_retransmit;
	uint32_t tx_drop;
	uint32_t rx_drop;
	uint32_t tx_success_cnt;
	uint32_t tx_success_bytes;
	uint32_t rx_cnt;
	uint32_t rx_bytes;
	uint32_t tx_try;
	uint32_t rssi_avg;
	uint32_t rssi_min;
	uint32_t rssi_max;
	uint32_t beacon_miss_cnt;
} trwifi_msg_stats_s;

struct netdev;
/**
 * @brief   Initialize wi-fi library
 *
 * @param[in]   dev : struct netdev registered by netdev_register()
 *
 * @function_type  synchronous call
 *
 * @description    run STA mode when wi-fi library initialized.
 *
 * @return TRWIFI_SUCCESS : success
 * @return TRWIFI_FAIL    : fail
 */
typedef trwifi_result_e (*trwifi_init)(struct netdev *dev);

/**
 * @brief   De-initialize wi-fi library
 *
 * @param[in]   dev : struct netdev registered by netdev_register()
 *
 * @function_type  synchronous call
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_deinit)(struct netdev *dev);

/**
 * @brief   Scan access points
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   config : an access point information to scan.
 *
 * @function_type  asynchronous call : Send event by lwnl_postmsg()
 * @event LWNL_EVT_SCAN_DONE    : scan success
 * @event LWNL_EVT_SCAN_FAILED  : scan fail
 *
 * @description If the call is successful, then it should generate
 *              events(LWNL_EVT_SCAN_DONE or LWNL_EVT_SCAN_FAILED).
 *              If the call fails then it shouldn't generate events.
 *
 * @return TRWIFI_SUCCESS      : success (should generate an event.)
 * @return TRWIFI_FAIL         : fail (shouldn't generate an event.)
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
/* scan type
 *     full scan: config is null
 *     scan with specific SSID: ssid is set in config
 *     scan with specific channel: channel is set in config
 *
 * invalid usage
 *     if both SSID and channel are set in config then it'll return error.
 */
typedef trwifi_result_e (*trwifi_scan_ap)(struct netdev *dev, trwifi_scan_config_s *config);

/**
 * @brief   Connect to an access point
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   config : an access point information to connect
 * @param[in]   arg    : not used.
 *
 * @function_type  asynchronous call : Send event by lwnl_postmsg()
 * @event LWNL_EVT_STA_CONNECTED      : connection success
 * @event LWNL_EVT_STA_CONNECT_FAILED : connection fail
 *
 * @description if the call is successful, then it should generate
 *              events(LWNL_EVT_STA_CONNECTED or LWNL_EVT_STA_CONNECT_FAILED).
 *              if the call fails then it shouldn't generate events.
 *              In STA mode and it's connected to an access point, wi-fi library can generates
 *              LWNL_EVT_STA_DISCONNECTED event when it's disconnected to AP.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_connect_ap)(struct netdev *dev, trwifi_ap_config_s *config, void *arg);

/**
 * @brief   Disconnect to an access point
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   arg    : not used.
 *
 * @function_type  asynchronous call : Send event by lwnl_postmsg()
 * @event LWNL_EVT_STA_DISCONNECTED : disconnected to an access point
 *
 * @description   If wi-fi library is in STA mode and it's connected to an access point,
 *                it should generate LWNL_EVT_STA_DISCONNECTED event.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_disconnect_ap)(struct netdev *dev, void *arg);

/**
 * @brief   Change to STA mode
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 *
 * @function_type  synchronous call
 *
 * @description    The call changes wi-fi to STA mode.
 *                 If wi-fi is in STA mode this API is not called, because wi-fi manager manages
 *                 Wi-Fi state.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_start_sta)(struct netdev *dev);

/**
 * @brief   Start SoftAP mode
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   config : SoftAP configuration @ref trwifi_softap_config_s
 *
 * @function_type  synchronous call
 *
 * @description    The call changes wi-fi to SoftAP mode.
 *                 If wi-fi is in SoftAP mode this API is not called, because wi-fi manager manages Wi-Fi state.
 *                 In this state, wi-fi library can generate LWNL_EVT_SOFTAP_STA_JOINED or LWNL_EVT_SOFTAP_STA_LEFT events.
 *                 LWNL_EVT_SOFTAP_STA_JOINED is called when a wi-fi device is connected to softAP.
 *                 LWNL_EVT_SOFTAP_STA_LEFT is called when a wi-fi device is left from softAP.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_start_softap)(struct netdev *dev, trwifi_softap_config_s *config);

/**
 * @brief   Stop to SoftAP mode
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 *
 * @function_type  synchronous call
 *
 * @description    Stop softAP mode.
 *                 @ref trwifi_start_sta or @ref trwifi_deinit can be called after this API
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_stop_softap)(struct netdev *dev);

/**
 * @brief   Set auto-connection to wi-fi library
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   chk    : 0: disable
 *                       1: enable
 *
 * @function_type  synchronous call
 *
 * @description    enable auto-connection features in Wi-Fi library.
 *                 If it's enabled and it connected to an access point then
 *                 it should automatically connect to an access point when
 *                 it is disconnected due to an exceptional situation(e.g power off on an access point,
 *                 weak signal). when wi-fi is disconnected it should generate LWNL_EVT_STA_DISCONNECTED event.
 *                 And it generate LWNL_EVT_STA_CONNECTED event when it succeeds to reconnect to an access point.
 *                 During auto connect state, if @ref trwifi_disconnect_ap() is called then it stop the auto-connect.
 *                 otherwise if it's disabled, it should generate LWNL_EVT_STA_DISCONNECTED event.
 *                 exceptional situation means all cases except calling @ref trwifi_disconnect_ap()
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_set_autoconnect)(struct netdev *dev, uint8_t chk);

/**
 * @brief   Get wi-fi information
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[out]  info   : wi-fi information
 *
 * @function_type  synchronous call
 *
 * @description    Refer trwifi_info. only rssi is required. Other members are not used.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 */
typedef trwifi_result_e (*trwifi_get_info)(struct netdev *dev, trwifi_info *info);

/**
 * @brief   Set driver speicific operation
 *
 * @param[in]   dev    : struct netdev registered by netdev_register()
 * @param[in]   cmd    : driver specific command
 * @param[in]   arg    : driver specific argument for command
 *
 * @function_type  synchronous call
 *
 * @description    Set driver specific operation. it's decided by vendor.
 *                 If arg is not null arg is freed by upper layer.
 *
 * @return TRWIFI_SUCCESS      : success
 * @return TRWIFI_FAIL         : fail
 * @return TRWIFI_INVALID_ARGS : arguments are invalid
 * @return TRWIFI_MODE_NOT_SUPPORTED: operation is not supported
 */
typedef trwifi_result_e (*trwifi_drv_ioctl)(struct netdev *dev, trwifi_msg_s *msg);

struct trwifi_ops {
	trwifi_init init;
	trwifi_deinit deinit;
	trwifi_scan_ap scan_ap;
	trwifi_connect_ap connect_ap;
	trwifi_disconnect_ap disconnect_ap;
	trwifi_get_info get_info;
	trwifi_start_sta start_sta;
	trwifi_start_softap start_softap;
	trwifi_stop_softap stop_softap;
	trwifi_set_autoconnect set_autoconnect;
	trwifi_drv_ioctl drv_ioctl;
};

int trwifi_serialize_scaninfo(uint8_t **buffer, trwifi_scan_list_s *scan_list);
int trwifi_post_event(struct netdev *dev, lwnl_cb_wifi evt, void *buffer, uint32_t buf_len);

#endif // _TIZENRT_WIRELESS_WIFI_H__
