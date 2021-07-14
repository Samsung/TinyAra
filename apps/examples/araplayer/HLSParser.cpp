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

#include "HLSParser.h"
#include <algorithm>
#include <vector>
#include <map>

#include "AudioUtils.h"

const char *HLSParser::PLAYLIST_EXT = ".m3u8";
const char *HLSParser::SEGMENT_EXT = ".ts";
const char *HLSParser::TARGET_DURATION = "#EXT-X-TARGETDURATION";
const char *HLSParser::BEGIN_PLAYLIST_URL = "#EXT-X-STREAM-INF";
const char *HLSParser::BEGIN_SEGMENT_URL = "#EXTINF";
const char *HLSParser::BANDWIDTH = "BANDWIDTH";

HLSParser::hls_play_list_t *HLSParser::findPlaylist(const char *url)
{
	for (std::map<std::string, hls_play_list_t *>::iterator iter = mMapMasterPlaylist.begin(); iter != mMapMasterPlaylist.end(); ++iter) {
		if (iter->first.compare(url) == 0) {
			return iter->second;
		}

		std::map<std::string, hls_play_list_t *> mapMediaPlaylist = iter->second->mapMediaPlaylist;
		for (std::map<std::string, hls_play_list_t *>::iterator iterMedia = mapMediaPlaylist.begin(); iterMedia != mapMediaPlaylist.end(); ++iterMedia) {
			if (iterMedia->first.compare(url) == 0) {
				return iterMedia->second;
			}
		}
	}
	return NULL;
}

bool HLSParser::isFirstParsedMediaPlaylist(hls_play_list_t *mediaPlaylist)
{
    hls_play_list_t *masterPlaylist = mediaPlaylist->parent;
    bool ret = false;

    if (masterPlaylist == NULL) {
        return ret;
    }

    for (auto iter = masterPlaylist->mapMediaPlaylist.begin(); iter != masterPlaylist->mapMediaPlaylist.end(); ++iter) {
        if (masterPlaylist->parsingCnt == iter->second->parsingCnt) {
            ret = true;
        }
    }

    return ret;

}

int HLSParser::parsePlaylist(const char *url, const char *data, int size)
{
	hls_play_list_t *playlist = findPlaylist(url);
	play_list_info_t pInfo;
	int idx;

	if (playlist == NULL) {
		playlist = new hls_play_list_t;
		playlist->url = std::string(url);
		DEBUG_TRACE(LOG_DEBUG, "new playlist (cannot find playlist)");
	}

	if (playlist->parsingCnt > 0) {
		playlist->parsingCnt++;
		if (playlist->type == play_list_type_t::MASTER) {
			DEBUG_TRACE(LOG_DEBUG, "");
			for (std::map<std::string, hls_play_list_t *>::iterator iter = playlist->mapMediaPlaylist.begin(); iter != playlist->mapMediaPlaylist.end(); ++iter) {
				iter->second->parsingCnt = 0;
			}
			playlist->parsingCnt = 1;
		} else if ((playlist->type == play_list_type_t::MEDIA) && (mCallbackObject != NULL) && (mCBGetPlaylistInfo != NULL) && (isFirstParsedMediaPlaylist(playlist))) {
			DEBUG_TRACE(LOG_DEBUG, "Execute GetPlaylistInfo Callback from Adaptor");
			getMediaPlaylistInfo(playlist, pInfo);
			mCBGetPlaylistInfo(mCallbackObject, url, pInfo);
		}
		return 1;
	}

	char segUrl[1024];
	while (true) {
		while (*data != '#' && *data != 0) {
			++data;
		}

		if (*data == 0) {
			break;
		}

		if (strncmp(TARGET_DURATION, data, mTargetDurationLen) == 0) {
			int target_duration = 0;
			data += mTargetDurationLen + 1;
			while (*data >= '0' && *data <= '9') {
				target_duration = target_duration * 10 + (*data - '0');
				++data;
			}
			playlist->targetDuration = target_duration;
		} else if (strncmp(BEGIN_PLAYLIST_URL, data, mBeginPlaylistUrlLen) == 0) {
			if (playlist->type == play_list_type_t::NONE) {
				playlist->type = play_list_type_t::MASTER;
				mMapMasterPlaylist.insert(make_pair(std::string(url), playlist));
			}

			while (strncmp(BANDWIDTH, data, mBandwidthLen) != 0) {
				++data;
			}
			data += mBandwidthLen + 1;

			int bandwidth = 0;
			while (*data >= '0' && *data <= '9') {
				bandwidth = bandwidth * 10 + (*data - '0');
				++data;
			}

			while (*data != '\n') {
				++data;
			}
			++data;

			idx = 0;
			while (*data != '\r' && *data != '\n') {
				segUrl[idx++] = *data++;
			}

			segUrl[idx] = 0;

			hls_play_list_t *mediaPlaylist = new hls_play_list_t;
			mediaPlaylist->url = std::string(segUrl);
			mediaPlaylist->type = play_list_type_t::MEDIA;
			mediaPlaylist->bandwidth = bandwidth;
			mediaPlaylist->parent = playlist;

			playlist->vecUrl.push_back(mediaPlaylist->url);
			playlist->mapMediaPlaylist.insert(make_pair(mediaPlaylist->url, mediaPlaylist));
		} else if (strncmp(BEGIN_SEGMENT_URL, data, mBeginSegmentUrlLen) == 0) {
			if (playlist->type == play_list_type_t::NONE) {
				playlist->type = play_list_type_t::SINGLE;
				if (playlist->parent == NULL) {
					DEBUG_TRACE(LOG_DEBUG, "Single Media Playlist [%s]", url);
					mMapMasterPlaylist.insert(make_pair(std::string(url), playlist));
				}
			}

			data += mBeginSegmentUrlLen + 1;
			int point = 0;
			double duration = 0;
			while (*data != ',') {
				duration = duration * 10 + (*data - '0');
				point *= 10;
				if (*data == '.') {
					point = 1;
				}
				++data;
			}

			if (point) {
				duration /= point;
			}

			++data;
			while (*data == '\r' || *data == '\n') {
				++data;
			}

			idx = 0;
			while (*data != '\r' && *data != '\n') {
				segUrl[idx++] = *data++;
			}

			segUrl[idx] = 0;
			playlist->vecUrl.push_back(std::string(segUrl));
		} else {
			++data;
		}
	}

	playlist->parsingCnt++;
	if (mCallbackObject != NULL && mCBGetPlaylistInfo != NULL) {
		if (isFirstParsedMediaPlaylist(playlist)) {
			pInfo.playlistUrl = playlist->parent->url;
		} else {
			pInfo.playlistUrl = url;
		}

		getMediaPlaylistInfo(playlist, pInfo);
		mCBGetPlaylistInfo(mCallbackObject, pInfo.playlistUrl.c_str(), pInfo);
	}

	return 1;
}

int HLSParser::getPlaylistInfo(const char *url, int bandwidth, has_segment_info_t *outSegInfo)
{
	int ret = 0;
	PlaylistIter iter = mMapMasterPlaylist.find(url);
	if (iter->second->parsingCnt <= 0 || iter == mMapMasterPlaylist.end()) {
		DEBUG_TRACE(LOG_ERR, "there is no appropriate playlist with url: %s", url);
		return -1;
	}

	std::map<std::string, hls_play_list_t *> &mapMediaPlaylist = iter->second->mapMediaPlaylist;
	for (iter = mapMediaPlaylist.begin(); iter != mapMediaPlaylist.end(); ++iter) {
		hls_play_list_t *playlist = iter->second;
		if (playlist->bandwidth <= bandwidth) {
			outSegInfo->url = std::string(playlist->url);
			outSegInfo->bandwidth = bandwidth;
			ret = 1;
		}
	}

	return ret;
}

int HLSParser::getSegmentInfo(const char *url, int idx, int bandwidth, has_segment_info_t *outSegInfo)
{
	PlaylistIter iter = mMapMasterPlaylist.find(url);
	hls_play_list_t *playlist;

	if (iter == mMapMasterPlaylist.end()) {
		DEBUG_TRACE(LOG_ERR, "there is no appropriate playlist with url: %s", url);
		return -1;
	}

	std::map<std::string, hls_play_list_t *> &mapMediaPlaylist = iter->second->mapMediaPlaylist;
	if (mapMediaPlaylist.size() > 0) {
		for (iter = mapMediaPlaylist.begin(); iter != mapMediaPlaylist.end(); ++iter) {
			playlist = iter->second;
			if (playlist->bandwidth == bandwidth) {
				if (idx >= (int)playlist->vecUrl.size()) {
					DEBUG_TRACE(LOG_ERR, "index is larger than media playlist vector size, idx: %d, bandwidth: %d", idx, bandwidth);
					return -1;
				}
				outSegInfo->url = std::string(playlist->vecUrl.at(idx));
				outSegInfo->strRange = "";
				outSegInfo->bandwidth = bandwidth;
				return 1;
			}
		}
	} else {
		playlist = iter->second;
		if (idx >= (int)playlist->vecUrl.size()) {
			DEBUG_TRACE(LOG_ERR, "index is larger than media playlist vector size, idx: %d, bandwidth: %d", idx, bandwidth);
			return -1;
		}

		outSegInfo->url = std::string(playlist->vecUrl.at(idx));
		outSegInfo->strRange = "";
		outSegInfo->bandwidth = bandwidth;
		return 1;
	}

	DEBUG_TRACE(LOG_DEBUG, "there is no appropriate media playlist, idx: %d, bandwidth: %d, url: %s", idx, bandwidth, url);
	return 0;
}

int HLSParser::getMediaPlaylistInfo(hls_play_list_t *playlist, play_list_info_t &pInfo)
{
	std::map<std::string, hls_play_list_t *> mapMediaPlaylist;
	std::vector<int> &bandWidthQuality = pInfo.bandWidthQuality;
	bandWidthQuality.clear();

	pInfo.duration = playlist->targetDuration;
	pInfo.hasType = HLS_MANIFEST;
	pInfo.type = playlist->type;

	if (playlist->parent == NULL) {
		mapMediaPlaylist = playlist->mapMediaPlaylist;
	} else {
		mapMediaPlaylist = playlist->parent->mapMediaPlaylist;
	}

	for (std::map<std::string, hls_play_list_t *>::iterator iter = mapMediaPlaylist.begin(); iter != mapMediaPlaylist.end(); ++iter) {
		bandWidthQuality.push_back(iter->second->bandwidth);
	}

	std::sort(bandWidthQuality.begin(), bandWidthQuality.end());
	return 1;
}

void HLSParser::setCallback_GetPlaylistInfo(void *callbackObj, Callback_GetPlaylistInfo cbGetPlaylistInfo)
{
	mCallbackObject = callbackObj;
	mCBGetPlaylistInfo = cbGetPlaylistInfo;
}
