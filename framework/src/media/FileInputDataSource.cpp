/* ****************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include <tinyara/config.h>
#include <stdio.h>
#include <debug.h>

#include <media/FileInputDataSource.h>
#include "utils/MediaUtils.h"
#include "Decoder.h"
#include "InputStreamBuffer.h"

#ifndef CONFIG_FILE_DATASOURCE_STREAM_BUFFER_SIZE
#define CONFIG_FILE_DATASOURCE_STREAM_BUFFER_SIZE 4096
#endif

#ifndef CONFIG_FILE_DATASOURCE_STREAM_BUFFER_THRESHOLD
#define CONFIG_FILE_DATASOURCE_STREAM_BUFFER_THRESHOLD 2048
#endif

namespace media {
namespace stream {

FileInputDataSource::FileInputDataSource() : InputDataSource(), mDataPath(""), mFp(nullptr)
{
}

FileInputDataSource::FileInputDataSource(const std::string &dataPath)
	: InputDataSource(), mDataPath(dataPath), mFp(nullptr)
{
}

FileInputDataSource::FileInputDataSource(const FileInputDataSource &source)
	: InputDataSource(source), mDataPath(source.mDataPath), mFp(source.mFp)
{
}

FileInputDataSource &FileInputDataSource::operator=(const FileInputDataSource &source)
{
	InputDataSource::operator=(source);
	return *this;
}

bool FileInputDataSource::open()
{
	if (!getStreamBuffer()) {
		auto streamBuffer = InputStreamBuffer::create( \
			CONFIG_FILE_DATASOURCE_STREAM_BUFFER_SIZE, \
			CONFIG_FILE_DATASOURCE_STREAM_BUFFER_THRESHOLD);

		if (!streamBuffer) {
			meddbg("streamBuffer is nullptr!\n");
			return false;
		}

		setStreamBuffer(streamBuffer);
	}

	if (!mFp) {
		setAudioType(utils::getAudioTypeFromPath(mDataPath));

		switch (getAudioType()) {
		case AUDIO_TYPE_MP3:
		case AUDIO_TYPE_AAC:
		case AUDIO_TYPE_OPUS:
			setDecoder(std::make_shared<Decoder>(getChannels(), getSampleRate()));
			break;
		case AUDIO_TYPE_FLAC:
			/* To be supported */
			break;
		default:
			/* Don't set any decoder for unsupported formats */
			break;
		}

		mFp = fopen(mDataPath.c_str(), "rb");
		if (mFp) {
			medvdbg("file open success\n");
			start();
			return true;
		} else {
			meddbg("file open failed error : %d\n", errno);
			return false;
		}
	}

	/** return true if mFp is not null, because it means it using now */
	return true;
}

bool FileInputDataSource::close()
{
	if (mFp) {
		stop();

		int ret = fclose(mFp);
		if (ret == OK) {
			mFp = nullptr;
			medvdbg("close success!!\n");
			return true;
		} else {
			meddbg("close failed ret : %d error : %d\n", ret, errno);
			return false;
		}
	}

	meddbg("close failed, mFp is nullptr!!\n");
	return false;
}

bool FileInputDataSource::isPrepare()
{
	if (mFp == nullptr) {
		medvdbg("mFp is nullptr\n");
		return false;
	}
	return true;
}

ssize_t FileInputDataSource::onStreamBufferWritable()
{
	unsigned char buf[1024];

	size_t rlen = fread(buf, sizeof(unsigned char), sizeof(buf), mFp);
	medvdbg("read size : %d\n", rlen);
	if (rlen == 0) {
		/* If file position reaches end of file, it's a normal case, we returns 0 */
		if (feof(mFp)) {
			medvdbg("eof!!!\n");
			return 0;
		}

		/* Otherwise, an error occurred, we also returns error */
		return EOF;
	}

	return writeToStreamBuffer(buf, rlen);
}

int FileInputDataSource::seek(long offset, int origin)
{
	return fseek(mFp, offset, origin);
}

FileInputDataSource::~FileInputDataSource()
{
}

} // namespace stream
} // namespace media
