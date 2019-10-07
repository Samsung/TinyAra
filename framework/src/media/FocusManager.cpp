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

#include <media/FocusManager.h>

namespace media {

FocusManager::FocusRequester::FocusRequester(std::shared_ptr<stream_info_t> stream_info, std::shared_ptr<FocusChangeListener> listener)
	: mId(stream_info->id), mListener(listener)
{
}

bool FocusManager::FocusRequester::hasSameId(std::shared_ptr<FocusRequest> focusRequest)
{
	return mId == focusRequest->getStreamInfo()->id;
}

void FocusManager::FocusRequester::notify(int focusChange)
{
	if (mListener) {
		mListener->onFocusChange(focusChange);
	}
}

FocusManager &FocusManager::getFocusManager()
{
	static FocusManager focusManager;
	return focusManager;
}

int FocusManager::abandonFocus(std::shared_ptr<FocusRequest> focusRequest)
{
	std::lock_guard<std::mutex> lock(mFocusLock);
	if (focusRequest == nullptr) {
		return FOCUS_REQUEST_FAIL;
	}

	if ((!mFocusList.empty()) && (mFocusList.front()->hasSameId(focusRequest))) {
		auto focus = mFocusList.front();
		mFocusList.pop_front();
		focus->notify(FOCUS_LOSS);
		if (!mFocusList.empty()) {
			mFocusList.front()->notify(FOCUS_GAIN);
		}
	} else {
		removeFocusElement(focusRequest);
	}

	return FOCUS_REQUEST_SUCCESS;
}

int FocusManager::requestFocus(std::shared_ptr<FocusRequest> focusRequest)
{
	std::lock_guard<std::mutex> lock(mFocusLock);
	if (focusRequest == nullptr) {
		return FOCUS_REQUEST_FAIL;
	}

	if ((!mFocusList.empty()) && (mFocusList.front()->hasSameId(focusRequest))) {
		return FOCUS_REQUEST_SUCCESS;
	}

	removeFocusElement(focusRequest);

	if (!mFocusList.empty()) {
		mFocusList.front()->notify(FOCUS_LOSS);
	}

	auto focusRequester = std::make_shared<FocusRequester>(focusRequest->getStreamInfo(), focusRequest->getListener());
	mFocusList.push_front(focusRequester);
	focusRequester->notify(FOCUS_GAIN);

	return FOCUS_REQUEST_SUCCESS;
}

void FocusManager::removeFocusElement(std::shared_ptr<FocusRequest> focusRequest)
{
	auto iterator = mFocusList.begin();
	while (iterator != mFocusList.end()) {
		if ((*iterator)->hasSameId(focusRequest)) {
			iterator = mFocusList.erase(iterator);
		} else {
			++iterator;
		}
	}
}
} // namespace media
