/*
 * Copyright (C) 2007 The Android Open Source Project
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
 */

/**
 * This is a temporary hack to enable logging from cutils.
 */

#ifndef _CUTILS_LOGHACK_H
#define _CUTILS_LOGHACK_H
#ifndef LOG_TAG
#define LOG_TAG __FILE__
#endif
#ifdef HAVE_ANDROID_OS
#include <cutils/log.h>
#else
#include <stdio.h>
#define ALOG(level, ...) \
        do{printf(level "/%s:%s-%d: ", LOG_TAG, __func__, __LINE__);\
				printf(  __VA_ARGS__);\
					printf("\n");\
		}while(0)
#define ALOGV(...)   ALOG("V", __VA_ARGS__)
#define ALOGD(...)   ALOG("D", __VA_ARGS__)
#define ALOGI(...)   ALOG("I", __VA_ARGS__)
#define ALOGW(...)   ALOG("W", __VA_ARGS__)
#define ALOGE(...)   ALOG("E", __VA_ARGS__)
#define LOG_ALWAYS_FATAL(...)   do { ALOGE(__VA_ARGS__); exit(1); } while (0)
#endif

#endif // _CUTILS_LOGHACK_H
