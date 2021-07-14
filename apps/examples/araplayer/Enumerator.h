#pragma once

/**
 * @brief HLS Playlist type.
 * @details
 * @since TizenRT v3.0
 */
typedef enum play_list_type_s {
	/** HLS Playlist type case */
	NONE = -1,
	/** HLS Playlist has a multi-tree structure */
	MASTER = 0,
	/** HLS Playlist has a single-tree structure */
	SINGLE = 1,
	/** HLS Playlist has a Media file */
	MEDIA,
} play_list_type_t;

/**
 * @brief Http Adaptive Streaming Type.
 * @details
 * @since TizenRT v3.0
 */
typedef enum has_type_s {
	/** Http live streaming */
	HLS_MANIFEST,
	/** Mpeg Dash */
	DASH_MPD
} has_type_t;
