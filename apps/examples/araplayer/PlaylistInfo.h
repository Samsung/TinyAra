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

#pragma once

#include <vector>
#include <string>
#include "Enumerator.h"
#include "HASType.h"

/**
 * @struct  play_list_info_s
 * @brief   Define Playlist output structure, inludes url, streaming type, playlist type,
 *          and duration, bandwith
 */
struct play_list_info_s {
	/* playlist request url */
	std::string playlistUrl;
	/* http adaptive streaming type (hls/dash) */
	has_type_t hasType;
	/* playlist type (master/single/media) */
	play_list_type_t type;
	/* media file play duration time */
	int duration;
	/* bandwith level list (bps)*/
	std::vector<int> bandWidthQuality;
	
	//TO-DO will remove
	int bufferSize;
};

/**
 * @struct  has_segment_info_s
 * @brief   Define Segment output structure, includes url, bandwith
 */
struct has_segment_info_s {
	std::string url;
	int bandwidth;
	//TO-DO will remove
	std::string strRange;
	//TO-DO will remove
	range_t range;
};

typedef play_list_info_s play_list_info_t;
typedef has_segment_info_s has_segment_info_t;
