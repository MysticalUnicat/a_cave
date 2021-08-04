#pragma once

#include <alias/math.h>

#if 0

#include <stdbool.h>

#define _GNU_SOURCE
#include <math.h>

typedef struct point2 {
  union {
    struct {
      float x;
      float y;
    };
    struct {
      float _1;
      float _2;
    };
  };
} point2;

typedef struct vector2 {
  union {
    struct {
      float x;
      float y;
    };
    struct {
      float _1;
      float _2;
    };
  };
} vector2;

typedef struct normal2 {
  union {
    struct {
      float x;
      float y;
    };
    struct {
      float _1;
      float _2;
    };
  };
} normal2;

typedef struct vector3 {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    struct {
      float _1;
      float _2;
      float _3;
    };
  };
} vector3;

static inline float vector3_dot_product(vector3 a, vector3 b) {
  return (a.x*b.x + a.y*b.y + a.z*b.z);
}

typedef struct matrix2 {
  union {
    float _[4];
    struct {
      float _11, _12;
      float _21, _22;
    };
  };
} matrix2;

static inline float matrix2_determinant(matrix2 m) {
  return m._11*m._22 - m._21*m._12;
}

typedef struct matrix3 {
  union {
    float _[9];
    vector3 row[3];
    struct {
      float _11, _12, _13;
      float _21, _22, _23;
      float _31, _32, _33;
    };
  };
} matrix3;

static inline matrix3 matrix3_minors(matrix3 m) {
  return (matrix3) {
      matrix2_determinant((matrix2){ m._22, m._23, m._32, m._33 })
    , matrix2_determinant((matrix2){ m._21, m._23, m._31, m._33 })
    , matrix2_determinant((matrix2){ m._21, m._22, m._31, m._32 })
    , matrix2_determinant((matrix2){ m._12, m._13, m._32, m._33 })
    , matrix2_determinant((matrix2){ m._11, m._13, m._31, m._33 })
    , matrix2_determinant((matrix2){ m._11, m._32, m._31, m._12 })
    , matrix2_determinant((matrix2){ m._12, m._13, m._22, m._23 })
    , matrix2_determinant((matrix2){ m._11, m._13, m._21, m._23 })
    , matrix2_determinant((matrix2){ m._11, m._13, m._21, m._22 })
  };
}

static inline float matrix3_determinant(matrix3 m) {
  return m._11*matrix2_determinant((matrix2){ m._22, m._23, m._32, m._33 })
       - m._12*matrix2_determinant((matrix2){ m._21, m._23, m._31, m._33 })
       + m._13*matrix2_determinant((matrix2){ m._21, m._22, m._31, m._32 })
       ;
}

static inline matrix3 matrix3_transpose(matrix3 m) {
  return (matrix3) {
      m._11, m._12, m._13
    , m._12, m._22, m._32
    , m._13, m._23, m._33
  };
}

static inline matrix3 matrix3_scale_elements(matrix3 m, float s) {
  return (matrix3) {
      m._11*s, m._12*s, m._13*s
    , m._21*s, m._22*s, m._23*s
    , m._31*s, m._32*s, m._33*s
  };
}

static inline bool matrix3_inverse(matrix3 m, matrix3 * out) {
  matrix3 matrix_of_minors = matrix3_minors(m);
  matrix3 matrix_of_cofactors = (matrix3) {
       matrix_of_minors._11, -matrix_of_minors._12,  matrix_of_minors._13
    , -matrix_of_minors._21,  matrix_of_minors._22, -matrix_of_minors._23
    ,  matrix_of_minors._31, -matrix_of_minors._32,  matrix_of_minors._33
  };
  matrix3 adjugate = matrix3_transpose(matrix_of_cofactors);
  float determinate =
      m._11*matrix_of_minors._11
    - m._12*matrix_of_minors._12
    + m._13*matrix_of_minors._13
    ;
  if(determinate != 0.0f) {
    *out = matrix3_scale_elements(m, 1.0f / determinate);
    return true;
  }
  return false;
}

static inline matrix3 create_translation_matrix3(vector2 v) {
  return (matrix3) {
      1.0f, 0.0f, v.x
    , 0.0f, 1.0f, v.y
    , 0.0f, 0.0f, 1.0f
  };
}

static inline matrix3 create_rotation_matrix3(float a) {
  float s = sin(a);
  float c = cos(a);
  return (matrix3) {
         c,   -s, 0.0f
    ,    s,    c, 0.0f
    , 0.0f, 0.0f, 1.0f
  };
}

static inline matrix3 create_translation_rotation_matrix3(vector2 v, float a) {
  float s = sin(a);
  float c = cos(a);
  return (matrix3) {
         c,   -s, v.x
    ,    s,    c, v.y
    , 0.0f, 0.0f, 1.0f
  };
}

static inline matrix3 multiply_matrix3_matrix3(matrix3 m1, matrix3 m2) {
  return (matrix3) {
      m1._11*m2._11 + m1._12*m2._21 + m1._13*m2._31
    , m1._11*m2._12 + m1._12*m2._22 + m1._13*m2._32
    , m1._11*m2._13 + m1._12*m2._23 + m1._13*m2._33

    , m1._21*m2._11 + m1._22*m2._21 + m1._23*m2._31
    , m1._21*m2._12 + m1._22*m2._22 + m1._23*m2._32
    , m1._21*m2._13 + m1._22*m2._23 + m1._23*m2._33

    , m1._31*m2._11 + m1._32*m2._21 + m1._33*m2._31
    , m1._31*m2._12 + m1._32*m2._22 + m1._33*m2._32
    , m1._31*m2._13 + m1._32*m2._23 + m1._33*m2._33
  };
}

static inline point2 multiply_matrix3_point2(matrix3 m, point2 p) {
  return (point2) {
      m._11*p._1 + m._12*p._1 + m._13*p._1
    , m._21*p._2 + m._22*p._2 + m._13*p._2
  //, m._31*p._3 + m._32*p._3 + m._13*p._3
  };
}

#define multiply_matrix3(A, B) \
  _Generic( \
      (B) \
    , matrix3: multiply_matrix3_matrix3(A, B) \
    , point2: multiply_matrix3_point2(A, B) \
    )

typedef struct matrix23 {
  float _11, _12, _13;
  float _21, _22, _23;
} matrix23;

static inline matrix23 create_matrix23(float x, float y, float a) {
  float s = sin(a);
  float c = cos(a);
  return (matrix23) {
     c, s, x,
    -s, c, y
  };
}

static inline matrix23 create_matrix23_inverse(float x, float y, float a) {
  float s = sin(a);
  float c = cos(a);
  return (matrix23) {
      c, -s, s*y - c*x
    , s,  c, s*x - c*y
  };
}

static inline matrix23 multiply_matrix23_matrix23(matrix23 m1, matrix23 m2) {
  return (matrix23) {
      m1._11*m2._11 + m1._12*m2._21
    , m1._11*m2._12 + m1._12*m2._22
    , m1._11*m2._13 + m1._12*m2._23 + m1._13
    , m1._21*m2._11 + m1._22*m2._21
    , m1._21*m2._12 + m1._22*m2._22
    , m1._21*m2._13 + m1._22*m2._23 + m1._23
  };
}

static inline point2 multiply_matrix23_point2(matrix23 m, point2 p) {
  return (point2) {
      m._11*p.x + m._12*p.y + m._13
    , m._21*p.x + m._22*p.y + m._23
  };
}

#define multiply_matrix23(A, B) \
  _Generic( \
      (B) \
    , matrix3: multiply_matrix23_matrix23(A, B) \
    , point2: multiply_matrix23_point2(A, B) \
    )

#define multiply(A, B) \
  _Generic(\
      (A) \
    , matrix3: multiply_matrix3(A, B) \
    , matrix23: multiply_matrix23(A, B) \
    )

#endif

