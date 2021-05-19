#pragma once

#define _GNU_SOURCE
#include <math.h>

typedef struct v2 {
  float x;
  float y;
  // 1
} v2;

static inline v2 mk_v2(float x, float y) {
  return (v2){ x, y };
}

typedef struct v3 {
  float x;
  float y;
  float z;
  // 1
} v3;

static inline v3 mk_v3(float x, float y, float z) {
  return (v3){ x, y, z };
}

static inline float v3_dot_product(v3 a, v3 b) {
  return (a.x*b.x + a.y*b.y + a.z*b.z);
}

typedef struct n2 {
  float x;
  float y;
  // 0
} n2;

typedef struct m23 {
  union {
    float _[6];
    v3 rows[2];
    struct {
      float _11;
      float _12;
      float _13;
      float _21;
      float _22;
      float _23;
    };
  };
} m23;

static const m23 m23_identity = {
    1.0f, 0.0f, 0.0f
  , 0.0f, 1.0f, 0.0f
//, 0.0f, 0.0f, 1.0f
};

static inline m23 mk_m23_rotation(float angle) {
  float s, c;
  sincosf(angle, &s, &c);
  return (m23) {
    c, -s, 0.0f,
    s,  c, 0.0f
  };
}

static inline m23 mk_m23_translation(v2 position) {
  return (m23) {
    1.0f, 0.0f, position.x,
    0.0f, 1.0f, position.y
  };
}

static inline m23 m23_multiply(m23 a, m23 b) {
  return (m23) {
      a._11*b._11 + a._12*b._21
    , a._11*b._12 + a._12*b._22
    , a._11*b._13 + a._12*b._23 + a._13

    , a._21*b._11 + a._22*b._21
    , a._21*b._12 + a._22*b._22
    , a._21*b._13 + a._22*b._23 + a._23
  };
}


