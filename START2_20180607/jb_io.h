#ifndef JB_IO_H
#define JB_IO_H

#include "KernelInterface.h"

#define READ_TDO	TDO_READ()
#define SET_TCK(x)	TCK_WRITE(x)
#define SET_TMS(x)	TMS_WRITE(x)
#define SET_TDI(x)	TDI_WRITE(x)

#endif
