
#pragma once

#include <cstdlib>
#include <cstdint>
#include "../core/Shape.h"

#ifdef _MSC_VER
#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
#define PACKED( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

namespace msdfgen {

typedef uint32_t unicode_t;

class FreetypeHandle;
class FontHandle;

PACKED(class GlyphIndex {

public:
    explicit GlyphIndex(unsigned index = 0);
    unsigned getIndex() const;
    bool operator!() const;

private:
    uint32_t index;

});

/// Global metrics of a typeface (in font units).
struct FontMetrics {
    /// The size of one EM.
    double emSize;
    /// The vertical position of the ascender and descender relative to the baseline.
    double ascenderY, descenderY;
    /// The vertical difference between consecutive baselines.
    double lineHeight;
    /// The vertical position and thickness of the underline.
    double underlineY, underlineThickness;
};

/// Initializes the FreeType library.
FreetypeHandle * initializeFreetype();
/// Deinitializes the FreeType library.
void deinitializeFreetype(FreetypeHandle *library);

#ifdef FT_FREETYPE_H
/// Creates a FontHandle from FT_Face that was loaded by the user. destroyFont must still be called but will not affect the FT_Face.
FontHandle * adoptFreetypeFont(FT_Face ftFace);
#endif
/// Loads a font file and returns its handle.
FontHandle * loadFont(FreetypeHandle *library, const char *filename);
/// Unloads a font file.
void destroyFont(FontHandle *font);
/// Outputs the metrics of a font file.
bool getFontMetrics(FontMetrics &metrics, FontHandle *font);
/// Outputs the width of the space and tab characters.
bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FontHandle *font);
/// Outputs the glyph index corresponding to the specified Unicode character.
bool getGlyphIndex(GlyphIndex &glyphIndex, FontHandle *font, unicode_t unicode);
/// Loads the geometry of a glyph from a font file.
bool loadGlyph(Shape &output, FontHandle *font, GlyphIndex glyphIndex, double *advance = NULL);
bool loadGlyph(Shape &output, FontHandle *font, unicode_t unicode, double *advance = NULL);
/// Outputs the kerning distance adjustment between two specific glyphs.
bool getKerning(double &output, FontHandle *font, GlyphIndex glyphIndex1, GlyphIndex glyphIndex2);
bool getKerning(double &output, FontHandle *font, unicode_t unicode1, unicode_t unicode2);

}
