
#ifndef types_h
#define types_h

#include <stdint.h>

typedef struct
{
//    union { double x, u; };
//    union { double y, v; };
    double x, y;
} vec2;

typedef struct
{
    double x,y,z;
} vec3;

typedef struct
{
    union { double w, a; };
    union { double x, r; };
    union { double y, g; };
    union { double z, b; };
} vec4;

typedef struct
{
    vec2 tex_coords;
    vec2 uv;
    vec4 colour;
} Texel;

typedef struct
{
    Texel tl, tr, bl, br;
} TexelSelection;

typedef enum {
    TextureFormat_RGB,
    TextureFormat_RGBA,
    TextureFormat_BGRA,
} TextureFormat;

typedef struct
{
    void *pixels;
    unsigned w;
    unsigned h;
    TextureFormat format;
} Texture;

#endif /* types_h */
