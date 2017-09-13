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

///
/// This sample provides the way to create cloud sample
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "cacommon.h"
#include "cacommonutil.h"
#include "logger.h"
#include "mbedtls/ssl_ciphersuites.h"
#include "thingslogger.h"

#include "octypes.h"
#include "oiccommon.h"
#include "securevirtualresourcetypes.h"
#include "mbedtls/see_api.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pem.h"
#include "ssskeyandcert.h"
#include "credresource.h"
#include "srmresourcestrings.h"
#if defined(CONFIG_TLS_WITH_SSS) && defined(CONFIG_HW_ECDSA)
#include "oicsecuritymanager.h"
#endif




unsigned int g_iotivity_cacert_index;
#if defined(CONFIG_TLS_WITH_SSS) && defined(CONFIG_HW_ECDSA)
unsigned int g_iotivity_key_index;
unsigned int g_iotivity_key_curve;
unsigned int g_iotivity_cert_index;

uint8_t *g_certChain;
#endif


// pkss
// temp iotivity key stored at sss
const char g_iot_ss_cert[664] = \
{0x30, 0x82, 0x2 , 0x94, 0x30, 0x82, 0x2 , 0x38, 0xa0, 0x3 , 0x2 , 0x1 , 0x2 , 0x2 , 0x14, 0x48, 0x41, 0x30, 0x31, 0x50, 0x31, 0x37, 0x30, 0x31, 
0x33, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30, 0x36, 0x31, 0x33, 0x39, 0x30, 0xc , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 
0x0 , 0x30, 0x7e, 0x31, 0x33, 0x30, 0x31, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x2a, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 
0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x4f, 0x43, 0x46, 0x20, 0x48, 0x41, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 
0x53, 0x75, 0x62, 0x43, 0x41, 0x20, 0x76, 0x31, 0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xb , 0x13, 0x13, 0x4f, 0x43, 0x46, 0x20, 0x48, 
0x41, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x53, 0x75, 0x62, 0x43, 0x41, 0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 
0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 
0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 0x20, 0x17, 0xd , 0x31, 0x37, 0x30, 0x31, 0x33, 0x31, 0x30, 0x34, 0x33, 0x31, 0x31, 
0x33, 0x5a, 0x18, 0xf , 0x32, 0x30, 0x36, 0x39, 0x31, 0x32, 0x33, 0x31, 0x31, 0x34, 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x81, 0x8e, 0x31, 0x49, 
0x30, 0x47, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x40, 0x4f, 0x43, 0x46, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x3a, 0x20, 0x41, 0x70, 0x70, 
0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x20, 0x50, 0x55, 0x46, 0x20, 0x28, 0x36, 0x36, 0x32, 0x37, 0x65, 0x63, 0x65, 0x30, 0x2d, 0x33, 0x30, 0x64, 
0x39, 0x2d, 0x34, 0x62, 0x34, 0x31, 0x2d, 0x38, 0x33, 0x36, 0x36, 0x2d, 0x62, 0x38, 0x63, 0x36, 0x61, 0x65, 0x35, 0x37, 0x38, 0x65, 0x62, 0x62, 
0x29, 0x31, 0x16, 0x30, 0x14, 0x6 , 0x3 , 0x55, 0x4 , 0xb , 0x13, 0xd , 0x4f, 0x43, 0x46, 0x20, 0x48, 0x41, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 
0x65, 0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 
0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 0x59, 0x30, 0x13, 
0x6 , 0x7 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x2 , 0x1 , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x3 , 0x1 , 0x7 , 0x3 , 0x42, 0x0 , 0x4 , 0xb7, 
0x15, 0xce, 0x3c, 0xfc, 0x41, 0x96, 0x22, 0xc2, 0x37, 0xeb, 0x15, 0x37, 0x62, 0xd9, 0x99, 0xc2, 0xc4, 0x15, 0xa7, 0xbb, 0x36, 0x7b, 0x90, 0xb1, 
0xf9, 0x42, 0xbe, 0x89, 0x3b, 0x7d, 0x4c, 0x8f, 0x84, 0x63, 0x41, 0x35, 0x63, 0xcc, 0xe9, 0xbf, 0x47, 0x45, 0x5e, 0xa8, 0x2f, 0x74, 0x35, 0x48, 
0x13, 0xfc, 0x1e, 0x38, 0x8d, 0xb1, 0xc5, 0xec, 0x4e, 0x85, 0x1f, 0x17, 0xa7, 0x34, 0xa , 0xa3, 0x7f, 0x30, 0x7d, 0x30, 0xe , 0x6 , 0x3 , 0x55, 
0x1d, 0xf , 0x1 , 0x1 , 0xff, 0x4 , 0x4 , 0x3 , 0x2 , 0x3 , 0xc8, 0x30, 0x32, 0x6 , 0x3 , 0x55, 0x1d, 0x1f, 0x4 , 0x2b, 0x30, 0x29, 0x30, 0x27, 
0xa0, 0x25, 0xa0, 0x23, 0x86, 0x21, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x70, 0x72, 0x6f, 0x64, 0x63, 0x61, 0x2e, 0x73, 0x61, 0x6d, 0x73, 
0x75, 0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x63, 0x72, 0x6c, 0x30, 0x37, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 
0x7 , 0x1 , 0x1 , 0x4 , 0x2b, 0x30, 0x29, 0x30, 0x27, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 0x7 , 0x30, 0x1 , 0x86, 0x1b, 0x68, 0x74, 0x74, 
0x70, 0x3a, 0x2f, 0x2f, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x73, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 
0x30, 0xc , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 0x0 , 0x3 , 0x48, 0x0 , 0x30, 0x45, 0x2 , 0x20, 0x44, 0x40, 0xb1, 
0x5e, 0x2b, 0x93, 0xae, 0xce, 0xe3, 0xc7, 0x84, 0xba, 0x88, 0x77, 0x3 , 0x99, 0x22, 0x41, 0x25, 0x95, 0x33, 0xbe, 0xb9, 0x6c, 0x8e, 0x47, 0x50, 
0x64, 0xf4, 0x32, 0x30, 0x94, 0x2 , 0x21, 0x0 , 0xcb, 0x1c, 0x9f, 0xa9, 0xe5, 0xaf, 0x21, 0x4b, 0x9a, 0x94, 0xe1, 0x47, 0xf5, 0x32, 0x6 , 0xf9, 
0x99, 0x9b, 0xbe, 0xab, 0x47, 0x7 , 0x8d, 0x4d, 0x6c, 0xb0, 0xb1, 0x60, 0x80, 0xa5, 0xca, 0x79, };

const char g_iot_ss_cacert[1260] = \
{0x30, 0x82, 0x2 , 0x5d, 0x30, 0x82, 0x2 , 0x1 , 0xa0, 0x3 , 0x2 , 0x1 , 0x2 , 0x2 , 0x1 , 0x1 , 0x30, 0xc , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 
0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 0x0 , 0x30, 0x6b, 0x31, 0x28, 0x30, 0x26, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x1f, 0x53, 0x61, 0x6d, 0x73, 0x75, 
0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x4f, 0x43, 0x46, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 
0x43, 0x41, 0x31, 0x14, 0x30, 0x12, 0x6 , 0x3 , 0x55, 0x4 , 0xb , 0x13, 0xb , 0x4f, 0x43, 0x46, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43, 0x41, 
0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 
0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 0x20, 0x17, 0xd , 0x31, 
0x36, 0x31, 0x31, 0x32, 0x34, 0x30, 0x32, 0x35, 0x35, 0x31, 0x31, 0x5a, 0x18, 0xf , 0x32, 0x30, 0x36, 0x39, 0x31, 0x32, 0x33, 0x31, 0x31, 0x34, 
0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x6b, 0x31, 0x28, 0x30, 0x26, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x1f, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 
0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x4f, 0x43, 0x46, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43, 
0x41, 0x31, 0x14, 0x30, 0x12, 0x6 , 0x3 , 0x55, 0x4 , 0xb , 0x13, 0xb , 0x4f, 0x43, 0x46, 0x20, 0x52, 0x6f, 0x6f, 0x74, 0x20, 0x43, 0x41, 0x31, 
0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 
0x6f, 0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 0x59, 0x30, 0x13, 0x6 , 0x7 , 
0x2a, 0x86, 0x48, 0xce, 0x3d, 0x2 , 0x1 , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x3 , 0x1 , 0x7 , 0x3 , 0x42, 0x0 , 0x4 , 0x62, 0xc7, 0xff, 
0x2b, 0x1f, 0xac, 0x50, 0x50, 0x50, 0x11, 0x26, 0xee, 0xca, 0xd4, 0xc3, 0x3f, 0x2 , 0xcf, 0x21, 0xed, 0x17, 0xff, 0xcf, 0xc1, 0xd4, 0xbe, 0xdb, 
0xdd, 0xa6, 0xf1, 0x13, 0xdc, 0x34, 0x81, 0x6 , 0x40, 0x7c, 0x8f, 0x16, 0x61, 0x49, 0xa , 0x7c, 0xd7, 0xcf, 0xec, 0x75, 0xe1, 0xd4, 0xce, 0x52, 
0xa , 0x73, 0xa4, 0x7f, 0x5 , 0xab, 0x6a, 0x5b, 0x46, 0x38, 0xa4, 0xde, 0x5f, 0xa3, 0x81, 0x91, 0x30, 0x81, 0x8e, 0x30, 0xe , 0x6 , 0x3 , 0x55, 
0x1d, 0xf , 0x1 , 0x1 , 0xff, 0x4 , 0x4 , 0x3 , 0x2 , 0x1 , 0xc6, 0x30, 0x32, 0x6 , 0x3 , 0x55, 0x1d, 0x1f, 0x4 , 0x2b, 0x30, 0x29, 0x30, 0x27, 
0xa0, 0x25, 0xa0, 0x23, 0x86, 0x21, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x70, 0x72, 0x6f, 0x64, 0x63, 0x61, 0x2e, 0x73, 0x61, 0x6d, 0x73, 
0x75, 0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x63, 0x72, 0x6c, 0x30, 0xf , 0x6 , 0x3 , 0x55, 0x1d, 0x13, 0x1 , 0x1 , 
0xff, 0x4 , 0x5 , 0x30, 0x3 , 0x1 , 0x1 , 0xff, 0x30, 0x37, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 0x7 , 0x1 , 0x1 , 0x4 , 0x2b, 0x30, 0x29, 
0x30, 0x27, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 0x7 , 0x30, 0x1 , 0x86, 0x1b, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6f, 0x63, 0x73, 
0x70, 0x2e, 0x73, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0xc , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 
0xce, 0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 0x0 , 0x3 , 0x48, 0x0 , 0x30, 0x45, 0x2 , 0x20, 0x11, 0x63, 0xd6, 0x92, 0x13, 0x7d, 0x2a, 0xdf, 0x5a, 0xb9, 
0xbf, 0xc0, 0x78, 0xb0, 0x97, 0x33, 0x6 , 0xa3, 0xa9, 0xec, 0xb , 0x3 , 0xf6, 0x8f, 0x19, 0x22, 0xe3, 0x66, 0x1f, 0xb2, 0x30, 0x4b, 0x2 , 0x21, 
0x0 , 0xb7, 0xd1, 0xe7, 0xa8, 0xdc, 0x5e, 0x81, 0x62, 0xb3, 0xf9, 0xc3, 0xc7, 0x4b, 0x50, 0xdb, 0x14, 0xc8, 0xfd, 0xfd, 0x1b, 0xec, 0x5e, 0xad, 
0x58, 0xfa, 0xa3, 0xda, 0xfc, 0x8a, 0x41, 0xe3, 0x51, 0x30, 0x82, 0x2 , 0x87, 0x30, 0x82, 0x2 , 0x2a, 0xa0, 0x3 , 0x2 , 0x1 , 0x2 , 0x2 , 0x14, 
0x4 , 0xc , 0xd1, 0x45, 0x82, 0x14, 0x38, 0x56, 0xbd, 0x43, 0x5a, 0x35, 0x34, 0x8c, 0xee, 0x5c, 0xaa, 0x78, 0x6e, 0x97, 0x30, 0xc , 0x6 , 0x8 , 
0x2a, 0x86, 0x48, 0xce, 0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 0x0 , 0x30, 0x6b, 0x31, 0x28, 0x30, 0x26, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x1f, 0x53, 
0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x4f, 0x43, 0x46, 0x20, 0x52, 
0x6f, 0x6f, 0x74, 0x20, 0x43, 0x41, 0x31, 0x14, 0x30, 0x12, 0x6 , 0x3 , 0x55, 0x4 , 0xb , 0x13, 0xb , 0x4f, 0x43, 0x46, 0x20, 0x52, 0x6f, 0x6f, 
0x74, 0x20, 0x43, 0x41, 0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 
0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 
0x20, 0x17, 0xd , 0x31, 0x37, 0x30, 0x31, 0x30, 0x32, 0x30, 0x34, 0x30, 0x37, 0x34, 0x37, 0x5a, 0x18, 0xf , 0x32, 0x30, 0x36, 0x39, 0x31, 0x32, 
0x33, 0x31, 0x31, 0x34, 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x7e, 0x31, 0x33, 0x30, 0x31, 0x6 , 0x3 , 0x55, 0x4 , 0x3 , 0x13, 0x2a, 0x53, 0x61, 
0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69, 0x63, 0x73, 0x20, 0x4f, 0x43, 0x46, 0x20, 0x48, 0x41, 
0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x53, 0x75, 0x62, 0x43, 0x41, 0x20, 0x76, 0x31, 0x31, 0x1c, 0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 
0xb , 0x13, 0x13, 0x4f, 0x43, 0x46, 0x20, 0x48, 0x41, 0x20, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x20, 0x53, 0x75, 0x62, 0x43, 0x41, 0x31, 0x1c, 
0x30, 0x1a, 0x6 , 0x3 , 0x55, 0x4 , 0xa , 0x13, 0x13, 0x53, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x20, 0x45, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 
0x6e, 0x69, 0x63, 0x73, 0x31, 0xb , 0x30, 0x9 , 0x6 , 0x3 , 0x55, 0x4 , 0x6 , 0x13, 0x2 , 0x4b, 0x52, 0x30, 0x59, 0x30, 0x13, 0x6 , 0x7 , 0x2a, 
0x86, 0x48, 0xce, 0x3d, 0x2 , 0x1 , 0x6 , 0x8 , 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x3 , 0x1 , 0x7 , 0x3 , 0x42, 0x0 , 0x4 , 0x75, 0x94, 0xec, 0x8a, 
0xe9, 0x4f, 0x6c, 0x90, 0xdb, 0x3b, 0x10, 0x3f, 0x71, 0x2e, 0xaa, 0x91, 0x3d, 0xd4, 0x85, 0xbf, 0x8a, 0xe2, 0xa1, 0x74, 0x43, 0xe7, 0x79, 0x72, 
0xa4, 0x46, 0xf5, 0x75, 0xf4, 0xc1, 0x71, 0x48, 0xe1, 0xb4, 0x5d, 0xfc, 0xa2, 0xc , 0xb8, 0x69, 0xa6, 0x5b, 0xc1, 0xd7, 0xc4, 0xcb, 0x2 , 0x6d, 
0x30, 0xaf, 0xc1, 0x38, 0x10, 0x2e, 0xcc, 0x7f, 0xb3, 0xe5, 0x92, 0x73, 0xa3, 0x81, 0x94, 0x30, 0x81, 0x91, 0x30, 0xe , 0x6 , 0x3 , 0x55, 0x1d, 
0xf , 0x1 , 0x1 , 0xff, 0x4 , 0x4 , 0x3 , 0x2 , 0x1 , 0xc6, 0x30, 0x32, 0x6 , 0x3 , 0x55, 0x1d, 0x1f, 0x4 , 0x2b, 0x30, 0x29, 0x30, 0x27, 0xa0, 
0x25, 0xa0, 0x23, 0x86, 0x21, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x70, 0x72, 0x6f, 0x64, 0x63, 0x61, 0x2e, 0x73, 0x61, 0x6d, 0x73, 0x75, 
0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x63, 0x72, 0x6c, 0x30, 0x12, 0x6 , 0x3 , 0x55, 0x1d, 0x13, 0x1 , 0x1 , 0xff, 
0x4 , 0x8 , 0x30, 0x6 , 0x1 , 0x1 , 0xff, 0x2 , 0x1 , 0x0 , 0x30, 0x37, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 0x7 , 0x1 , 0x1 , 0x4 , 0x2b, 
0x30, 0x29, 0x30, 0x27, 0x6 , 0x8 , 0x2b, 0x6 , 0x1 , 0x5 , 0x5 , 0x7 , 0x30, 0x1 , 0x86, 0x1b, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6f, 
0x63, 0x73, 0x70, 0x2e, 0x73, 0x61, 0x6d, 0x73, 0x75, 0x6e, 0x67, 0x69, 0x6f, 0x74, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0xc , 0x6 , 0x8 , 0x2a, 
0x86, 0x48, 0xce, 0x3d, 0x4 , 0x3 , 0x2 , 0x5 , 0x0 , 0x3 , 0x49, 0x0 , 0x30, 0x46, 0x2 , 0x21, 0x0 , 0xf5, 0x22, 0x21, 0xd2, 0x9a, 0x3c, 0x4e, 
0xb8, 0x69, 0x82, 0x95, 0xc7, 0x43, 0xea, 0x2c, 0xe3, 0x8 , 0xaa, 0x0 , 0x29, 0xb , 0xa2, 0xae, 0x34, 0xff, 0xfa, 0xff, 0xbe, 0x54, 0xa8, 0x83, 
0x95, 0x2 , 0x21, 0x0 , 0x8f, 0xfd, 0xf2, 0x46, 0x91, 0x59, 0xb1, 0xb8, 0x2e, 0x4f, 0x53, 0xdf, 0xda, 0x46, 0xfe, 0xa5, 0x1c, 0x3a, 0x51, 0xa2, 
0xbf, 0x43, 0xb9, 0x81, 0xbe, 0x9d, 0x3e, 0x7b, 0xef, 0x76, 0xa6, 0x6e, };

// ~pkss
#define TAG "SSSHANDLE"

OCStackResult SSSRootCAHandler(OicUuid_t* subjectUuid,uint16_t credId)
{
	OCStackResult res = OC_STACK_ERROR;
	unsigned char *buf = NULL;
	unsigned int buflen = SEE_MAX_BUF_SIZE;
	printf("NOT USE_SAVE_TRUST go to CredSaveTrustCertChain()\n");
	buf = malloc(SEE_MAX_BUF_SIZE);

	if(buf == NULL){
		THINGS_LOG_D_ERROR(THINGS_ERROR, TAG, "Failed to allocate buffer");
		return OC_STACK_ERROR;
	}

#if 1 // pkss
    /* Load rootca certificate from SSS */
    if((res = see_get_certificate(buf, &buflen,
                                  g_iotivity_cacert_index, CERT_PEM)) != 0) {
	    THINGS_LOG_D_ERROR(THINGS_ERROR, TAG, "Failed to get certificate");
	    free(buf);
		return OC_STACK_ERROR;
    }
#else
	buflen = 1260;
	memcpy(buf, g_iot_ss_cacert, buflen);

#endif
	printf("root calen(%d)\n", buflen);
	printf("=======================begin(cacert)=======================\n");
	int idx = 0;
	for(; idx < buflen; idx++) {
		printf("%3d ", buf[idx]);
		if (idx % 32 == 0) printf("\n");
	}
	printf("\n");
	printf("=======================end(cacert)=======================\n");
	
	res = CredSaveTrustCertChain(subjectUuid, buf, buflen, OIC_ENCODING_PEM, TRUST_CA, &credId);
	if(OC_STACK_OK != res) {
		THINGS_LOG_D_ERROR(THINGS_ERROR, TAG, "SRPCredSaveTrustCertChain #1 error");
		free(buf);
	    return res;
	}
	THINGS_LOG_D(THINGS_DEBUG, TAG, "global_ocf_connect_cert_root.der HW saved w/ cred ID=%d", credId);
	free(buf);
	return res;
}

#if defined(CONFIG_TLS_WITH_SSS) && defined(CONFIG_HW_ECDSA)

//this callback will be invoked to get key context based on key usage
void* OCGetHwKey(const char* service, const char* usage, const char* keytype)
{
	THINGS_LOG(DEBUG,TAG, "in");

	if(service == NULL || usage == NULL)
		return NULL;

	mbedtls_pk_context *pkey;

	pkey = malloc(sizeof(mbedtls_pk_context));

	if(pkey == NULL) {
		printf("%s : pkey context alloc failed\n", __func__);
		return NULL;
	}
	THINGS_LOG(DEBUG,TAG, "out");
	return (void *)pkey;
}

//this callback will free key context that was retreived from TZ
int OCFreeHwKey(void* keyContext)
{
	THINGS_LOG(DEBUG,TAG, "in");

	if(keyContext == NULL)
		return -1;

	free(keyContext);

	THINGS_LOG(DEBUG,TAG, "out");

	return 0;
}

//this callback will be invoked to load own certificate in case of TZ
int OCGetOwnCertFromHw(const void* keyContext, uint8_t** certChain, size_t* certChainLen)
{
	int ret;
	unsigned char *buf = NULL;
	unsigned int buflen = SEE_BUF_MAX_SIZE;

	unsigned char *t1_buf = NULL;
	unsigned int t1_buflen, t2_buflen;

	(void *) keyContext;

	THINGS_LOG(DEBUG,TAG, "in");

	if(certChain == NULL || certChainLen == NULL)
		return -1;

	/* If input buffer's address is same with previous allocated memory,
       it will return the success value and iotivity stack use previous memory. */
	if(*certChain && *certChain == g_certChain) {
		return 0;
	}

	buf = (unsigned char *)malloc(buflen);

	if(buf == NULL)
		return -1;
#if 1 // pkss
	if((ret = see_get_certificate(buf, &buflen,
				      g_iotivity_cert_index, CERT_PEM)) != 0) {
		printf("see_get_cert fail %d\n", ret);
		free(buf);
		return -1;
	}
#else
	buflen = 664;
	memcpy(buf, g_iot_ss_cert, buflen);
#endif
	printf("[pkss] len(%d)\n", buflen);
	printf("=======================begin(cert)=======================\n");
	int idx = 0;
	for(; idx < buflen; idx++) {
		printf("%3d ", buf[idx]);
		if (idx % 32 == 0 ) printf("\n");
	}
	printf("\n");
	printf("=======================end(cert)=======================\n");
	t1_buf = malloc(buflen);

	if(t1_buf == NULL){
		free(buf);
		return -1;
	}

	memcpy(t1_buf, buf, buflen);
	t1_buflen = buflen;

	buflen = SEE_BUF_MAX_SIZE;
#if 1 // pkss
	if((ret = see_get_certificate(buf, &buflen,
				      g_iotivity_cacert_index, CERT_PEM)) != 0) {
		printf("see_get_cert fail %d\n", ret);
		free(buf);
		free(t1_buf);
		return -1;
	}
#else
	buflen = 1260;
	memcpy(buf, g_iot_ss_cacert, buflen);	
#endif
	printf("[pkss] len(%d)\n", buflen);
	printf("=======================begin(cacert)=======================\n");
	idx = 0;
	for(; idx < buflen; idx++) {
		printf("%3d ", buf[idx]);
		if (idx % 32 == 0) printf("\n");
	}
	printf("\n");
	printf("=======================end(cacert)=======================\n");
	
	int cnt = 0, i = 0;
	while(i++ <= buflen) {
		if(buf[i] == 0x30 && buf[i+1] == 0x82)
			cnt++;

		if(cnt == 2)
			break;
	}

	if(i > buflen) {
		free(buf);
		free(t1_buf);
		return -1;
	}

	t2_buflen = buflen - i;

	*certChain = malloc(t1_buflen + t2_buflen);

	if(*certChain == NULL) {
		free(buf);
		free(t1_buf);
		return -1;
	}

	memcpy(*certChain, t1_buf, t1_buflen);
	memcpy(*certChain + t1_buflen, buf + i, t2_buflen);

	*certChainLen = t1_buflen + t2_buflen;

	free(buf);
	free(t1_buf);

	g_certChain = *certChain;

	THINGS_LOG(DEBUG,TAG, "out");
	return 0;
}

//this callback will be invoked to load private key in case of TZ
int OCSetupPkContextFromHw(mbedtls_pk_context* ctx, void* keyContext)
{
	THINGS_LOG(DEBUG,TAG, "in");

	if(ctx == NULL || keyContext == NULL)
		return -1;

	mbedtls_pk_context* pkey = keyContext;

	mbedtls_pk_init(pkey);
	const mbedtls_pk_info_t *pk_info;

	if((pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))== NULL) {
		mbedtls_pk_free(pkey);
		free(pkey);
		return -1;
	}

	if(mbedtls_pk_setup(pkey, pk_info)) {
		mbedtls_pk_free(pkey);
		free(pkey);
		return -1;
	}

	mbedtls_ecdsa_init((mbedtls_ecdsa_context*) pkey->pk_ctx);
	((mbedtls_ecdsa_context *)(pkey->pk_ctx))->key_index = g_iotivity_key_index;
	((mbedtls_ecdsa_context *)(pkey->pk_ctx))->grp.id = g_iotivity_key_curve;

	ctx->pk_info = pkey->pk_info;
	ctx->pk_ctx = pkey->pk_ctx;

	THINGS_LOG(DEBUG,TAG, "out");

	return 0;
}

void iotivity_sss_factorykey(unsigned int cacrt_idx, unsigned int crt_idx,
							 unsigned int key_idx, unsigned int key_type)
{
	THINGS_LOG(DEBUG,TAG, "called");

	/*
	 * This setting is only for connecting with the product cloud.
	 */

	/* 1. Setting up the factory certificate index */
	g_iotivity_cert_index = crt_idx;
	g_iotivity_cacert_index = cacrt_idx;

	/* 2. Setting up the factory key index and curve type */
	g_iotivity_key_curve = key_type;
	g_iotivity_key_index = key_idx;
}
#endif /* CONFIG_TLS_WITH_SSS && CONFIG_HW_ECDSA */

int InitializeSSSKeyHandlers(void)
{
	THINGS_LOG(DEBUG,TAG, "called");

#if defined(CONFIG_TLS_WITH_SSS) && defined(CONFIG_HW_ECDSA)
	iotivity_sss_factorykey(FACTORYKEY_IOTIVITY_SUB_CA_CERT,
							FACTORYKEY_IOTIVITY_ECC_CERT,
							FACTORYKEY_IOTIVITY_ECC,
							MBEDTLS_ECP_DP_SECP256R1);

	if (SetHwPkixCallbacks(OCGetHwKey, OCFreeHwKey,
						   OCGetOwnCertFromHw, OCSetupPkContextFromHw)) {
		printf("SetHwPkixCallbacks failed\n");
		return -1;
	}
#endif

	uint16_t cipher = MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
	if (CA_STATUS_OK != CASelectCipherSuite(cipher, CA_ADAPTER_TCP)) {
		printf("[%s]  CASelectCipherSuite returned an error\n", TAG);
	}

	return 0;
}

