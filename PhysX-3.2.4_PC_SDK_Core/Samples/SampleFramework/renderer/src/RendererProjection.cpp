/*
 * Copyright 2008-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */
#include <RendererProjection.h>
#include <math.h>

using namespace SampleRenderer;

RendererProjection::RendererProjection(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	RENDERER_ASSERT(farPlane > nearPlane, "Cannot construct a Projection Matrix whose nearPlane is further than the farPlane.");
	memset(m_matrix, 0, sizeof(m_matrix));

	float fd = 1/tanf(fov/2);
	m_matrix[0]  = fd/aspectRatio;
	m_matrix[5]  = fd;
	m_matrix[10] = (farPlane + nearPlane)/(nearPlane - farPlane);
	m_matrix[11] = -1;
	m_matrix[14] = (2 * farPlane * nearPlane)/(nearPlane - farPlane);
}

RendererProjection::RendererProjection(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	memset(m_matrix, 0, sizeof(m_matrix));
	m_matrix[0]  =  2/(right - left);
	m_matrix[5]  =  2/(top - bottom);
	m_matrix[10] = -2/(farPlane - nearPlane);
	m_matrix[12] = - (right + left)   / (right - left);
	m_matrix[13] = - (top + bottom) / (top - bottom);
	m_matrix[14] = - (farPlane + nearPlane)   / (farPlane - nearPlane);
	m_matrix[15] = 1;
}

void RendererProjection::getColumnMajor44(float *f) const
{
	if(f) memcpy(f, m_matrix, sizeof(m_matrix));
}

void RendererProjection::getRowMajor44(float *f) const
{
	getColumnMajor44(f);
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			float save = f[4 * i + j];
			f[4 * i + j] = f[4 * j + i];
			f[4 * j + i] = save;
		}
	}
}

class vec4
{
public:
	vec4(void) {}

	vec4(const PxVec3& v3)
	{
		x=v3.x;
		y=v3.y;
		z=v3.z;
		w=1.0f;
	}

	vec4(PxF32 _x, PxF32 _y, PxF32 _z, PxF32 _w)
	{
		x=_x;
		y=_y;
		z=_z;
		w=_w;
	}

	vec4 operator*=(PxF32 f)
	{
		x*=f;
		y*=f;
		z*=f;
		w*=f;
		return *this;
	}

public:
	PxF32 x, y, z, w;
};

class mat4x4
{
public:
	mat4x4(void){}

	mat4x4(const physx::PxTransform &m)
	{
		PxMat44 mat44(m);
		memcpy(&x.x, mat44.front(), 4 * 4 * sizeof (PxReal));
	}

	mat4x4(const RendererProjection &m)
	{
		m.getColumnMajor44(&x.x);
	}

public:
	vec4 x;
	vec4 y;
	vec4 z;
	vec4 w;
};

mat4x4 invert(const mat4x4 &m)
{
	mat4x4 inv;

#define det3x3(a0, a1, a2, a3, a4, a5, a6, a7, a8) \
(a0 * (a4*a8 - a7*a5) - a1 * (a3*a8 - a6*a5) + a2 * (a3*a7 - a6*a4))

	inv.x.x =  det3x3(m.y.y, m.y.z, m.y.w,   m.z.y, m.z.z, m.z.w,   m.w.y, m.w.z, m.w.w);
	inv.x.y = -det3x3(m.x.y, m.x.z, m.x.w,   m.z.y, m.z.z, m.z.w,   m.w.y, m.w.z, m.w.w);
	inv.x.z =  det3x3(m.x.y, m.x.z, m.x.w,   m.y.y, m.y.z, m.y.w,   m.w.y, m.w.z, m.w.w);
	inv.x.w = -det3x3(m.x.y, m.x.z, m.x.w,   m.y.y, m.y.z, m.y.w,   m.z.y, m.z.z, m.z.w);

	inv.y.x = -det3x3(m.y.x, m.y.z, m.y.w,   m.z.x, m.z.z, m.z.w,   m.w.x, m.w.z, m.w.w);
	inv.y.y =  det3x3(m.x.x, m.x.z, m.x.w,   m.z.x, m.z.z, m.z.w,   m.w.x, m.w.z, m.w.w);
	inv.y.z = -det3x3(m.x.x, m.x.z, m.x.w,   m.y.x, m.y.z, m.y.w,   m.w.x, m.w.z, m.w.w);
	inv.y.w =  det3x3(m.x.x, m.x.z, m.x.w,   m.y.x, m.y.z, m.y.w,   m.z.x, m.z.z, m.z.w);

	inv.z.x =  det3x3(m.y.x, m.y.y, m.y.w,   m.z.x, m.z.y, m.z.w,   m.w.x, m.w.y, m.w.w);
	inv.z.y = -det3x3(m.x.x, m.x.y, m.x.w,   m.z.x, m.z.y, m.z.w,   m.w.x, m.w.y, m.w.w);
	inv.z.z =  det3x3(m.x.x, m.x.y, m.x.w,   m.y.x, m.y.y, m.y.w,   m.w.x, m.w.y, m.w.w);
	inv.z.w = -det3x3(m.x.x, m.x.y, m.x.w,   m.y.x, m.y.y, m.y.w,   m.z.x, m.z.y, m.z.w);

	inv.w.x = -det3x3(m.y.x, m.y.y, m.y.z,   m.z.x, m.z.y, m.z.z,   m.w.x, m.w.y, m.w.z);
	inv.w.y =  det3x3(m.x.x, m.x.y, m.x.z,   m.z.x, m.z.y, m.z.z,   m.w.x, m.w.y, m.w.z);
	inv.w.z = -det3x3(m.x.x, m.x.y, m.x.z,   m.y.x, m.y.y, m.y.z,   m.w.x, m.w.y, m.w.z);
	inv.w.w =  det3x3(m.x.x, m.x.y, m.x.z,   m.y.x, m.y.y, m.y.z,   m.z.x, m.z.y, m.z.z);

#undef det3x3

	PxF32 det = m.x.x*inv.x.x + m.y.x*inv.x.y + m.z.x*inv.x.z + m.w.x*inv.x.w;
	RENDERER_ASSERT(det, "Matrix inversion failed!");
	if(!det) det = 1;
	PxF32 invDet = 1 / det;

	inv.x *= invDet;
	inv.y *= invDet;
	inv.z *= invDet;
	inv.w *= invDet;

	return inv;
}

mat4x4 operator*(const mat4x4 &a, const mat4x4 &b)
{
	mat4x4 t;
#define _MULT(_r, _c)        \
t._c ._r = a._c.x * b.x._r + \
a._c.y * b.y._r + \
a._c.z * b.z._r + \
a._c.w * b.w._r;
	_MULT(x,x); _MULT(x,y); _MULT(x,z); _MULT(x,w);
	_MULT(y,x); _MULT(y,y); _MULT(y,z); _MULT(y,w);
	_MULT(z,x); _MULT(z,y); _MULT(z,z); _MULT(z,w);
	_MULT(w,x); _MULT(w,y); _MULT(w,z); _MULT(w,w);
#undef _MULT
	return t;
}

vec4 operator*(const mat4x4 &a, const vec4 &b)
{
	vec4 v;
	v.x = a.x.x * b.x + a.y.x * b.y + a.z.x * b.z + a.w.x;
	v.y = a.x.y * b.x + a.y.y * b.y + a.z.y * b.z + a.w.y;
	v.z = a.x.z * b.x + a.y.z * b.y + a.z.z * b.z + a.w.z;
	v.w = a.x.w * b.x + a.y.w * b.y + a.z.w * b.z + a.w.w;
	return v;
}

void SampleRenderer::buildProjectMatrix(float *dst, const RendererProjection &proj, const physx::PxTransform &view)
{
	mat4x4 projview = invert(mat4x4(view)) * mat4x4(proj);
	memcpy(dst, &projview.x.x, sizeof(float)*16);
}

void SampleRenderer::buildUnprojectMatrix(float *dst, const RendererProjection &proj, const physx::PxTransform &view)
{
	mat4x4 invprojview = invert(mat4x4(view) * mat4x4(proj));
	memcpy(dst, &invprojview.x.x, sizeof(float)*16);
}

PxVec3 SampleRenderer::unproject(const RendererProjection &proj, const physx::PxTransform &view, PxF32 x, PxF32 y, PxF32 z)
{
	vec4   screenPoint(x, y, z, 1);
	mat4x4 invprojview = invert(mat4x4(view) * mat4x4(proj));
	vec4   nearPoint = invprojview * screenPoint;
	RENDERER_ASSERT(nearPoint.w, "Unproject failed!");
	if(nearPoint.w) nearPoint *= 1.0f / nearPoint.w;
	return PxVec3(nearPoint.x, nearPoint.y, nearPoint.z);
}

PxVec3 SampleRenderer::project( const RendererProjection &proj, const physx::PxTransform &view, const PxVec3& pos)
{
	mat4x4 projView    = mat4x4(view) * mat4x4(proj);
	vec4   screenPoint = projView * vec4(pos);
	float rw = 1.0f / screenPoint.w;
	return PxVec3( screenPoint.x, screenPoint.y, screenPoint.z ) * rw;
}
