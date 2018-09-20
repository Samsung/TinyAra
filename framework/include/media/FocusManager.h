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

/**
 * @ingroup MEDIA
 * @{
 */

/**
 * @file media/FocusManager.h
 * @brief Media FocusManager APIs
 */

#ifndef __MEDIA_FOCUSMANAGER_H
#define __MEDIA_FOCUSMANAGER_H

#include <list>
#include <memory>
#include <mutex>
#include <string>

#include <unistd.h>

#include <media/FocusRequest.h>

namespace media {

static const int FOCUS_REQUEST_SUCCESS = 0;
static const int FOCUS_REQUEST_FAIL = -1;

/**
 * @class 
 * @brief This class is focus manager
 * @details @b #include <media/FocusManager.h>
 * @since TizenRT v2.0
 */
class FocusManager
{
public:
	/**
	 * @brief Get FocusManager Instance
	 * @details @b #include <media/FocusManager.h>
	 * @return Instance of FocusManager
	 * @since TizenRT v2.0
	 */
	static FocusManager &getFocusManager();
	/**
	 * @brief Abandon Focus
	 * @remarks Do not call this funtion within FocusChangeListner::onFocusChange()
	 * @details @b #include <media/FocusManager.h>
	 * param[in] focusRequest FocusRequest to abandon
	 * @return return FOCUS_REQUEST_SUCCESS on Success, else return FOCUS_REQUEST_FAIL
	 * @since TizenRT v2.0
	 */
	int abandonFocus(std::shared_ptr<FocusRequest> focusRequest);
	/**
	 * @brief Request Focus
	 * @remarks Do not call this funtion within FocusChangeListner::onFocusChange()
	 * @details @b #include <media/FocusManager.h>
	 * param[in] focusRequest FocusRequest to request
	 * @return return FOCUS_REQUEST_SUCCESS on Success, else return FOCUS_REQUEST_FAIL
	 * @since TizenRT v2.0
	 */
	int requestFocus(std::shared_ptr<FocusRequest> focusRequest);
	/**
	 * @brief Get a pid of the current FocusRequester
	 * @remarks Do not call this funtion within FocusChangeListner::onFocusChange()
	 * @details @b #include <media/FocusManager.h>
	 * @return return pid of the current focused thread
	 * @since TizenRT v2.0
	 */
	pid_t getCurrentRequesterPid();

private:
	class FocusRequester
	{
	public:
		FocusRequester(std::string id, std::shared_ptr<FocusChangeListener> listener, pid_t pid);
		bool hasSameId(std::string id);
		void notify(int focusChange);
		pid_t getpid();

	private:
		std::string mId;
		std::shared_ptr<FocusChangeListener> mListener;
		pid_t mPid;
	};

	FocusManager();
	virtual ~FocusManager() = default;
	void removeFocusElement(std::string id);
	pid_t mCurrentRequesterPid;
	std::list<std::shared_ptr<FocusRequester>> mFocusList;
	std::mutex mFocusLock;
};
} // namespace media

#endif
/** @} */ // end of MEDIA group
