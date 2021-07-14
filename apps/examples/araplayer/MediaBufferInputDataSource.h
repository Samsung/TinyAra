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

#ifndef __MEDIA_BUFFERINPUTDATASOURCE_H__
#define __MEDIA_BUFFERINPUTDATASOURCE_H__

#include <media/InputDataSource.h>
#include <media/MediaTypes.h>

using namespace media;
using namespace media::stream;

class MediaBufferInputDataSource : public media::stream::InputDataSource
{
public:
	MediaBufferInputDataSource();
	virtual ~MediaBufferInputDataSource();

	MediaBufferInputDataSource(audio_type_t audioType, audio_format_type_t format, unsigned int rate, unsigned int chan, void *stream);

	bool isPrepared() override;
	bool open() override;
	bool close() override;
	ssize_t read(unsigned char *buf, size_t size) override;

	void *mStream;

private:
	unsigned char mBuf[4096];
	bool mIsOpened;
};
#endif
