#ifndef HDR_dataTypes
#define HDR_dataTypes

#include <stdint.h>
#include <math.h>

#include "geoTypeDef.h"

namespace geo {

/**
 *  @brief Coordinate policy used by geo::point
 *
 *  只包含 point<C> 真正用到的内容（关联类型、舍入、比较、平方长度）。
 *  名字与 geo/core/geoTypes.h 里的 coord_traits / generic_coord_traits 区分开，
 *  两者可以在同一个编译单元中共存而不冲突。
 */
template <class C, class A, class D>
struct basic_point_traits
{
  typedef C coord_type;
  typedef A area_type;
  typedef D distance_type;

  static coord_type rounded (double v) { return coord_type (v > 0 ? v + 0.5 : v - 0.5); }
  static coord_type rounded (float v) { return coord_type (v > 0 ? v + 0.5 : v - 0.5); }

  template <class X>
  static coord_type rounded (X v) { return coord_type (v); }

  static distance_type rounded_distance (double v) { return distance_type (v > 0 ? v + 0.5 : v - 0.5); }

  static bool equal (coord_type c1, coord_type c2)
  {
    return c1 == c2;
  }

  static area_type sq_length (coord_type ax, coord_type ay,
                              coord_type bx, coord_type by)
  {
    return ((area_type) ax - (area_type) bx) * ((area_type) ax - (area_type) bx)
         + ((area_type) ay - (area_type) by) * ((area_type) ay - (area_type) by);
  }
};

/**
 *  @brief point<C> 的坐标策略声明（未提供特化的类型会在编译期报错）
 */
template <class C>
struct point_traits
{
};

/**
 *  @brief 32 bit integer coordinates
 */
template <>
struct point_traits<int32_t>
  : public basic_point_traits<int32_t, int64_t, uint32_t>
{
};

/**
 *  @brief 16 bit integer coordinates
 */
template <>
struct point_traits<int16_t>
  : public basic_point_traits<int16_t, int32_t, uint32_t>
{
};

/**
 *  @brief double coordinates
 */
template <>
struct point_traits<double>
{
  typedef double coord_type;
  typedef double area_type;
  typedef double distance_type;

  template <class X>
  static double rounded (X v) { return double (v); }

  static double rounded_distance (double v) { return v; }

  static bool equal (double c1, double c2)
  {
    return fabs (c1 - c2) < 1e-5;
  }

  static area_type sq_length (coord_type ax, coord_type ay,
                              coord_type bx, coord_type by)
  {
    return (ax - bx) * (ax - bx) + (ay - by) * (ay - by);
  }
};

} // namespace geo

#endif
