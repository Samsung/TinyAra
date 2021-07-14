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

#include <string.h>
#include "HASParser.h"
#include "HLSParser.h"
#include "AudioUtils.h"

HASParser::HASParser()
{
	mMapHasParser.insert(std::make_pair(".m3u8", new HLSParser));
}

HASParserInterface *HASParser::findParserWithPlaylistUrl(const char *url)
{
	int urlLen = strlen(url);
	if (urlLen <= 5) {
		DEBUG_TRACE(LOG_ERR, "url length <= 5");
		return NULL;
	}

	int i;
	for (i = urlLen - 1; i >= 0 && url[i] != '.'; --i);

	if (i < 0) {
		DEBUG_TRACE(LOG_ERR, "there is no extension");
		return NULL;
	}

	ParserIter iter = mMapHasParser.find(url + i);
	if (iter == mMapHasParser.end()) {
		DEBUG_TRACE(LOG_ERR, "there is no appropriate parser");
		return NULL;
	}

	return iter->second;
}

int HASParser::parsePlaylist(const char *url, const char *data, int size)
{
	HASParserInterface *parser = findParserWithPlaylistUrl(url);
	if (parser == NULL) {
		DEBUG_TRACE(LOG_ERR, "parser is null");
		return -1;
	}

	return parser->parsePlaylist(url, data, size);
}

int HASParser::parseSegment(const char *url, const char *data, int size)
{
	for (ParserIter iter = mMapHasParser.begin(); iter != mMapHasParser.end(); ++iter) {
		int ret = iter->second->parseSegment(url, data, size);
		if (ret > 0) {
			return 1;
		}
	}

	DEBUG_TRACE(LOG_DEBUG, "segment index box parsing fail");
	return 0;
}

int HASParser::getPlaylistInfo(const char *url, int bandwidth, has_segment_info_t *outSegInfo)
{
	if (bandwidth <= 0) {
		DEBUG_TRACE(LOG_ERR, "GetSegmentUrlWithIndex, bandwidth is %d", bandwidth);
		return -1;
	}

	HASParserInterface *parser = findParserWithPlaylistUrl(url);
	return parser->getPlaylistInfo(url, bandwidth, outSegInfo);
}

int HASParser::getSegmentInfo(const char *url, unsigned int idx, int bandwidth, has_segment_info_t *outSegInfo)
{
	HASParserInterface *parser = findParserWithPlaylistUrl(url);
	if (parser == nullptr) {
		return -1;
	}

	return parser->getSegmentInfo(url, idx, bandwidth, outSegInfo);
}

void HASParser::setCallback_GetPlaylistInfo(void *callbackObj, HASParserInterface::Callback_GetPlaylistInfo cbGetPlaylistInfo)
{
	for (ParserIter iter = mMapHasParser.begin(); iter != mMapHasParser.end(); ++iter) {
		iter->second->setCallback_GetPlaylistInfo(callbackObj, cbGetPlaylistInfo);
	}
}

HASParser *HASParser::getInstance()
{
	static HASParser instance;
	return &instance;
}
