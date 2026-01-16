#pragma once
#include <FLHook.h>

TransformMatrix MultiplyMatrix(const TransformMatrix& mat1, const TransformMatrix& mat2);
TransformMatrix SetupTransform(const Vector& p, const Vector& r);