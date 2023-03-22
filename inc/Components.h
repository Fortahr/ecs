#pragma once

#include <intrin.h>
#include <glm/mat4x4.hpp>

namespace Esteem
{
	struct Zero /* */ { glm::mat4 data = glm::mat4(1.f); };
	struct One /*  */ { __m128 data = _mm_set_ps1(1); };
	struct Two /*  */ { __m128 data = _mm_set_ps1(2); };
	struct Three /**/ { __m128 data = _mm_set_ps1(3); };
	struct Four /* */ { __m128 data = _mm_set_ps1(4); };
	struct Five /* */ { __m128 data = _mm_set_ps1(5); };

	struct Six /*  */ { __m128 data = _mm_set_ps1(5); };
	struct Seven /**/ { __m128 data = _mm_set_ps1(5); };
	struct Eight /**/ { __m128 data = _mm_set_ps1(5); };
	struct Nine /* */ { __m128 data = _mm_set_ps1(5); };
}