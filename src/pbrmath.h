/*******************************************************************************************
*
*   pbraylib [math] - Physically Based Rendering math functions for raylib
*
*   Copyright (c) 2017 Victor Fisac
*
********************************************************************************************/

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include <math.h>

//----------------------------------------------------------------------------------
// Functions Declaration
//----------------------------------------------------------------------------------
Matrix MatrixIdentity(void);                                // Returns identity matrix
void MatrixTranspose(Matrix *mat);                          // Transposes provided matrix
void MatrixInvert(Matrix *mat);                             // Invert provided matrix
Matrix MatrixTranslate(float x, float y, float z);          // Returns translation matrix
Matrix MatrixRotate(Vector3 axis, float angle);             // Returns rotation matrix for an angle around an specified axis (angle in radians)
Matrix MatrixScale(float x, float y, float z);              // Returns scaling matrix
Matrix MatrixMultiply(Matrix left, Matrix right);           // Returns two matrix multiplication

//----------------------------------------------------------------------------------
// Functions Definition
//----------------------------------------------------------------------------------
// Returns identity matrix
Matrix MatrixIdentity(void)
{
    Matrix result = { 1.0f, 0.0f, 0.0f, 0.0f, 
                      0.0f, 1.0f, 0.0f, 0.0f, 
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };

    return result;
}

// Returns translation matrix
Matrix MatrixTranslate(float x, float y, float z)
{
    Matrix result = { 1.0f, 0.0f, 0.0f, 0.0f, 
                      0.0f, 1.0f, 0.0f, 0.0f, 
                      0.0f, 0.0f, 1.0f, 0.0f, 
                      x, y, z, 1.0f };

    return result;
}

// Create rotation matrix from axis and angle
// NOTE: Angle should be provided in radians
Matrix MatrixRotate(Vector3 axis, float angle)
{
    Matrix result;

    Matrix mat = MatrixIdentity();

    float x = axis.x, y = axis.y, z = axis.z;

    float length = sqrtf(x*x + y*y + z*z);

    if ((length != 1.0f) && (length != 0.0f))
    {
        length = 1.0f/length;
        x *= length;
        y *= length;
        z *= length;
    }

    float sinres = sinf(angle);
    float cosres = cosf(angle);
    float t = 1.0f - cosres;

    // Cache some matrix values (speed optimization)
    float a00 = mat.m0, a01 = mat.m1, a02 = mat.m2, a03 = mat.m3;
    float a10 = mat.m4, a11 = mat.m5, a12 = mat.m6, a13 = mat.m7;
    float a20 = mat.m8, a21 = mat.m9, a22 = mat.m10, a23 = mat.m11;

    // Construct the elements of the rotation matrix
    float b00 = x*x*t + cosres, b01 = y*x*t + z*sinres, b02 = z*x*t - y*sinres;
    float b10 = x*y*t - z*sinres, b11 = y*y*t + cosres, b12 = z*y*t + x*sinres;
    float b20 = x*z*t + y*sinres, b21 = y*z*t - x*sinres, b22 = z*z*t + cosres;

    // Perform rotation-specific matrix multiplication
    result.m0 = a00*b00 + a10*b01 + a20*b02;
    result.m1 = a01*b00 + a11*b01 + a21*b02;
    result.m2 = a02*b00 + a12*b01 + a22*b02;
    result.m3 = a03*b00 + a13*b01 + a23*b02;
    result.m4 = a00*b10 + a10*b11 + a20*b12;
    result.m5 = a01*b10 + a11*b11 + a21*b12;
    result.m6 = a02*b10 + a12*b11 + a22*b12;
    result.m7 = a03*b10 + a13*b11 + a23*b12;
    result.m8 = a00*b20 + a10*b21 + a20*b22;
    result.m9 = a01*b20 + a11*b21 + a21*b22;
    result.m10 = a02*b20 + a12*b21 + a22*b22;
    result.m11 = a03*b20 + a13*b21 + a23*b22;
    result.m12 = mat.m12;
    result.m13 = mat.m13;
    result.m14 = mat.m14;
    result.m15 = mat.m15;

    return result;
}

// Transposes provided matrix
void MatrixTranspose(Matrix *mat)
{
    Matrix temp;

    temp.m0 = mat->m0;
    temp.m1 = mat->m4;
    temp.m2 = mat->m8;
    temp.m3 = mat->m12;
    temp.m4 = mat->m1;
    temp.m5 = mat->m5;
    temp.m6 = mat->m9;
    temp.m7 = mat->m13;
    temp.m8 = mat->m2;
    temp.m9 = mat->m6;
    temp.m10 = mat->m10;
    temp.m11 = mat->m14;
    temp.m12 = mat->m3;
    temp.m13 = mat->m7;
    temp.m14 = mat->m11;
    temp.m15 = mat->m15;

    *mat = temp;
}

// Invert provided matrix
void MatrixInvert(Matrix *mat)
{
    Matrix temp;

    // Cache the matrix values (speed optimization)
    float a00 = mat->m0, a01 = mat->m1, a02 = mat->m2, a03 = mat->m3;
    float a10 = mat->m4, a11 = mat->m5, a12 = mat->m6, a13 = mat->m7;
    float a20 = mat->m8, a21 = mat->m9, a22 = mat->m10, a23 = mat->m11;
    float a30 = mat->m12, a31 = mat->m13, a32 = mat->m14, a33 = mat->m15;

    float b00 = a00*a11 - a01*a10;
    float b01 = a00*a12 - a02*a10;
    float b02 = a00*a13 - a03*a10;
    float b03 = a01*a12 - a02*a11;
    float b04 = a01*a13 - a03*a11;
    float b05 = a02*a13 - a03*a12;
    float b06 = a20*a31 - a21*a30;
    float b07 = a20*a32 - a22*a30;
    float b08 = a20*a33 - a23*a30;
    float b09 = a21*a32 - a22*a31;
    float b10 = a21*a33 - a23*a31;
    float b11 = a22*a33 - a23*a32;

    // Calculate the invert determinant (inlined to avoid double-caching)
    float invDet = 1.0f/(b00*b11 - b01*b10 + b02*b09 + b03*b08 - b04*b07 + b05*b06);

    temp.m0 = (a11*b11 - a12*b10 + a13*b09)*invDet;
    temp.m1 = (-a01*b11 + a02*b10 - a03*b09)*invDet;
    temp.m2 = (a31*b05 - a32*b04 + a33*b03)*invDet;
    temp.m3 = (-a21*b05 + a22*b04 - a23*b03)*invDet;
    temp.m4 = (-a10*b11 + a12*b08 - a13*b07)*invDet;
    temp.m5 = (a00*b11 - a02*b08 + a03*b07)*invDet;
    temp.m6 = (-a30*b05 + a32*b02 - a33*b01)*invDet;
    temp.m7 = (a20*b05 - a22*b02 + a23*b01)*invDet;
    temp.m8 = (a10*b10 - a11*b08 + a13*b06)*invDet;
    temp.m9 = (-a00*b10 + a01*b08 - a03*b06)*invDet;
    temp.m10 = (a30*b04 - a31*b02 + a33*b00)*invDet;
    temp.m11 = (-a20*b04 + a21*b02 - a23*b00)*invDet;
    temp.m12 = (-a10*b09 + a11*b07 - a12*b06)*invDet;
    temp.m13 = (a00*b09 - a01*b07 + a02*b06)*invDet;
    temp.m14 = (-a30*b03 + a31*b01 - a32*b00)*invDet;
    temp.m15 = (a20*b03 - a21*b01 + a22*b00)*invDet;

    *mat = temp;
}

// Returns scaling matrix
Matrix MatrixScale(float x, float y, float z)
{
    Matrix result = { x, 0.0f, 0.0f, 0.0f, 
                      0.0f, y, 0.0f, 0.0f, 
                      0.0f, 0.0f, z, 0.0f, 
                      0.0f, 0.0f, 0.0f, 1.0f };

    return result;
}

// Returns two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
Matrix MatrixMultiply(Matrix left, Matrix right)
{
    Matrix result;

    result.m0 = right.m0*left.m0 + right.m1*left.m4 + right.m2*left.m8 + right.m3*left.m12;
    result.m1 = right.m0*left.m1 + right.m1*left.m5 + right.m2*left.m9 + right.m3*left.m13;
    result.m2 = right.m0*left.m2 + right.m1*left.m6 + right.m2*left.m10 + right.m3*left.m14;
    result.m3 = right.m0*left.m3 + right.m1*left.m7 + right.m2*left.m11 + right.m3*left.m15;
    result.m4 = right.m4*left.m0 + right.m5*left.m4 + right.m6*left.m8 + right.m7*left.m12;
    result.m5 = right.m4*left.m1 + right.m5*left.m5 + right.m6*left.m9 + right.m7*left.m13;
    result.m6 = right.m4*left.m2 + right.m5*left.m6 + right.m6*left.m10 + right.m7*left.m14;
    result.m7 = right.m4*left.m3 + right.m5*left.m7 + right.m6*left.m11 + right.m7*left.m15;
    result.m8 = right.m8*left.m0 + right.m9*left.m4 + right.m10*left.m8 + right.m11*left.m12;
    result.m9 = right.m8*left.m1 + right.m9*left.m5 + right.m10*left.m9 + right.m11*left.m13;
    result.m10 = right.m8*left.m2 + right.m9*left.m6 + right.m10*left.m10 + right.m11*left.m14;
    result.m11 = right.m8*left.m3 + right.m9*left.m7 + right.m10*left.m11 + right.m11*left.m15;
    result.m12 = right.m12*left.m0 + right.m13*left.m4 + right.m14*left.m8 + right.m15*left.m12;
    result.m13 = right.m12*left.m1 + right.m13*left.m5 + right.m14*left.m9 + right.m15*left.m13;
    result.m14 = right.m12*left.m2 + right.m13*left.m6 + right.m14*left.m10 + right.m15*left.m14;
    result.m15 = right.m12*left.m3 + right.m13*left.m7 + right.m14*left.m11 + right.m15*left.m15;

    return result;
}