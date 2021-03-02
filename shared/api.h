
/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR v1.8 (2021-02-25) - shared library
 * --------------------------------------------------------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2020
 *
 */

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cmath>
#include <cstring>

#include "../msdfgen.h"
#include "../msdfgen-ext.h"

#include "../core/ShapeDistanceFinder.h"

//#define SDF_ERROR_ESTIMATE_PRECISION 19
//#define DEFAULT_ANGLE_THRESHOLD 3.


enum Format {
    AUTO,
    PNG,
    BMP,
    TIFF,
    TEXT,
    TEXT_FLOAT,
    BINARY,
    BINARY_FLOAT,
    BINARY_FLOAT_BE
};
enum MSDF_MODE{
    SINGLE,
    PSEUDO,
    MULTI,
    MULTI_AND_TRUE,
    METRICS
};
enum MSDF_RANGE_MODE{
    RANGE_UNIT,
    RANGE_PX
};
enum MSDF_ORIENTATION{
    KEEP,
    REVERSE,
    GUESS
};
enum MSDF_COLORING_STRATEGY {
    SIMPLE,
    INK_TRAP
};


struct MSDFSettings {
    MSDF_ORIENTATION orientation = MSDF_ORIENTATION::KEEP;
    float outputDistanceShift = 0.0f;

    msdfgen::Vector2 translate = 0.;
    double errorCorrectionThreshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD;
    double range = 2;
    
    msdfgen::FillRule fillRule = msdfgen::FillRule::FILL_NONZERO;
    
    MSDF_RANGE_MODE rangeMode = MSDF_RANGE_MODE::RANGE_PX;
    
    double normalizedRange() {
        switch(rangeMode) {
            case MSDF_RANGE_MODE::RANGE_PX:
                return range / std::min(scale.x, scale.y);
            case MSDF_RANGE_MODE::RANGE_UNIT:
                return range;
            default:
                return 0;
                break;
        }
    }
    
    double averageScale() {
        return 0.5*(scale.x+scale.y);
    }
    
    
    bool scaleSpecified = false;
    msdfgen::Vector2 scale = 1.;
    
    bool legacyMode = false;
    bool overlapSupport = false;

};

struct MSDFState {
    bool scanlinePass;
    int width;
    int height;
};

struct MSDF {
    MSDF() : data(nullptr) {}
    ~MSDF() {
        if (data != nullptr)
            delete data;
    }
    msdfgen::Shape shape;
    MSDF_MODE mode = MSDF_MODE::MULTI_AND_TRUE;
    MSDFSettings settings;
    MSDFState state;
    msdfgen::BitmapBase* data;
};

typedef void (*edgeColoringFunc_t)(msdfgen::Shape &, double, unsigned long long);

#ifdef __cplusplus
extern "C" {
#endif
    bool MSDF_Shape_LoadFromFontFile(msdfgen::Shape& shape, const char* fontFileName, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode);
    bool MSDF_Shape_LoadFromSVGFile(msdfgen::Shape& shape, const char* svgFileName, int svgPathIndex);
    bool MSDF_Shape_LoadFromDescriptionFile(msdfgen::Shape& shape, const char* descriptionFile, bool* skipColoring);
    bool MSDF_Shape_PreprocessGeometry(msdfgen::Shape& shape);
    bool MSDF_Shape_Validate(msdfgen::Shape& shape);
    bool MSDF_Shape_Export(msdfgen::Shape& shape, const char* fileName);
    void MSDF_Shape_Normalize(msdfgen::Shape& shape);
    void MSDF_Shape_FlipY(msdfgen::Shape& shape, bool flipped);
    bool MSDF_Shape_LoadFromFreetypeFont(msdfgen::Shape& shape, msdfgen::FontHandle *font, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode);
    
    void MSDF_AutoFrame(MSDF& msdf);

    msdfgen::BitmapBase* MSDF_Bitmap_Create(int channels, int width, int height);

    void MSDF_Bitmap_Delete(msdfgen::BitmapBase* bitmap);
    
    void* MSDF_Bitmap_GetPixelData(msdfgen::BitmapBase& bitmap, int& width, int& height, int& channelCount);


    bool MSDF_TestRender(MSDF& msdf, msdfgen::Bitmap<float, 1>& output);
    bool MSDF_TestRenderMulti(MSDF& msdf, msdfgen::BitmapBase& renderDestination);
    bool MSDF_ApplyOrientation(MSDF& msdf);

    bool MSDF_ApplyOutputDistanceShift(MSDF& msdf);
    bool MSDF_ApplyScanlinePass(MSDF& msdf);

    bool MSDF_Save(MSDF& msdf, const char *filename, Format format);
    
    void MSDF_Initialize(MSDF& msdf, MSDF_MODE mode, int width, int height);
    
    msdfgen::Shape* MSDF_GetShape(MSDF& msdf);
    msdfgen::BitmapBase* MSDF_GetData(MSDF& msdf);
    
    void MSDF_ColorEdges(MSDF& msdf, edgeColoringFunc_t edgeColoring, unsigned long long coloringSeed, bool skipColoring, const char* edgeAssignment, double angleThreshold);
    
    bool MSDF_Generate(MSDF& msdf);
    
    bool MSDF_ConvertTo8Bit(MSDF& msdf);
    
    void MSDF_PrintMetrics(MSDF& msdf, bool autoFrame);
    
    MSDF* MSDF_Create(MSDF_MODE mode, int width, int height);
    void MSDF_Delete(MSDF* msdf);
    
    const char* MSDF_GetErrorMessage();
    
    edgeColoringFunc_t MSDF_GetColoringStrategy(MSDF_COLORING_STRATEGY coloringStrategy);
    
#ifdef __cplusplus
}

#endif

