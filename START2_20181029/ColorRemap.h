#ifndef __COLOR_REMAP_H__
#define __COLOR_REMAP_H__

void ColorMapInit(int level);
void ColorMapSet(int level);
void GrayScaleMapBuild(int level);
void LuminanceMapBuild(int level);
void ChrominanceMapBuild(void);

#endif
