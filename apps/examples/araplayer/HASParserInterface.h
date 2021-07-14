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
 * @file HASParserInterface.h
 * @brief HASParserInterface APIS.
 */

#pragma once

#include "PlaylistInfo.h"

/**
 * @class
 * @brief This class provides an interface to the user.
 * @details @b #include <araplay/HASParserInterface.h>
 * This class informs the user of the parser apis.
 * @since TizenRT v3.0
 */

class HASParserInterface
{
public:
	/**
	 * callback function type.
	 * size_t callback(void *callbackObj, const char *url, play_list_info_t &out_pInfo)
	 * The callbacks CANNOT be non-static class member functions.
	 */
	typedef void (*Callback_GetPlaylistInfo)(void *callbackObj, const char *url, play_list_info_t &out_pInfo);

	virtual int parsePlaylist(const char *url, const char *data, int size)
	{
		return 0;
	}

	virtual int parseSegment(const char *url, const char *data, int size)
	{
		return 0;
	}

	virtual int getPlaylistInfo(const char *url, int bandwidth, has_segment_info_t *outSegInfo)
	{
		return 0;
	}

	virtual int getSegmentInfo(const char *url, int idx, int bandwidth, has_segment_info_t *outSegInfo)
	{
		return 0;
	}

	virtual void setCallback_GetPlaylistInfo(void *callbackObj, Callback_GetPlaylistInfo cbGetPlaylistInfo)
	{
	}

protected:
	void *mCallbackObject;

	HASParserInterface() :
		mCallbackObject(0)
	{
	}
};
