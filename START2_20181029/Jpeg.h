#ifndef __JPEG_H__
#define __JPEG_H__

#include "main.h"

void JpegInitialize(struct jpeg_compress_struct *comp, struct jpeg_error_mgr *err, unsigned char **dst, unsigned long *length);
void JpegStart(struct jpeg_compress_struct *comp, ImageAttributionsTypeDef *attr);
void JpegReset(struct jpeg_compress_struct *comp, struct jpeg_error_mgr *err, unsigned char **dst, unsigned long *length);

#endif
