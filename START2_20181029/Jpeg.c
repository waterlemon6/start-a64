#include <stdio.h>
#include "Jpeg.h"

void JpegInitialize(struct jpeg_compress_struct *comp, struct jpeg_error_mgr *err, unsigned char **dst, unsigned long *length)
{
    comp->err = jpeg_std_error(err);
    jpeg_create_compress(comp);
    jpeg_mem_dest(comp, dst, length);
}

void JpegStart(struct jpeg_compress_struct *comp, ImageAttributionsTypeDef *attr)
{
    comp->image_width = (JDIMENSION) attr->width;
    comp->image_height = (JDIMENSION) attr->height;
    if(attr->depth == 3) {
        comp->input_components = 3;
        comp->in_color_space = JCS_YCbCr;
    }
    else if(attr->depth == 1) {
        comp->input_components = 1;
        comp->in_color_space = JCS_GRAYSCALE;
    }
    jpeg_set_defaults(comp);
    comp->density_unit = 1;
    comp->X_density = (UINT16) attr->dpi;
    comp->Y_density = (UINT16) attr->dpi;
    jpeg_set_quality(comp, 35, TRUE);
    comp->dct_method = JDCT_FASTEST;
    jpeg_start_compress(comp, TRUE);
}

void JpegReset(struct jpeg_compress_struct *comp, struct jpeg_error_mgr *err, unsigned char **dst, unsigned long *length)
{
    jpeg_destroy_compress(comp);
    *dst = NULL;
    *length = 0;
    JpegInitialize(comp, err, dst, length);
}