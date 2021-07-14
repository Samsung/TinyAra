/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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
 * @file HLSParser.h
 * @brief Http Live Streaming Parser APIS.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <string.h>

#include "HASParserInterface.h"
#include "Enumerator.h"

class HLSParser : public HASParserInterface
{
public:
	/**
	 * @brief HLSParser Constructs a new object provide
	 * @details @b #include <araplayer/HASParser.h>
	 * @since TizenRT v3.0
	 */
	HLSParser() :
		mTargetDurationLen(strlen(TARGET_DURATION)), mBeginPlaylistUrlLen(strlen(BEGIN_PLAYLIST_URL)), mBeginSegmentUrlLen(strlen(BEGIN_SEGMENT_URL)), mBandwidthLen(strlen(BANDWIDTH)), mCBGetPlaylistInfo(NULL)
	{
	}

	/**
	 * @brief HLS Playlist(m3u8) data parsing API
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] url  requested playlist url
	 * @param[in] data playlist data buffer point
	 * @param[in] data playlist data size
	 * @return True is Success, False is Fail
	 * @since TizenRT v3.0
	 */
	int parsePlaylist(const char *url, const char *data, int size);
	/**
	 * @brief Get the Playlist(m3u8)Information
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] url requested playlist url
	 * @param[in] bandwidth value of bandwith
	 * @param[out] outSegInfo playlist info output data structure
	 * @return True is Success, False is Fail
	 * @since TizenRT v3.0
	 */
	int getPlaylistInfo(const char *url, int bandwidth, has_segment_info_t *outSegInfo);
	/**
	 * @brief Get the Segment Information
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] url  requested playlist url
	 * @param[in] inx  segment index
	 * @param[in] bandwidth value of bandwith
	 * @param[out] outSegInfo playlist info output data structure
	 * @return True is Success, False is Fail
	 * @since TizenRT v3.0
	 */
	int getSegmentInfo(const char *url, int idx, int bandwidth, has_segment_info_t *outSegInfo);
	/**
	 * @brief Sets the callback for Playlist parser data
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] callbackObj the callback to be set for HASStream
	 * @param[in] callbackFnc sets the callback function
	 * @since TizenRT v3.0
	 */
	void setCallback_GetPlaylistInfo(void *callbackObj, Callback_GetPlaylistInfo cbGetPlaylistInfo);

private:
	struct hls_play_list_s {
		int parsingCnt;
		int bandwidth;
		int targetDuration;
		play_list_type_t type;
		hls_play_list_s *parent;
		std::string url;
		std::vector<std::string> vecUrl;
		std::map<std::string, hls_play_list_s *> mapMediaPlaylist;

		hls_play_list_s() :
			parsingCnt(0), bandwidth(0), type(play_list_type_t::NONE), parent(NULL)
		{
		}
	};

	typedef hls_play_list_s hls_play_list_t;
	typedef std::map<std::string, hls_play_list_t *>::iterator PlaylistIter;

	static const char *PLAYLIST_EXT;
	static const char *SEGMENT_EXT;
	static const char *TARGET_DURATION;
	static const char *BEGIN_PLAYLIST_URL;
	static const char *BEGIN_SEGMENT_URL;
	static const char *BANDWIDTH;

	int mTargetDurationLen;
	int mBeginPlaylistUrlLen;
	int mBeginSegmentUrlLen;
	int mBandwidthLen;

	std::map<std::string, hls_play_list_t *> mMapMasterPlaylist;
	Callback_GetPlaylistInfo mCBGetPlaylistInfo;

	hls_play_list_t *findPlaylist(const char *url);
	int getMediaPlaylistInfo(hls_play_list_t *playlist, play_list_info_t &pInfo);
	bool isFirstParsedMediaPlaylist(hls_play_list_t *playlist);
};
