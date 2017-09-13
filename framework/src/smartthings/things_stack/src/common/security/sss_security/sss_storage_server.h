/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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


#ifndef _SSS_KEY_CERT_H_
#define _SSS_KEY_CERT_H_
#include <stdio.h>
#include <string.h>

/** Persistent storage open handler with secure storage. */
extern FILE* secure_fopen(const char *path, const char *mode);

/** Persistent storage read handler with secure storage.*/
extern size_t secure_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/** Persistent storage write handler with secure storage.*/
extern size_t secure_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/** Persistent storage close handler with secure storage. */
extern int secure_fclose(FILE *fp);

/** Persistent storage unlink handler.*/
extern int secure_funlink(const char *path);

/** Persistent secure_remove handler.*/
extern int secure_remove(const char *path);
#endif
