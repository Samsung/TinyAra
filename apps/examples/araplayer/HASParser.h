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
 * @file HASParser.h
 * @brief Http Adaptive Streaming Parser APIS.
 */

#pragma once

#include <map>
#include <string>
#include "HASParserInterface.h"

#define DEBUG_LOG(X) DEBUG_TRACE(X)

/**
 * @class
 * @brief This class is http adaptive streaming parser data structure
 * @details @b #include <araplayer/HASParser.h>
 * @since TizenRT v3.0
 */

class HASParser
{
public:
	/**
	 * @brief HASParser Constructs a new object provide
	 * @details @b #include <araplayer/HASParser.h>
	 * @since TizenRT v3.0
	 */
	HASParser();
	/**
	 * @brief Playlist parsing API
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] url  requested playlist url
	 * @param[in] data playlist data buffer point
	 * @param[in] data playlist data size
	 * @return True is Success, False is Fail
	 * @since TizenRT v3.0
	 */
	int parsePlaylist(const char *url, const char *data, int size);
	/**
	 * @brief Get the Playlist Information
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] url  requested playlist url
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
	int getSegmentInfo(const char *url, unsigned int idx, int bandwidth, has_segment_info_t *outSegInfo);
	/**
	 * @brief Sets the callback for Playlist parser data
	 * @details @b #include <araplayer/HASParser.h>
	 * @param[in] callbackObj the callback to be set for HASStream
	 * @param[in] callbackFnc sets the callback function
	 * @since TizenRT v3.0
	 */
	void setCallback_GetPlaylistInfo(void *callbackObj, HASParserInterface::Callback_GetPlaylistInfo cbGetPlaylistInfo);
	
	//TO-DO will remove
	static HASParser *getInstance();
	//TO-DO will remove
	int parseSegment(const char *url, const char *data, int size);

private:
	HASParserInterface *findParserWithPlaylistUrl(const char *url);
	HASParserInterface *findParserWithSegmentUrl(const char *url);
	typedef std::map<std::string, HASParserInterface *>::iterator ParserIter;
	std::map<std::string, HASParserInterface *> mMapHasParser;
};
