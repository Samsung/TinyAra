/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
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
#ifndef __BOARD_ARTIK05X_INCLUDE_SYMTAB_H__
#define __BOARD_ARTIK05X_INCLUDE_SYMTAB_H__
#define FLASH_BASE 0x04000000
int up_symtab_flashdrv_init(void);
void up_symtab_flashdrv_read(void *buffer, uint32_t offset_addr, int bytes);

#endif /* __BOARD_ARTIK05X_INCLUDE_SYMTAB_H__ */
