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
 * @defgroup MEDIA MEDIA
 * @brief Provides APIs for Media Framework 
 * @{
 */

/**
 * @file media/MediaRecorder.h
 * @brief Media MediaRecorder APIs
 */

#ifndef __MEDIA_MEDIARECORDER_H
#define __MEDIA_MEDIARECORDER_H

#include <memory>
#include <media/OutputDataSource.h>
#include "MediaRecorderObserverInterface.h"

namespace media {
/**
 * @brief result of call the apis
 * @details @b #include <media/MediaRecorder.h>
 * @since TizenRT v2.0 PRE
 */
typedef enum recorder_result_e {
	/** MediaRecorder Error case */
	RECORDER_ERROR,
	/** MediaRecorder Success case */
	RECORDER_OK
} recorder_result_t;

class MediaRecorderImpl;

/**
 * @class 
 * @brief This class implements the MediaRecorder capability agent.
 * @details @b #include <media/MediaRecorder.h>
 * @since TizenRT v2.0 PRE
 */
class MediaRecorder
{
public:
	/**
	 * @brief Constructs an empty MediaRecorder.
	 * @details @b #include <media/MediaRecorder.h>
	 * @since TizenRT v2.0 PRE
	 */
	MediaRecorder();
	/**
	 * @brief Deconstructs an empty MediaRecorder.
	 * @details @b #include <media/MediaRecorder.h>
	 * @since TizenRT v2.0 PRE
	 */
	~MediaRecorder();
	/**
	 * @brief Start the MediaRecorderWorker.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @return The result of the create operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t create();
	/**
	 * @brief Stop the MediaRecorderWorker.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @return The result of the destroy operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t destroy();
	/**
	 * @brief Allocate and prepare resources related to the recorder.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @return The result of the prepare operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t prepare();
	/**
	 * @brief Releases allocated resources related to the recorder.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @return The result of the unpreapre operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t unprepare();
	/**
	 * @brief Start recording.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is async call apis
	 * Order to MediaRecordWorker begin recording through the queue
	 * @return The result of the unpreapre operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t start();
	/**
	 * @brief Pause recording.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is async call apis
	 * Order to MediaRecordWorker pause recording through the queue
	 * @return The result of the pause operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t pause();
	/**
	 * @brief Stop recording.
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is async call apis
	 * Order to MediaRecordWorker stop recording through the queue
	 * @return The result of the stop operation
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t stop();
	/**
	 * @brief Gets the current volume
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @return The value of current mic volume
	 * @since TizenRT v2.0 PRE
	 */
	int getVolume();
	/**
	 * @brief Sets the volume adjusted
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @param[in] vol The vol that the value of mic volume
	 * @return The result of setting the mic volume
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t setVolume(int vol);
	/**
	 * @brief Sets the DatSource of output data
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * @param[in] dataSource The dataSource that the config of output data
	 * @return The result of setting the datasource
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t setDataSource(std::unique_ptr<stream::OutputDataSource> dataSource);
	/**
	 * @brief Sets the observer of MediaRecorder
	 * @details @b #include <media/MediaRecorder.h>
	 * This function is sync call apis
	 * It sets the user's function
	 * @return The result of setting the observer
	 * @since TizenRT v2.0 PRE
	 */
	recorder_result_t setObserver(std::shared_ptr<MediaRecorderObserverInterface> observer);

private:
	std::shared_ptr<MediaRecorderImpl> mPMrImpl;
};
} // namespace media

#endif
/** @} */ // end of MEDIA group
