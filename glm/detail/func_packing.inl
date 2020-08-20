/// @ref core
/// @file glm/detail/func_packing.inl

#include "../common.hpp"
#include "type_half.hpp"

namespace glm
{
	GLM_FUNC_QUALIFIER vec2 unpackUnorm2x16(uint p)
	{
		union
		{
			uint in;
			unsigned short out[2];
		} u;

		u.in = p;

		return vec2(u.out[0], u.out[1]) * 1.5259021896696421759365224689097e-5f;
	}

	GLM_FUNC_QUALIFIER vec4 unpackUnorm4x8(uint p)
	{
		union
		{
			uint in;
			unsigned char out[4];
		} u;

		u.in = p;

		return vec4(u.out[0], u.out[1], u.out[2], u.out[3]) * 0.0039215686274509803921568627451f;
	}
	GLM_FUNC_QUALIFIER double packDouble2x32(uvec2 const& v)
	{
		union
		{
			uint   in[2];
			double out;
		} u;

		u.in[0] = v[0];
		u.in[1] = v[1];

		return u.out;
	}

	GLM_FUNC_QUALIFIER uvec2 unpackDouble2x32(double v)
	{
		union
		{
			double in;
			uint   out[2];
		} u;

		u.in = v;

		return uvec2(u.out[0], u.out[1]);
	}

	GLM_FUNC_QUALIFIER uint packHalf2x16(vec2 const& v)
	{
		union
		{
			signed short in[2];
			uint out;
		} u;

		u.in[0] = detail::toFloat16(v.x);
		u.in[1] = detail::toFloat16(v.y);

		return u.out;
	}

	GLM_FUNC_QUALIFIER vec2 unpackHalf2x16(uint v)
	{
		union
		{
			uint in;
			signed short out[2];
		} u;

		u.in = v;

		return vec2(
			detail::toFloat32(u.out[0]),
			detail::toFloat32(u.out[1]));
	}
}//namespace glm

#if GLM_CONFIG_SIMD == GLM_ENABLE
#	include "func_packing_simd.inl"
#endif

