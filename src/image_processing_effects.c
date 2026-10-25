#include "global.h"
#include "image_processing_effects.h"
#include "constants/rgb.h"

// IWRAM common
COMMON_DATA u8 gCanvasColumnStart = 0;
COMMON_DATA u16 *gCanvasPixels = NULL;
COMMON_DATA u8 gCanvasRowEnd = 0;
COMMON_DATA u8 gCanvasHeight = 0;
COMMON_DATA u8 gCanvasColumnEnd = 0;
COMMON_DATA u8 gCanvasRowStart = 0;
COMMON_DATA u8 gCanvasMonPersonality = 0;
COMMON_DATA u8 gCanvasWidth = 0;
COMMON_DATA u16 *gCanvasPalette = NULL;
COMMON_DATA u16 gCanvasPaletteStart = 0;


#define MAX_DIMENSION 64

#include "data/pointillism_points.h"














struct PointillismPoint
{
    u8 column;
    u8 row;
    u16 delta;
};

























