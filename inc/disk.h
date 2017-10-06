//
// Copyright (c) 2017 The Altra64 project contributors
// Portions (c) 2011 KRIK
// See LICENSE file in the project root for full license information.
//

#ifndef _DISK_H
#define	_DISK_H

#include "types.h"

u8 diskGetInterface();
u8 diskInit();
u8 diskRead(u32 saddr, void *buff, u16 slen);
u8 diskWrite(u32 saddr, u8 *buff, u16 slen);
void diskSetInterface(u32 interface);



#define WAIT 1024

#define DISK_IFACE_SPI 0
#define DISK_IFACE_SD 1



#endif	/* _DISK_H */

