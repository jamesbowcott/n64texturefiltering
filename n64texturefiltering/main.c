#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>

#include "vector.h"
#include "vector_simd.h"

#ifndef DEBUG
//#define assert(a) ((void)0)
inline void assert(int condition)
{
    if (!condition)
    {
        abort();
    }
}

#else
void assert(int condition)
{
    if (!condition)
    {
        return;
    }
}
#endif

vec4 Vector4_FromSDLColor(SDL_Color color)
{
    vec4 v;
    v.r = color.r / 255.0;
    v.g = color.g / 255.0;
    v.b = color.b / 255.0;
    v.a = color.a / 255.0;
    return v;
}

SDL_Color Vector4_ToSDLColor(vec4 v)
{
    SDL_Color color;
    color.r = (int)(v.r * 255);
    color.g = (int)(v.g * 255);
    color.b = (int)(v.b * 255);
    color.a = (int)(v.a * 255);
    return color;
}

SDL_Colour SDL_GetSurfacePixel(SDL_Surface *surface, Uint32 x, Uint32 y)
{
    Uint8 *pixels = surface->pixels + (surface->pitch * y) + (x * 4);
    SDL_Color colour;
    colour.b = pixels[0];
    colour.g = pixels[1];
    colour.r = pixels[2];
    colour.a = pixels[3];
    return colour;
}

void SDL_SetSurfacePixel(SDL_Surface *surface, Uint32 x, Uint32 y, SDL_Color colour)
{
    Uint8 *pixels = surface->pixels + (surface->pitch * y) + (x * 4);
    pixels[0] = colour.b;
    pixels[1] = colour.g;
    pixels[2] = colour.r;
    pixels[3] = colour.a;
}

void assertVec4Norm(vec4 v)
{
    int w = v.w * 255;
    int x = v.x * 255;
    int y = v.y * 255;
    int z = v.z * 255;
    assert(!(w < 0 || w > 255 || x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255));
}

double BilinearInterpolation(double q11, double q12, double q21, double q22, double x1, double x2, double y1, double y2, double x, double y)
{
//    assert(x >= x1);
//    assert(x <= x2);
//    assert(y >= y1);
//    assert(y <= y2);
    double x2x1, y2y1, x2x, y2y, yy1, xx1;
    x2x1 = x2 - x1;
    y2y1 = y2 - y1;
    x2x = x2 - x;
    y2y = y2 - y;
    yy1 = y - y1;
    xx1 = x - x1;
    return 1.0 / (x2x1 * y2y1) * (q11 * x2x * y2y +
                                  q21 * xx1 * y2y +
                                  q12 * x2x * yy1 +
                                  q22 * xx1 * yy1
                                  );
}

vec4 BilinearInterpVector4(vec4 q11, vec4 q12, vec4 q21, vec4 q22, double x1, double x2, double y1, double y2, double x, double y)
{
    vec4 v;
    v.w = BilinearInterpolation(q11.w, q12.w, q21.w, q22.w, x1, x2, y1, y2, x, y);
    v.x = BilinearInterpolation(q11.x, q12.x, q21.x, q22.x, x1, x2, y1, y2, x, y);
    v.y = BilinearInterpolation(q11.y, q12.y, q21.y, q22.y, x1, x2, y1, y2, x, y);
    v.z = BilinearInterpolation(q11.z, q12.z, q21.z, q22.z, x1, x2, y1, y2, x, y);
    assertVec4Norm(v);
    return v;
}


vec3 Barycentric(vec2 p, vec2 a, vec2 b, vec2 c)
{
    vec3 v;
    vec2 v0 = vec2sub(b, a);
    vec2 v1 = vec2sub(c, a);
    vec2 v2 = vec2sub(p, a);
    double d00 = vec2dot(v0, v0);
    double d01 = vec2dot(v0, v1);
    double d11 = vec2dot(v1, v1);
    double d20 = vec2dot(v2, v0);
    double d21 = vec2dot(v2, v1);
    double denom = d00 * d11 - d01 * d01;
    v.y = (d11 * d20 - d01 * d21) / denom;
    v.z = (d00 * d21 - d01 * d20) / denom;
    v.x = 1.0f - v.y - v.z;
    return v;
}

vec4 TriInterp(vec2 v, vec2 va, vec2 vb, vec2 vc, vec4 ca, vec4 cb, vec4 cc)
{
    vec3 w = Barycentric(v, va, vb, vc);
//    if (w.x > 1.0 || w.x < 0.0 ||
//        w.y > 1.0 || w.y < 0.0 ||
//        w.z > 1.0 || w.z < 0.0)
//    {
//        //return vec4make(0.5,0.5,0.5,1.0);
//    }
    
    ca = vec4muls(ca, w.x);
    cb = vec4muls(cb, w.y);
    cc = vec4muls(cc, w.z);
    vec4 c;
    c = vec4add(ca, cb);
    c = vec4add(c, cc);
    return c;
}

vec4 TriInterpTexel(vec2 uv, Texel t0, Texel t1, Texel t2)
{
    return TriInterp(uv, t0.uv, t1.uv, t2.uv, t0.colour, t1.colour, t2.colour);
}

vec4 TriFilter(TexelSelection texsel, vec2 uv)
{
    vec4 c;
    double y1 = texsel.tl.uv.y;
    double y2 = texsel.bl.uv.y;
    double x1 = texsel.tl.uv.x;
    double x2 = texsel.tr.uv.x;
    double y = uv.y - y1;
    double x = uv.x - x1;

    double ydiff = y2 - y1;
    double xdiff = x2 - x1;
    
    double ynorm = y / ydiff;
    double xnorm = x / xdiff;

    assert(ynorm >= 0.0);
    assert(ynorm <= 1.0);
    assert(xnorm >= 0.0);
    assert(xnorm <= 1.0);

#if 0
    if (xnorm - ynorm > 0.0)
    {
        c = TriInterpTexel(uv, texsel.tl, texsel.tr, texsel.br);
    }
    else
    {
        c = TriInterpTexel(uv, texsel.tl, texsel.bl, texsel.br);
    }
#else
    if (ynorm + xnorm < 1.0)
    {
        return TriInterpTexel(uv, texsel.bl, texsel.tl, texsel.tr);
    }
    else
    {
        return TriInterpTexel(uv, texsel.tr, texsel.br, texsel.bl);
    }
#endif
    return c;
}


vec4 NearestFilter(TexelSelection texsel, vec2 uv)
{
    double x_dist = texsel.tr.uv.x - texsel.tl.uv.x;
    double y_dist = texsel.bl.uv.y - texsel.tl.uv.y;
    
    double x_threshold = texsel.tl.uv.x + (x_dist / 2);
    double y_threshold = texsel.tl.uv.y + (y_dist / 2);

    if (uv.y < y_threshold) // top
    {
        if (uv.x < x_threshold) // left
        {
            return texsel.tl.colour;
        }
        else // right
        {
            return texsel.tr.colour;
        }
    }
    else // bottom
    {
        if (uv.x < x_threshold) // left
        {
            return texsel.bl.colour;
        }
        else // right
        {
            return texsel.br.colour;
        }
    }

}

vec4 BiFilter(TexelSelection texsel, vec2 uv)
{
    Texel q11 = texsel.bl;
    Texel q12 = texsel.tl;
    Texel q21 = texsel.br;
    Texel q22 = texsel.tr;
    vec4 c = BilinearInterpVector4(q11.colour, q12.colour, q21.colour, q22.colour, q11.uv.x, q22.uv.x, q11.uv.y, q22.uv.y, uv.x, uv.y);
    
    return c;
}

vec4 ReadTexturePixelRGB(Texture tex, unsigned x, unsigned y)
{
    unsigned offset = ((y * tex.w) + x) * 3;
    vec4 colour;
    colour.r = ((uint8_t*)tex.pixels)[offset++] / 255.0;
    colour.g = ((uint8_t*)tex.pixels)[offset++] / 255.0;
    colour.b = ((uint8_t*)tex.pixels)[offset] / 255.0;
    colour.a = 1.0;

    return colour;
}

vec4 ReadTexturePixelRGBA(Texture tex, unsigned x, unsigned y)
{
    uint32_t pixel = ((uint32_t*)tex.pixels)[(y * tex.w) + x];
    uint8_t *components = (uint8_t*)&pixel;
    vec4 colour;
    colour.r = components[0] / 255.0;
    colour.g = components[1] / 255.0;
    colour.b = components[2] / 255.0;
    colour.a = components[3] / 255.0;
    return colour;
}

vec4 ReadTexturePixelBGRA(Texture tex, unsigned x, unsigned y)
{
    uint32_t pixel = ((uint32_t*)tex.pixels)[(y * tex.w) + x];
    uint8_t *components = (uint8_t*)&pixel;
    vec4 colour;
    colour.b = components[0] / 255.0;
    colour.g = components[1] / 255.0;
    colour.r = components[2] / 255.0;
    colour.a = components[3] / 255.0;
    return colour;
}

vec4 ReadTexturePixel(Texture tex, unsigned tex_x, unsigned tex_y)
{
    assert(tex_x < tex.w);
    assert(tex_y < tex.h);
    
    switch (tex.format) {
        case TextureFormat_RGB:
            return ReadTexturePixelRGB(tex, tex_x, tex_y);
        case TextureFormat_RGBA:
            return ReadTexturePixelRGBA(tex, tex_x, tex_y);
        case TextureFormat_BGRA:
            return ReadTexturePixelBGRA(tex, tex_x, tex_y);
        default:
            assert(0);
    }
}

Texel SelectTexels_MakeTexel(Texture tex, int tex_x, int tex_y)
{
    Texel t;
    t.tex_coords.x = tex_x;
    t.tex_coords.y = tex_y;
    t.uv.x = (double)tex_x / (tex.w - 1);
    t.uv.y = (double)tex_y / (tex.h - 1);
    assert(t.uv.x >= 0);
    assert(t.uv.x <= tex.w - 1);
    assert(t.uv.y >= 0);
    assert(t.uv.y <= tex.h - 1);
    assert(tex_x == 0 ? t.uv.x == 0.0 : 1);
    assert(tex_x == tex.w - 1 ? t.uv.x == 1.0 : 1);
    t.colour = ReadTexturePixel(tex, tex_x, tex_y);
    return t;
}


TexelSelection SelectTexels(Texture tex, vec2 uv)
{
    TexelSelection out;
    
    vec2 tex_uv = vec2make(uv.x * (tex.w-1), uv.y * (tex.h-1));

    assert(tex_uv.x >= 0.0);
    assert(tex_uv.x < (double)tex.w - 1.0);
    assert(tex_uv.y >= 0.0);
    assert(tex_uv.y < (double)tex.h - 1.0);

    int tex_x = floor(tex_uv.x);
    int tex_y = floor(tex_uv.y);
    assert(tex_x >= 0);
    assert(tex_x < tex.w - 1);
    assert(tex_y >= 0);
    assert(tex_y < tex.h - 1);
    
    out.tl = SelectTexels_MakeTexel(tex, tex_x, tex_y);
    out.tr = SelectTexels_MakeTexel(tex, tex_x + 1, tex_y);
    out.br = SelectTexels_MakeTexel(tex, tex_x + 1, tex_y + 1);
    out.bl = SelectTexels_MakeTexel(tex, tex_x, tex_y + 1);

    assert(out.tl.uv.x <= uv.x);
    assert(out.tr.uv.x >= uv.x);
    assert(out.tl.uv.y <= uv.y);
    assert(out.bl.uv.y >= uv.y);

    return out;
}

#define EPSILON (1.0 / 255.0)

SDL_Color Filter(SDL_Surface *sdltex, double u, double v)
{
    Texture tex;
    tex.w = sdltex->w;
    tex.h = sdltex->h;
    tex.pixels = sdltex->pixels;
    switch (sdltex->format->format) {
        case SDL_PIXELFORMAT_RGB24:
            tex.format = TextureFormat_RGB;
            break;
        case SDL_PIXELFORMAT_RGBA32:
            tex.format = TextureFormat_RGBA;
            break;
        case SDL_PIXELFORMAT_BGRA32:
            tex.format = TextureFormat_BGRA;
            break;
        default:
            assert(0);
    }
    
    vec2 uv = vec2make(u, v);
    
    TexelSelection texsel = SelectTexels(tex, uv);

    vec4 c = TriFilter(texsel, uv);
    
    if (c.r < 0.0 || c.r > 1.0 + EPSILON ||
        c.g < 0.0 || c.g > 1.0 + EPSILON ||
        c.b < 0.0 || c.b > 1.0 + EPSILON ||
        c.a < 0.0 || c.a > 1.0 + EPSILON)
    {
        // return (SDL_Color){128,128,128,SDL_ALPHA_OPAQUE};
        //abort();
    }
    
    return Vector4_ToSDLColor(c);
}


struct render_thread_ctx
{
    const SDL_Surface *dst, *tex;
    int target_h, target_w;
    int seg_c, seg_i;
    int status;
};


void *render_thread(void* data)
{
    struct render_thread_ctx *ctx = data;
    
    for (int y = (ctx->target_h / ctx->seg_c) * ctx->seg_i; y < (ctx->target_h / ctx->seg_c) * (ctx->seg_i + 1); y++)
    {
        double v = y / (double)(ctx->target_h);
        for (int x = 0; x < ctx->target_w; x++)
        {
            double u = x / (double)(ctx->target_w);
            SDL_SetSurfacePixel(ctx->dst, x, y, Filter(ctx->tex, u, v));
        }
    }
    ctx->status = 0;
    pthread_exit(NULL);
}

void render_present(SDL_Renderer *renderer, SDL_Surface *dst)
{
    SDL_PumpEvents();
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, dst);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
}

void render(SDL_Window *window, SDL_Renderer* renderer, SDL_Surface *tex)
{
    int target_w, target_h;
    SDL_GetWindowSize(window, &target_w, &target_h);
    SDL_Surface *dst = SDL_CreateRGBSurfaceWithFormat(0, target_w, target_h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (dst == NULL) return;
    
    const int THREAD_COUNT = 2;
    
    pthread_t threads[THREAD_COUNT];
    struct render_thread_ctx thread_ctxs[THREAD_COUNT];
    
    for (int t = 0; t < THREAD_COUNT; t++)
    {
        thread_ctxs[t].dst = dst;
        thread_ctxs[t].tex = tex;
        thread_ctxs[t].target_w = target_w;
        thread_ctxs[t].target_h = target_h;
        thread_ctxs[t].seg_i = t;
        thread_ctxs[t].seg_c = THREAD_COUNT;
        thread_ctxs[t].status = 1;
        
        assert(pthread_create(&threads[t], NULL, render_thread, (void*)&thread_ctxs[t]) == 0);
    }
    
#if 0
    int all_status;
    do
    {
        all_status = 0;
        for (int t = 0; t < THREAD_COUNT; t++) {
            all_status |= thread_ctxs[t].status;
        }
        render_present(renderer, dst);
    } while (all_status);
#else
#endif
    
    for (int t = 0; t < THREAD_COUNT; t++) {
        assert(pthread_join(threads[t], NULL) == 0);
    }
    render_present(renderer, dst);

    SDL_FreeSurface(dst);
}

int main(int argc, const char * argv[]) {
    int exitcode = EXIT_FAILURE;
    if (argc < 2) return EXIT_FAILURE;
    
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
    
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(80, 80, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE, &window, &renderer);
    
    SDL_Surface *tex = IMG_Load(argv[1]);
    if (tex == NULL) goto FAIL;
    //if (tex->w != tex->h) goto FAIL;
    const char * format_name = SDL_GetPixelFormatName(tex->format->format);
    
    render(window, renderer, tex);
    
    SDL_Event event;
    while (SDL_WaitEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            break;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            printf("SDL_WINDOWEVENT %d\n", event.window.event);
            switch (event.window.event) {
                case SDL_WINDOWEVENT_EXPOSED:
                //case SDL_WINDOWEVENT_SIZE_CHANGED:
                //case SDL_WINDOWEVENT_RESIZED:
                    render(window, renderer, tex);
                    break;
            }
        }
        
    }
    
    exitcode = EXIT_SUCCESS;
FAIL:
    IMG_Quit();
    SDL_Quit();
    return exitcode;
}
