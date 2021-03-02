
#ifdef MSDFGEN_SHARED

#include "api.h"
#include <cctype>

void convert8bit(const msdfgen::BitmapRef<float, 1> &bitmap) {
    const float *end = bitmap.pixels+1*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = msdfgen::pixelByteToFloat(msdfgen::pixelFloatToByte(*p));
}

void convert8bit(const msdfgen::BitmapRef<float, 3> &bitmap) {
    const float *end = bitmap.pixels+3*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = msdfgen::pixelByteToFloat(msdfgen::pixelFloatToByte(*p));
}

void convert8bit(const msdfgen::BitmapRef<float, 4> &bitmap) {
    const float *end = bitmap.pixels+4*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = msdfgen::pixelByteToFloat(msdfgen::pixelFloatToByte(*p));
}
static void parseColoring(msdfgen::Shape &shape, const char *edgeAssignment) {
    unsigned c = 0, e = 0;
    if (shape.contours.size() < c) return;
    msdfgen::Contour *contour = &shape.contours[c];
    bool change = false;
    bool clear = true;
    for (const char *in = edgeAssignment; *in; ++in) {
        switch (*in) {
            case ',':
                if (change)
                    ++e;
                if (clear)
                    while (e < contour->edges.size()) {
                        contour->edges[e]->color = msdfgen::WHITE;
                        ++e;
                    }
                ++c, e = 0;
                if (shape.contours.size() <= c) return;
                contour = &shape.contours[c];
                change = false;
                clear = true;
                break;
            case '?':
                clear = false;
                break;
            case 'C': case 'M': case 'W': case 'Y': case 'c': case 'm': case 'w': case 'y':
                if (change) {
                    ++e;
                    change = false;
                }
                if (e < contour->edges.size()) {
                    contour->edges[e]->color = msdfgen::EdgeColor(
                        (*in == 'C' || *in == 'c')*msdfgen::CYAN|
                        (*in == 'M' || *in == 'm')*msdfgen::MAGENTA|
                        (*in == 'Y' || *in == 'y')*msdfgen::YELLOW|
                        (*in == 'W' || *in == 'w')*msdfgen::WHITE);
                    change = true;
                }
                break;
        }
    }
}

static bool writeTextBitmap(FILE *file, const float *values, int cols, int rows) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int v = msdfgen::clamp(int((*values++)*0x100), 0xff);
            fprintf(file, col ? " %02X" : "%02X", v);
        }
        fprintf(file, "\n");
    }
    return true;
}

static bool writeTextBitmapFloat(FILE *file, const float *values, int cols, int rows) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            fprintf(file, col ? " %.9g" : "%.9g", *values++);
        }
        fprintf(file, "\n");
    }
    return true;
}

static bool writeBinBitmap(FILE *file, const float *values, int count) {
    for (int pos = 0; pos < count; ++pos) {
        unsigned char v = msdfgen::clamp(int((*values++)*0x100), 0xff);
        fwrite(&v, 1, 1, file);
    }
    return true;
}

#ifdef __BIG_ENDIAN__
static bool writeBinBitmapFloatBE(FILE *file, const float *values, int count)
#else
static bool writeBinBitmapFloat(FILE *file, const float *values, int count)
#endif
{
    return (int) fwrite(values, sizeof(float), count, file) == count;
}

#ifdef __BIG_ENDIAN__
static bool writeBinBitmapFloat(FILE *file, const float *values, int count)
#else
static bool writeBinBitmapFloatBE(FILE *file, const float *values, int count)
#endif
{
    for (int pos = 0; pos < count; ++pos) {
        const unsigned char *b = reinterpret_cast<const unsigned char *>(values++);
        for (int i = sizeof(float)-1; i >= 0; --i)
            fwrite(b+i, 1, 1, file);
    }
    return true;
}

static bool cmpExtension(const char *path, const char *ext) {
    for (const char *a = path+strlen(path)-1, *b = ext+strlen(ext)-1; b >= ext; --a, --b)
        if (a < path || std::toupper(*a) != std::toupper(*b))
            return false;
    return true;
}

template <int N>
static const char * writeOutput(const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, Format &format) {
    if (filename) {
        if (format == AUTO) {
            if (cmpExtension(filename, ".png")) format = PNG;
            else if (cmpExtension(filename, ".bmp")) format = BMP;
            else if (cmpExtension(filename, ".tif") || cmpExtension(filename, ".tiff")) format = TIFF;
            else if (cmpExtension(filename, ".txt")) format = TEXT;
            else if (cmpExtension(filename, ".bin")) format = BINARY;
            else
                return "Could not deduce format from output file name.";
        }
        switch (format) {
            case PNG: return msdfgen::savePng(bitmap, filename) ? NULL : "Failed to write output PNG image.";
            case BMP: return msdfgen::saveBmp(bitmap, filename) ? NULL : "Failed to write output BMP image.";
            case TIFF: return msdfgen::saveTiff(bitmap, filename) ? NULL : "Failed to write output TIFF image.";
            case TEXT: case TEXT_FLOAT: {
                FILE *file = fopen(filename, "w");
                if (!file) return "Failed to write output text file.";
                if (format == TEXT)
                    writeTextBitmap(file, bitmap.pixels, N*bitmap.width, bitmap.height);
                else if (format == TEXT_FLOAT)
                    writeTextBitmapFloat(file, bitmap.pixels, N*bitmap.width, bitmap.height);
                fclose(file);
                return NULL;
            }
            case BINARY: case BINARY_FLOAT: case BINARY_FLOAT_BE: {
                FILE *file = fopen(filename, "wb");
                if (!file) return "Failed to write output binary file.";
                if (format == BINARY)
                    writeBinBitmap(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                else if (format == BINARY_FLOAT)
                    writeBinBitmapFloat(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                else if (format == BINARY_FLOAT_BE)
                    writeBinBitmapFloatBE(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                fclose(file);
                return NULL;
            }
            default:;
        }
    } else {
        if (format == AUTO || format == TEXT)
            writeTextBitmap(stdout, bitmap.pixels, N*bitmap.width, bitmap.height);
        else if (format == TEXT_FLOAT)
            writeTextBitmapFloat(stdout, bitmap.pixels, N*bitmap.width, bitmap.height);
        else
            return "Unsupported format for standard output.";
    }
    return NULL;
}

template <int N>
static void invertColor(const msdfgen::BitmapRef<float, N> &bitmap) {
    const float *end = bitmap.pixels+N*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = 1.f-*p;
}

enum ERROR_CODES {
    SUCCESS = 0,
    ERROR_MESSAGE_QUEUE_FULL,
    NO_CHARACTER_SPECIFIED,
    LOAD_FONT_FAILED,
    LOAD_GLYPH_FAILED,
    LOAD_SVG_FAILED,
    LOAD_SHAPE_DESCRIPTION_FAILED,
    SHAPE_DESCRIPTION_PARSE_ERROR,
    SHAPE_PROCESSING_FAILED,
    SHAPE_PROCESSING_NOT_AVAILABLE,
    INVALID_GEOMETRY,
    PIXEL_RANGE_FIT_FAILED,
    OUTPUT_FILE_WRITE_FAILED,
    MISMATCHING_BITMAP_ELEMENT_COUNT,
    MISMATCHING_BITMAP_ELEMENT_TYPE,
    MISMATCHING_BITMAP,
    ARGUMENT_OUT_OF_RANGE,
    INVALID_NULL_ARGUMENT,
};

void pushError(int);

template <typename T, int N>
static bool TryConvertGenericBitmap(msdfgen::BitmapBase* bitmapBase, msdfgen::Bitmap<T, N>*& output) {
    output = nullptr;
    if (bitmapBase == nullptr) {
        pushError(INVALID_NULL_ARGUMENT);
        return false;
    }
    if (N != bitmapBase->elementsPerPixel()) {
        pushError(MISMATCHING_BITMAP_ELEMENT_COUNT);
        return false;
    }
    if (msdfgen::TypeToElementTypeMapping<T>::value != bitmapBase->elementType()) {
        pushError(MISMATCHING_BITMAP_ELEMENT_TYPE);
        return false;
    }
    
    output = dynamic_cast<msdfgen::Bitmap<T, N>*>(bitmapBase);
    
    if (output == nullptr) {
        pushError(MISMATCHING_BITMAP);
        return false;
    }
        
    return true;
}

// ##################################################################
#include <algorithm>



const char* errorMessages[] = {
    "Successful",
    "Error message queue full",
    "No character specified! Use -font <file.ttf/otf> <character code>. Character code can be a Unicode index (65, 0x41), a character in apostrophes ('A'), or a glyph index prefixed by g (g36, g0x24).",
    "Failed to load font file.",
    "Failed to load glyph from font file.",
    "Failed to load shape from SVG file.",
    "Failed to load shape description file.",
    "Parse error in shape description.",
    "Shape geometry preprocessing failed, skipping.",
    "Shape geometry preprocessing is not available in this version because the Skia library is not present.",
    "The geometry of the loaded shape is invalid.",
    "Cannot fit the specified pixel range.",
    "Failed to write output file.",
    "Mismatching bitmap elment count",
    "Mismatching bitmap elment type",
    "Mismatching bitmap",
    "Argument out of range",
    "Invalid passed null argument"
};

constexpr int MAX_ERROR_MESSAGES = 32;

int errors[MAX_ERROR_MESSAGES];
int currentErrorIndex = 0;



msdfgen::BitmapBase* MSDF_Bitmap_Create(int channels, int width, int height) {
    switch (channels) {
        case 1:
            return new msdfgen::Bitmap<float, 1>(width, height);
        case 2:
            return new msdfgen::Bitmap<float, 2>(width, height);
        case 3:
            return new msdfgen::Bitmap<float, 3>(width, height);
        case 4:
            return new msdfgen::Bitmap<float, 4>(width, height);
    }
    pushError(ARGUMENT_OUT_OF_RANGE);
    return nullptr;
}
void MSDF_Bitmap_Delete(msdfgen::BitmapBase* bitmap) {
    if (bitmap)
        delete bitmap;
}

bool MSDF_Shape_LoadFromFontFile(msdfgen::Shape& shape, const char* fontFileName, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode) {
    if (!glyphIndex && !unicode) {
        pushError(NO_CHARACTER_SPECIFIED);
        return false;
    }
    msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype();
    if (!ft) return false;
    msdfgen::FontHandle *font = loadFont(ft, fontFileName);
    if (!font) {
        deinitializeFreetype(ft);
        pushError(LOAD_FONT_FAILED);
        return false;
    }
    if (unicode)
        getGlyphIndex(glyphIndex, font, unicode);
    double glyphAdvance = 0;
    if (!loadGlyph(shape, font, glyphIndex, &glyphAdvance)) {
        destroyFont(font);
        deinitializeFreetype(ft);
        pushError(LOAD_GLYPH_FAILED);
        return false;
    }
    destroyFont(font);
    deinitializeFreetype(ft);
    return true;
}
bool MSDF_Shape_LoadFromFreetypeFont(msdfgen::Shape& shape, msdfgen::FontHandle *font, msdfgen::GlyphIndex glyphIndex, msdfgen::unicode_t unicode) {
    if (!glyphIndex && !unicode) {
        pushError(NO_CHARACTER_SPECIFIED);
        return false;
    }
    if (!font) {
        pushError(INVALID_NULL_ARGUMENT);
        return false;
    }
    if (unicode)
        getGlyphIndex(glyphIndex, font, unicode);
    double glyphAdvance = 0;
    if (loadGlyph(shape, font, glyphIndex, &glyphAdvance))
        return true;

    pushError(LOAD_GLYPH_FAILED);
    return false;
}
bool MSDF_Shape_LoadFromSVGFile(msdfgen::Shape& shape, const char* svgFileName, int svgPathIndex) {
    if (!svgFileName) return false;
    msdfgen::Vector2 svgDims;
    if (loadSvgShape(shape, svgFileName, svgPathIndex, &svgDims))
        return true;
    
    pushError(LOAD_SVG_FAILED);
    return false;
}
bool MSDF_Shape_LoadFromDescriptionFile(msdfgen::Shape& shape, const char* descriptionFile, bool* skipColoring) {
    FILE *file = fopen(descriptionFile, "r");
    if (!file) {
        pushError(LOAD_SHAPE_DESCRIPTION_FAILED);
        return false;
    }
    if (!readShapeDescription(file, shape, skipColoring))
        pushError(SHAPE_DESCRIPTION_PARSE_ERROR);
    fclose(file);
    
    return true;
}
bool MSDF_Shape_PreprocessGeometry(msdfgen::Shape& shape) {
    #ifdef MSDFGEN_USE_SKIA
        if (resolveShapeGeometry(shape))
            return true;
        pushError(SHAPE_PROCESSING_FAILED);
        return false;
    #else
        pushError(SHAPE_PROCESSING_NOT_AVAILABLE);
        return false;
    #endif
}

bool MSDF_Shape_Validate(msdfgen::Shape& shape) {
    // Validate and normalize shape
    if (shape.validate())
        return true;
    
    pushError(INVALID_GEOMETRY);
    return false;
}
void MSDF_Shape_Normalize(msdfgen::Shape& shape) {
    shape.normalize();
}
void MSDF_Shape_FlipY(msdfgen::Shape& shape, bool flipped) {
    shape.inverseYAxis = flipped;
}


void MSDF_AutoFrame(MSDF& msdf) {
    int width = msdf.state.width;
    int height = msdf.state.height;
    auto bounds = msdf.shape.getBounds();
    double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
    msdfgen::Vector2 frame(width, height);
    double m = .5+(double) msdf.settings.outputDistanceShift;
    if (!msdf.settings.scaleSpecified) {
        if (msdf.settings.rangeMode == MSDF_RANGE_MODE::RANGE_UNIT)
            l -= m*msdf.settings.range, b -= m*msdf.settings.range, r += m*msdf.settings.range, t += m*msdf.settings.range;
        else
            frame -= 2*m*msdf.settings.range;
    }
    if (l >= r || b >= t)
        l = 0, b = 0, r = 1, t = 1;
    if (frame.x <= 0 || frame.y <= 0)
        pushError(PIXEL_RANGE_FIT_FAILED);
    msdfgen::Vector2 dims(r-l, t-b);
    if (msdf.settings.scaleSpecified)
        msdf.settings.translate = .5*(frame/msdf.settings.scale-dims)-msdfgen::Vector2(l, b);
    else {
        if (dims.x*frame.y < dims.y*frame.x) {
            msdf.settings.translate.set(.5*(frame.x/frame.y*dims.y-dims.x)-l, -b);
            msdf.settings.scale = frame.y/dims.y;
        } else {
            msdf.settings.translate.set(-l, .5*(frame.y/frame.x*dims.x-dims.y)-b);
            msdf.settings.scale = frame.x/dims.x;
        }
    }
    if (msdf.settings.rangeMode == MSDF_RANGE_MODE::RANGE_PX && !msdf.settings.scaleSpecified)
        msdf.settings.translate += m*msdf.settings.range/msdf.settings.scale;
}
bool MSDF_Shape_Export(msdfgen::Shape& shape, const char* fileName) {
    FILE *file = fopen(fileName, "w");
    if (!file)
        return false;
        
    writeShapeDescription(file, shape);
    fclose(file);
    return true;
}
void MSDF_Initialize(MSDF& msdf, MSDF_MODE mode, int width, int height) {
    msdf.mode = mode;
    
    if (msdf.data != nullptr)
        delete msdf.data;
    
    switch (mode) {
        case SINGLE:
        case PSEUDO:
            msdf.data = new msdfgen::Bitmap<float, 1>(width, height);
            break;
        case MULTI:
            msdf.data = new msdfgen::Bitmap<float, 3>(width, height);
            break;
        case MULTI_AND_TRUE:
            msdf.data = new msdfgen::Bitmap<float, 4>(width, height);
            break;
        default:;
    }
    msdf.state.width = width;
    msdf.state.height = height;
}
void MSDF_ColorEdges(MSDF& msdf, edgeColoringFunc_t edgeColoring, unsigned long long coloringSeed, bool skipColoring, const char* edgeAssignment, double angleThreshold) {
    switch (msdf.mode) {
        case MULTI:
        case MULTI_AND_TRUE:
            if (!skipColoring)
                edgeColoring(msdf.shape, angleThreshold, coloringSeed);
            if (edgeAssignment)
                parseColoring(msdf.shape, edgeAssignment);
        default:;
    }
}
bool MSDF_Generate(MSDF& msdf) {
    double range = msdf.settings.normalizedRange();
    
    switch (msdf.mode) {
        case SINGLE: {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            if (msdf.settings.legacyMode)
                generateSDF_legacy(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate);
            else
                generateSDF(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.settings.overlapSupport);
            break;
        }
        case PSEUDO: {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            if (msdf.settings.legacyMode)
                generatePseudoSDF_legacy(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate);
            else
                generatePseudoSDF(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.settings.overlapSupport);
            break;
        }
        case MULTI: {
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            if (msdf.settings.legacyMode)
                generateMSDF_legacy(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.state.scanlinePass ? 0 : msdf.settings.errorCorrectionThreshold);
            else
                generateMSDF(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.settings.errorCorrectionThreshold, msdf.settings.overlapSupport);
            break;
        }
        case MULTI_AND_TRUE: {
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            if (msdf.settings.legacyMode)
                generateMTSDF_legacy(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.state.scanlinePass ? 0 : msdf.settings.errorCorrectionThreshold);
            else
                generateMTSDF(*data, msdf.shape, range, msdf.settings.scale, msdf.settings.translate, msdf.settings.errorCorrectionThreshold, msdf.settings.overlapSupport);
            break;
        }
        default:;
    }
    return true;
}


bool MSDF_TestRender(MSDF& msdf, msdfgen::Bitmap<float, 1>& output) {
    double range = msdf.settings.normalizedRange();
    double avgScale = msdf.settings.averageScale();
    switch(msdf.mode) {
        case MSDF_MODE::SINGLE:
        case MSDF_MODE::PSEUDO: {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            renderSDF(output, *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        case MSDF_MODE::MULTI: {
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            renderSDF(output, *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        case MSDF_MODE::MULTI_AND_TRUE: {
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            renderSDF(output, *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        default:;
    }
    return true;
}
bool MSDF_TestRenderMulti(MSDF& msdf, msdfgen::BitmapBase& renderDestination) {
    double range = msdf.settings.normalizedRange();
    double avgScale = msdf.settings.averageScale();
    
    if (renderDestination.elementType() != msdfgen::BITMAP_ELEMENT_TYPE::FLOAT)
        return false;
    
    auto destinationElementCount = renderDestination.elementsPerPixel();
    
    switch(msdf.mode) {
        case MSDF_MODE::SINGLE:
        case MSDF_MODE::PSEUDO: {
            if (destinationElementCount != 1)
                return false;
                
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
            
            renderSDF(dynamic_cast<msdfgen::Bitmap<float, 1>&>(renderDestination), *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        case MSDF_MODE::MULTI: {
            if (destinationElementCount != 3)
                return false;
            
            
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
                
            renderSDF(dynamic_cast<msdfgen::Bitmap<float, 3>&>(renderDestination), *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        case MSDF_MODE::MULTI_AND_TRUE: {
            if (destinationElementCount != 4)
                return false;
            
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;
                
            renderSDF(dynamic_cast<msdfgen::Bitmap<float, 4>&>(renderDestination), *data, avgScale*range, .5f+msdf.settings.outputDistanceShift);
            break;
        }
        default:;
    }
    
    return true;
}
bool MSDF_ApplyOrientation(MSDF& msdf) {
    auto bounds = msdf.shape.getBounds();
    if (msdf.settings.orientation == GUESS) {
        // Get sign of signed distance outside bounds
        msdfgen::Point2 p(bounds.l-(bounds.r-bounds.l)-1, bounds.b-(bounds.t-bounds.b)-1);
        double distance = msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(msdf.shape, p);
        msdf.settings.orientation = distance <= 0 ? KEEP : REVERSE;
    }
    if (msdf.settings.orientation == REVERSE) {
        switch (msdf.mode) {
            case SINGLE:
            case PSEUDO: {
                msdfgen::Bitmap<float, 1>* data;
                if (!TryConvertGenericBitmap(msdf.data, data))
                    return false;
                invertColor<1>(*data);
                break;
            }
            case MULTI: {
                msdfgen::Bitmap<float, 3>* data;
                if (!TryConvertGenericBitmap(msdf.data, data))
                    return false;
                invertColor<3>(*data);
                break;
            }
            case MULTI_AND_TRUE: {
                msdfgen::Bitmap<float, 4>* data;
                if (!TryConvertGenericBitmap(msdf.data, data))
                    return false;
                invertColor<4>(*data);
                break;
            }
            default:;
        }
        //msdf.settings.orientation = KEEP;
    }
    return true;
}

bool MSDF_ApplyOutputDistanceShift(MSDF& msdf) {
    if (msdf.settings.outputDistanceShift == 0)
        return true;
    
    if (msdf.data->elementType() != msdfgen::BITMAP_ELEMENT_TYPE::FLOAT)
        return false;
        
    float *pixel = (float *) msdf.data->pixelData();
    float *pixelsEnd = pixel + msdf.data->elementsPerPixel()*msdf.data->width()*msdf.data->height();

    while (pixel < pixelsEnd)
        *pixel++ += msdf.settings.outputDistanceShift;
    return true;
}
bool MSDF_ApplyScanlinePass(MSDF& msdf) {
    double range = msdf.settings.normalizedRange();
    switch (msdf.mode) {
        case SINGLE:
        case PSEUDO: {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            distanceSignCorrection(*data, msdf.shape, msdf.settings.scale, msdf.settings.translate, msdf.settings.fillRule);
            break;
        }
        case MULTI: {
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            distanceSignCorrection(*data, msdf.shape, msdf.settings.scale, msdf.settings.translate,msdf.settings. fillRule);
            if (msdf.settings.errorCorrectionThreshold > 0)
                msdfErrorCorrection(*data, msdf.settings.errorCorrectionThreshold/(msdf.settings.scale*range));
            break;
        }
        case MULTI_AND_TRUE: {
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            distanceSignCorrection(*data, msdf.shape, msdf.settings.scale, msdf.settings.translate, msdf.settings.fillRule);
            if (msdf.settings.errorCorrectionThreshold > 0)
                msdfErrorCorrection(*data, msdf.settings.errorCorrectionThreshold/(msdf.settings.scale*range));
            break;
        }
        default:;
    }
    msdf.state.scanlinePass = true;
    
    return true;
}

bool MSDF_Save(MSDF& msdf, const char *filename, Format format) {
    const char* error = "";
    switch (msdf.mode) {
        case SINGLE:
        case PSEUDO: {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            error = writeOutput<1>(*data, filename, format);
            break;
        }
        case MULTI: {
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            error = writeOutput<3>(*data, filename, format);
            break;
        }
        case MULTI_AND_TRUE: {
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            error = writeOutput<4>(*data, filename, format);
            break;
        }
        default:;
    }
    printf(error);
    
    return true;
}

bool MSDF_ConvertTo8Bit(MSDF& msdf) {
    switch (msdf.mode) {
        case SINGLE:
        case PSEUDO:  {
            msdfgen::Bitmap<float, 1>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            convert8bit(*data);
            break;
        }
        case MULTI:  {
            msdfgen::Bitmap<float, 3>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            simulate8bit(*data);
            break;
        }
        case MULTI_AND_TRUE:  {
            msdfgen::Bitmap<float, 4>* data;
            if (!TryConvertGenericBitmap(msdf.data, data))
                return false;

            simulate8bit(*data);
            break;
        }
        default:;
    }
    
    return true;
}

void MSDF_PrintMetrics(MSDF& msdf, bool autoFrame) {
    FILE *out = stdout;
    if (!out)
        pushError(OUTPUT_FILE_WRITE_FAILED);
        
    auto bounds = msdf.shape.getBounds();
    if (msdf.shape.inverseYAxis)
        fprintf(out, "inverseY = true\n");
    if (bounds.r >= bounds.l && bounds.t >= bounds.b)
        fprintf(out, "bounds = %.12g, %.12g, %.12g, %.12g\n", bounds.l, bounds.b, bounds.r, bounds.t);
    //if (svgDims.x != 0 && svgDims.y != 0)
    //    fprintf(out, "dimensions = %.12g, %.12g\n", svgDims.x, svgDims.y);
    //if (glyphAdvance != 0)
    //    fprintf(out, "advance = %.12g\n", glyphAdvance);
    if (autoFrame) {
        if (!msdf.settings.scaleSpecified)
            fprintf(out, "scale = %.12g\n", msdf.settings.averageScale());
        fprintf(out, "translate = %.12g, %.12g\n", msdf.settings.translate.x, msdf.settings.translate.y);
    }
    fprintf(out, "range = %.12g\n", msdf.settings.normalizedRange());
}

void* MSDF_Bitmap_GetPixelData(msdfgen::BitmapBase& bitmap, int& width, int& height, int& channelCount) {
    width = bitmap.width();
    height = bitmap.height();
    channelCount = bitmap.elementsPerPixel();
    return bitmap.pixelData();
}


MSDF* MSDF_Create(MSDF_MODE mode, int width, int height) {
    MSDF* msdf = new MSDF();
    MSDF_Initialize(*msdf, mode, width, height);
    return msdf;
}
void MSDF_Delete(MSDF* msdf) {
    delete msdf;
}
msdfgen::Shape* MSDF_GetShape(MSDF& msdf) {
    return &msdf.shape;
}
msdfgen::BitmapBase* MSDF_GetData(MSDF& msdf) {
    return msdf.data;
}

edgeColoringFunc_t MSDF_GetColoringStrategy(MSDF_COLORING_STRATEGY coloringStrategy) {
    switch(coloringStrategy) {
        case MSDF_COLORING_STRATEGY::SIMPLE:
            return msdfgen::edgeColoringSimple;
        case MSDF_COLORING_STRATEGY::INK_TRAP:
            return msdfgen::edgeColoringInkTrap;
    }
    pushError(ARGUMENT_OUT_OF_RANGE);
    return nullptr;
}

void pushError(int errorNumber) {
    auto index = currentErrorIndex;
    
    printf("push errorIndex bef: %d\n", index);
    if (index < MAX_ERROR_MESSAGES - 1) {
        errors[index++] = errorNumber;
    }
    else {
        errors[index] = ERROR_MESSAGE_QUEUE_FULL;
    }
    
    printf("new push errorIndex: %d\n", index);
    currentErrorIndex = index;
}

const char* MSDF_GetErrorMessage() {
    auto index = currentErrorIndex;
    if (index == 0)
        return nullptr;
    --index;
    
    printf("get errorIndex bef: %d\n", index);
    auto errorNumber = errors[index];
    errors[index] = 0;
    
    auto errorMessage = errorMessages[errorNumber];
    
    printf("new get errorIndex: %d\n", index);
    
    currentErrorIndex = index;
    
    return errorMessage;
}

#endif