/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
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

//#define LWNL_LOGI(tag, fmt, args...) nlvdbg("%d\t"tag"[T%d]"fmt"\n", __LINE__, getpid(), ##args)
//#define LWNL_LOGE(tag, fmt, args...) nldbg("%d\t"tag"[ERR][T%d]"fmt"\n", __LINE__, getpid(), ##args)
//#define LWNL_LOGI(tag, fmt, args...) lldbg("%d\t"tag"[T%d]"fmt"\n", __LINE__, getpid(), ##args)
//#define LWNL_LOGE(tag, fmt, args...) lldbg("%d\t"tag"[ERR][T%d]"fmt"\n", __LINE__, getpid(), ##args)
#define LWNL_LOGI(tag, fmt, args...)
#define LWNL_LOGE(tag, fmt, args...)
#define LWNL_ENTER(tag)							\
	do {										\
		LWNL_LOGI(tag, "-->");					\
	} while (0)

#define LWNL_LEAVE(tag)							\
	do {										\
		LWNL_LOGI(tag, "<--");					\
	} while (0)
