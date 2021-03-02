
#pragma once

#include "BitmapRef.hpp"

namespace msdfgen {

#ifdef MSDFGEN_USE_CPP11
    #define CONSTEXPR constexpr
#else
    #define CONSTEXPR const
#endif


enum BITMAP_ELEMENT_TYPE {
    UNKNOWN,
    FLOAT,
    BYTE
};
template<typename T>
struct TypeToElementTypeMapping {
    static CONSTEXPR BITMAP_ELEMENT_TYPE value = BITMAP_ELEMENT_TYPE::UNKNOWN;
};

template<>
struct TypeToElementTypeMapping<float> {
    static CONSTEXPR BITMAP_ELEMENT_TYPE value = BITMAP_ELEMENT_TYPE::FLOAT;
};

template<>
struct TypeToElementTypeMapping<unsigned char> {
    static CONSTEXPR BITMAP_ELEMENT_TYPE value = BITMAP_ELEMENT_TYPE::BYTE;
};

template<>
struct TypeToElementTypeMapping<char> {
    static CONSTEXPR BITMAP_ELEMENT_TYPE value = BITMAP_ELEMENT_TYPE::BYTE;
};
#undef CONSTEXPR
class BitmapBase {
public:
    BitmapBase(int width, int height, BITMAP_ELEMENT_TYPE elementType, int elementsPerPixel) : w(width), h(height), ePP(elementsPerPixel), eT(elementType) {
        switch(elementType) {
            case UNKNOWN:
                break;
            case FLOAT:
                eS = sizeof(float);
                break;
            case BYTE:
                eS = sizeof(char);
                break;
        }
    }
    /// Bitmap width in pixels.
    int width() const { return w; }
    /// Bitmap height in pixels.
    int height() const { return h; }
    
    int bytesPerPixel() const { return eS * ePP; }
    int elementSize() const { return eS; }
    int elementsPerPixel() const { return ePP; }
    
    BITMAP_ELEMENT_TYPE elementType() const { return eT; }
    
    virtual void* pixelData() const = 0;
    
    virtual ~BitmapBase() {}
    
protected:
    int w, h, ePP, eS;
    BITMAP_ELEMENT_TYPE eT;
};

/// A 2D image bitmap with N channels of type T. Pixel memory is managed by the class.
template <typename T, int N = 1>
class Bitmap : public BitmapBase {

public:
    Bitmap();
    Bitmap(int width, int height);
    Bitmap(const BitmapConstRef<T, N> &orig);
    Bitmap(const Bitmap<T, N> &orig);
#ifdef MSDFGEN_USE_CPP11
    Bitmap(Bitmap<T, N> &&orig);
#endif
    virtual ~Bitmap();
    Bitmap<T, N> & operator=(const BitmapConstRef<T, N> &orig);
    Bitmap<T, N> & operator=(const Bitmap<T, N> &orig);
#ifdef MSDFGEN_USE_CPP11
    Bitmap<T, N> & operator=(Bitmap<T, N> &&orig);
#endif

    T * operator()(int x, int y);
    const T * operator()(int x, int y) const;
#ifdef MSDFGEN_USE_CPP11
    explicit operator T *();
    explicit operator const T *() const;
#else
    operator T *();
    operator const T *() const;
#endif
    operator BitmapRef<T, N>();
    operator BitmapConstRef<T, N>() const;

#ifdef MSDFGEN_USE_CPP11
    void* pixelData() const override { return pixels; }
#else
    void* pixelData() const { return pixels; }
#endif
private:
    T *pixels;

};

}

#include "Bitmap.hpp"
