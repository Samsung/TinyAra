
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "AudioCircularBuffer.h"

AudioCircularBuffer::AudioCircularBuffer(size_t capacity) :
	mBegIdx(0), mEndIdx(0), mSize(0), mCapacity(capacity), mData(nullptr)
{
	mData = new unsigned char[capacity];
	assert(mData);
}

AudioCircularBuffer::~AudioCircularBuffer()
{
	delete[] mData;
}

size_t AudioCircularBuffer::write(const unsigned char *data, size_t bytes)
{
	if (bytes == 0) {
		return 0;
	}

	mMutex.lock();

	size_t bytes_to_write = std::min(bytes, mCapacity - mSize);

	if (bytes_to_write <= mCapacity - mEndIdx) {
		memcpy(mData + mEndIdx, data, bytes_to_write);
		mEndIdx += bytes_to_write;
		if (mEndIdx == mCapacity) {
			mEndIdx = 0;
		}
	} else {
		size_t size_1 = mCapacity - mEndIdx;
		memcpy(mData + mEndIdx, data, size_1);
		size_t size_2 = bytes_to_write - size_1;
		memcpy(mData, data + size_1, size_2);
		mEndIdx = size_2;
	}

	mSize += bytes_to_write;

	mMutex.unlock();
	return bytes_to_write;
}

size_t AudioCircularBuffer::read(unsigned char *buf, size_t bytes)
{
	if (bytes == 0) {
		return 0;
	}

	mMutex.lock();

	size_t bytes_to_read = std::min(bytes, mSize);

	if (bytes_to_read <= mCapacity - mBegIdx) {
		memcpy(buf, mData + mBegIdx, bytes_to_read);
		mBegIdx += bytes_to_read;
		if (mBegIdx == mCapacity) {
			mBegIdx = 0;
		}
	} else {
		size_t size_1 = mCapacity - mBegIdx;
		memcpy(buf, mData + mBegIdx, size_1);
		size_t size_2 = bytes_to_read - size_1;
		memcpy(buf + size_1, mData, size_2);
		mBegIdx = size_2;
	}

	mSize -= bytes_to_read;

	mMutex.unlock();
	return bytes_to_read;
}

void AudioCircularBuffer::clear()
{
	mMutex.lock();

	memset(mData, 0x00, mCapacity);
	mBegIdx = mEndIdx = mSize = 0;

	mMutex.unlock();
}
