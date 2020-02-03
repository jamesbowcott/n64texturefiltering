
#ifndef vector_h
#define vector_h

#include "types.h"

double vec2dot(vec2 a, vec2 b);
vec2 vec2sub(vec2 a, vec2 b);
vec2 vec2add(vec2 a, vec2 b);
vec4 vec4muls(vec4 v, double s);
vec2 vec2muls(vec2 v, double s);
vec4 vec4add(vec4 a, vec4 b);
vec2 vec2make(double x, double y);
vec4 vec4make(double w, double x, double y, double z);

#endif /* vector_h */
