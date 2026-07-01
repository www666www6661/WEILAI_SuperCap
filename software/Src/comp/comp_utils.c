/*
  自定义的小工具
*/

#include "comp_utils.h"

#include "bsp_def.h"

#ifndef M_2PI
#define M_2PI 6.283185f
#endif

/**
 * @brief 计算平方根倒数
 *
 * @param x 输入
 * @return float 计算结果
 */
float inv_sqrtf(float x) {
#if 0
  /* Fast inverse square-root */
  /* See: http://en.wikipedia.org/wiki/Fast_inverse_square_root */
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	y = y * (1.5f - (halfx * y * y));
	return y;
#else
  return 1.0f / sqrtf(x);
#endif
}

/**
 * @brief 将值限制在-limit和limit之间。
 *
 * @param x 输入
 * @param limit 上下界的绝对值
 * @return float 操作后的值
 */
float abs_clampf(float x, float limit) { return MIN(limit, MAX(x, -limit)); }

/**
 * @brief 将值限制在下限和上限之间。
 *
 * @param origin 被操作的值
 * @param lo 下限
 * @param hi 上限
 */
void clampf(float *origin, float lo, float hi) {
  XB_ASSERT(origin);
  *origin = MIN(hi, MAX(*origin, lo));
}

/**
 * @brief 符号函数
 *
 * @param in 输入
 * @return float 运算结果
 */
float signf(float x) {
  if (x == 0.0f) {
    return x;
  } else {
    return (x > 0) ? 1.0f : 0.0f;
  }
}

int float_to_uint(float x, float x_min, float x_max, int bits) {
  /// Converts a float to an unsigned int, given range and number of bits ///
  float span = x_max - x_min;
  float offset = x_min;
  return (int)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

float uint_to_float(int x_int, float x_min, float x_max, int bits) {
  /// converts unsigned int to float, given range and number of bits ///
  float span = x_max - x_min;
  float offset = x_min;
  return ((float)(x_int)) * span / ((float)((1 << bits) - 1)) + offset;
}
