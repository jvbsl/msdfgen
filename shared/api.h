
/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR v1.8 (2021-02-25) - shared library
 * --------------------------------------------------------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2020
 *
 */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #ifdef MSDFGEN_SHARED
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define DLL_EXPORT 
#endif


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
    MSDFSettings()
        : orientation(MSDF_ORIENTATION::KEEP),
          outputDistanceShift(0.0f),
          translate(0.),
          errorCorrectionThreshold(MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD),
          range(2),
          fillRule(msdfgen::FillRule::FILL_NONZERO),
          rangeMode(MSDF_RANGE_MODE::RANGE_PX),
          scaleSpecified(false),
          scale(1.),
          legacyMode(false),
          overlapSupport(false)
    {
    }
    MSDF_ORIENTATION orientation;
    float outputDistanceShift;

    msdfgen::Vector2 translate;
    double errorCorrectionThreshold;
    double range;
    
    msdfgen::FillRule fillRule;
    
    MSDF_RANGE_MODE rangeMode;
    
    bool scaleSpecified;
    msdfgen::Vector2 scale;
    
    bool legacyMode;
    bool overlapSupport;
    
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

};

struct MSDFState {
    bool scanlinePass;
    int width;
    int height;
};

struct MSDF {
    MSDF() : state({}), data(nullptr) {}
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
    DLL_EXPORT bool MSDF_Shape_LoadFromFontFile(msdfgen::Shape& shape, const char* fontFileName, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode);
    DLL_EXPORT bool MSDF_Shape_LoadFromSVGFile(msdfgen::Shape& shape, const char* svgFileName, int svgPathIndex);
    DLL_EXPORT bool MSDF_Shape_LoadFromDescriptionFile(msdfgen::Shape& shape, const char* descriptionFile, bool* skipColoring);
    DLL_EXPORT bool MSDF_Shape_PreprocessGeometry(msdfgen::Shape& shape);
    DLL_EXPORT bool MSDF_Shape_Validate(msdfgen::Shape& shape);
    DLL_EXPORT bool MSDF_Shape_Export(msdfgen::Shape& shape, const char* fileName);
    DLL_EXPORT void MSDF_Shape_Normalize(msdfgen::Shape& shape);
    DLL_EXPORT void MSDF_Shape_FlipY(msdfgen::Shape& shape, bool flipped);
    DLL_EXPORT bool MSDF_Shape_LoadFromFreetypeFont(msdfgen::Shape& shape, msdfgen::FontHandle *font, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode);
    
    DLL_EXPORT void MSDF_AutoFrame(MSDF& msdf);

    DLL_EXPORT msdfgen::BitmapBase* MSDF_Bitmap_Create(int channels, int width, int height);

    DLL_EXPORT void MSDF_Bitmap_Delete(msdfgen::BitmapBase* bitmap);
    
    DLL_EXPORT void* MSDF_Bitmap_GetPixelData(msdfgen::BitmapBase& bitmap, int& width, int& height, int& channelCount);


    DLL_EXPORT bool MSDF_TestRender(MSDF& msdf, msdfgen::Bitmap<float, 1>& output);
    DLL_EXPORT bool MSDF_TestRenderMulti(MSDF& msdf, msdfgen::BitmapBase& renderDestination);
    DLL_EXPORT bool MSDF_ApplyOrientation(MSDF& msdf);

    DLL_EXPORT bool MSDF_ApplyOutputDistanceShift(MSDF& msdf);
    DLL_EXPORT bool MSDF_ApplyScanlinePass(MSDF& msdf);

    DLL_EXPORT bool MSDF_Save(MSDF& msdf, const char *filename, Format format);
    
    DLL_EXPORT void MSDF_Initialize(MSDF& msdf, MSDF_MODE mode, int width, int height);
    
    DLL_EXPORT msdfgen::Shape* MSDF_GetShape(MSDF& msdf);
    DLL_EXPORT msdfgen::BitmapBase* MSDF_GetData(MSDF& msdf);
    
    DLL_EXPORT void MSDF_ColorEdges(MSDF& msdf, edgeColoringFunc_t edgeColoring, unsigned long long coloringSeed, bool skipColoring, const char* edgeAssignment, double angleThreshold);
    
    DLL_EXPORT bool MSDF_Generate(MSDF& msdf);
    
    DLL_EXPORT bool MSDF_ConvertTo8Bit(MSDF& msdf);
    
    DLL_EXPORT void MSDF_PrintMetrics(MSDF& msdf, bool autoFrame);
    
    DLL_EXPORT MSDF* MSDF_Create(MSDF_MODE mode, int width, int height);
    DLL_EXPORT void MSDF_Delete(MSDF* msdf);
    
    DLL_EXPORT const char* MSDF_GetErrorMessage();
    
    DLL_EXPORT edgeColoringFunc_t MSDF_GetColoringStrategy(MSDF_COLORING_STRATEGY coloringStrategy);
    
#ifdef __cplusplus
}

#endif

