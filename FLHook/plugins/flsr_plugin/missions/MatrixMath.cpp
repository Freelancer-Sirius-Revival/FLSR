#include "MatrixMath.h"

TransformMatrix MultiplyMatrix(const TransformMatrix& mat1, const TransformMatrix& mat2) {
    TransformMatrix result = { 0 };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                result.d[i][j] += mat1.d[i][k] * mat2.d[k][j];
    return result;
}

TransformMatrix SetupTransform(const Vector& p, const Vector& r)
{
    // Convert degrees into radians
#define PI (3.14159265358f)
    float ax = r.x * (PI / 180);
    float ay = r.y * (PI / 180);
    float az = r.z * (PI / 180);

    // Initial matrix
    TransformMatrix smat = { 0 };
    smat.d[0][0] = smat.d[1][1] = smat.d[2][2] = smat.d[3][3] = 1;

    // Translation matrix
    TransformMatrix tmat;
    tmat.d[0][0] = 1;
    tmat.d[0][1] = 0;
    tmat.d[0][2] = 0;
    tmat.d[0][3] = 0;
    tmat.d[1][0] = 0;
    tmat.d[1][1] = 1;
    tmat.d[1][2] = 0;
    tmat.d[1][3] = 0;
    tmat.d[2][0] = 0;
    tmat.d[2][1] = 0;
    tmat.d[2][2] = 1;
    tmat.d[2][3] = 0;
    tmat.d[3][0] = p.x;
    tmat.d[3][1] = p.y;
    tmat.d[3][2] = p.z;
    tmat.d[3][3] = 1;

    // X-axis rotation matrix
    TransformMatrix xmat;
    xmat.d[0][0] = 1;
    xmat.d[0][1] = 0;
    xmat.d[0][2] = 0;
    xmat.d[0][3] = 0;
    xmat.d[1][0] = 0;
    xmat.d[1][1] = std::cos(ax);
    xmat.d[1][2] = std::sin(ax);
    xmat.d[1][3] = 0;
    xmat.d[2][0] = 0;
    xmat.d[2][1] = -std::sin(ax);
    xmat.d[2][2] = std::cos(ax);
    xmat.d[2][3] = 0;
    xmat.d[3][0] = 0;
    xmat.d[3][1] = 0;
    xmat.d[3][2] = 0;
    xmat.d[3][3] = 1;

    // Y-axis rotation matrix
    TransformMatrix ymat;
    ymat.d[0][0] = std::cos(ay);
    ymat.d[0][1] = 0;
    ymat.d[0][2] = -std::sin(ay);
    ymat.d[0][3] = 0;
    ymat.d[1][0] = 0;
    ymat.d[1][1] = 1;
    ymat.d[1][2] = 0;
    ymat.d[1][3] = 0;
    ymat.d[2][0] = std::sin(ay);
    ymat.d[2][1] = 0;
    ymat.d[2][2] = std::cos(ay);
    ymat.d[2][3] = 0;
    ymat.d[3][0] = 0;
    ymat.d[3][1] = 0;
    ymat.d[3][2] = 0;
    ymat.d[3][3] = 1;

    // Z-axis rotation matrix
    TransformMatrix zmat;
    zmat.d[0][0] = std::cos(az);
    zmat.d[0][1] = std::sin(az);
    zmat.d[0][2] = 0;
    zmat.d[0][3] = 0;
    zmat.d[1][0] = -std::sin(az);
    zmat.d[1][1] = std::cos(az);
    zmat.d[1][2] = 0;
    zmat.d[1][3] = 0;
    zmat.d[2][0] = 0;
    zmat.d[2][1] = 0;
    zmat.d[2][2] = 1;
    zmat.d[2][3] = 0;
    zmat.d[3][0] = 0;
    zmat.d[3][1] = 0;
    zmat.d[3][2] = 0;
    zmat.d[3][3] = 1;

    TransformMatrix tm;
    tm = MultiplyMatrix(smat, tmat);
    tm = MultiplyMatrix(tm, xmat);
    tm = MultiplyMatrix(tm, ymat);
    tm = MultiplyMatrix(tm, zmat);

    return tm;
}