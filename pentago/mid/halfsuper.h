// Half of a super_t, by parity
#pragma once

/*
 * A super_t is a map from the abelian group G = Z_4^4 into Z_2, representing all local rotations of
 * a board. However, in the mid solver we only ever use half the bits of any particular super_t,
 * since rotations mix only through rmax which flips parity.
 *
 * Therefore, we introduce halfsuper_t, a map from Z_4^3 x {0,1} to Z_2, storing all bits of a super_t
 * with the same parity (which is either even or odd.  Specifically, if s is a super and p = 0 or 1,
 * we form two halfsuper_t's h(p) by
 *
 *   h(p)(a,b,c,d) = s(2a+(b+c+d+p)%2,b,c,d)
 *
 * Thus, for each value of (b,c,d), h(p) contains two of the four a values of s: {0,2} if p = 0 and
 * {1,3} if p = 1.
 *
 * Let's think about how rmax works.  We order the array in d-c-b-a major order, as with super_t.
 * Assume the parity field is first.  Say we start with x(p=0), and want y(p=1) = rmax(x).  Let
 * y = || y_i where y_i = rmax_i(x) is rmax with only one quadrant moving.  We have
 *
 *   y_0(0,r) = sy_0((r+1)%2,r) = sx(1+(r+1)%2,r) | sx(3+(r+1)%2,r) = x(0,r) | x(1,r)
 *   y_0(1,r) = same
 *
 * So the parity dimension is easy: it's just an allreduce or.  What about the other dimensions?
 *
 *   y_1(0,0,r) = sy_1(  (r+1)%2,0,r) = sx(  (r+1)%2,1,r) | sx(  (r+1)%2,3,r) = x(0,1,r) | x(0,3,r)
 *   y_1(1,0,r) = sy_1(2+(r+1)%2,0,r) = sx(2+(r+1)%2,1,r) | sx(2+(r+1)%2,3,r) = x(1,1,r) | x(1,3,r)
 *   y_1(0,1,r) = sy_1(  (r+2)%2,1,r) = sx(  (r+2)%2,0,r) | sx(  (r+2)%2,2,r) = x(0,0,r) | x(0,2,r)
 *   y_1(1,1,r) = sy_1(2+(r+2)%2,1,r) = sx(2+(r+2)%2,0,r) | sx(2+(r+2)%2,2,r) = x(1,0,r) | x(1,2,r)
 *
 * To summarize, if x has p = 1, we have
 *
 *   y_0(a,r)   =     x(0,r) | x(1,r)
 *   y_1(a,b,r) = x(a,b+1,r) | x(a,b-1,r)
 *
 * Does the situation change if x has p = 0?  In that case,
 *
 *   y_0(a,r) = sy_0(2a+r%2,r) = x(0,r) | x(1,r)
 *   y_1(a,b,r) = sy_1(2a+(b+r)%2,b,r) = sx(2a+(b+r)%2,b+1,r) | sx(2a+(b+r)%2,b-1,r) = x(a,b-1,r) | x(a,b+1,r)
 *
 * I.e., there is no difference.  That's extremely convenient.
 */

#include <pentago/base/superscore.h>
#if PENTAGO_SSE // halfsuper implemented only in SSE mode for now
namespace pentago {

struct halfsuper_t {
  __m128i x;

  halfsuper_t() = default;

  // Zero-only constructor
  halfsuper_t(zero*) {
    x = _mm_set1_epi32(0);
  }

  explicit halfsuper_t(__m128i x)
    : x(x) {}

  explicit operator bool() const {
    return _mm_movemask_epi8(~_mm_cmpeq_epi32(x,_mm_setzero_si128()))!=0;
  }

  bool operator==(halfsuper_t h) const { return    !(*this^h); }
  bool operator!=(halfsuper_t h) const { return bool(*this^h); }

  halfsuper_t operator~() const { return halfsuper_t(~x); }
  halfsuper_t operator|(halfsuper_t h) const { return halfsuper_t(x|h.x); }
  halfsuper_t operator&(halfsuper_t h) const { return halfsuper_t(x&h.x); }
  halfsuper_t operator^(halfsuper_t h) const { return halfsuper_t(x^h.x); }
  halfsuper_t operator|=(halfsuper_t h) { x |= h.x; return *this; }
  halfsuper_t operator&=(halfsuper_t h) { x &= h.x; return *this; }
  halfsuper_t operator^=(halfsuper_t h) { x ^= h.x; return *this; }

  // Do not use the follow functions in performance critical code

  bool operator[](uint8_t r) const {
    // Periodic with period 128
    return _mm_movemask_epi8(_mm_slli_epi16(x,7-(r&7)))>>(r>>3&15)&1;
  }

  bool operator()(int a, int b, int c, int d) const {
    return operator[]((a&1)+2*(b&3)+8*(c&3)+32*(d&3));
  }

  static halfsuper_t singleton(uint8_t r) {
    const bool hi = r>>6&1;
    const auto chunk = uint64_t(1)<<(r&63);
    return halfsuper_t(geode::pack(hi?0:chunk,hi?chunk:0));
  }

  static halfsuper_t singleton(int a, int b, int c, int d) {
    return singleton((a&1)+2*(b&3)+8*(c&3)+32*(d&3));
  }
};

// Split a super_t into two halfsuper_t's
GEODE_EXPORT Vector<halfsuper_t,2> split(const super_t s) GEODE_CONST;

// Merge even and odd bits into a single super_t
GEODE_EXPORT super_t merge(const halfsuper_t even, const halfsuper_t odd) GEODE_CONST;

// Both half of super_wins: (even,odd)
GEODE_EXPORT Vector<halfsuper_t,2> halfsuper_wins(const side_t side) GEODE_CONST;

// Same as rmax for super_t, but twice as fast.  Flips parity.
GEODE_CONST static inline halfsuper_t rmax(const halfsuper_t h) {
  const __m128i x = h.x;
  // First (parity) quadrant
  const __m128i t0 = (x|_mm_srli_epi16(x,1))&_mm_set1_epi8(0x55),
                y0 = t0|_mm_slli_epi16(t0,1);
  // Second and third quadrants
  const __m128i y1 = (_mm_slli_epi16(x,2) & _mm_set1_epi8(0b11111100))
                   | (_mm_slli_epi16(x,6) & _mm_set1_epi8(0b11000000))
                   | (_mm_srli_epi16(x,2) & _mm_set1_epi8(0b00111111))
                   | (_mm_srli_epi16(x,6) & _mm_set1_epi8(0b00000011));
  const __m128i y2 = _mm_slli_epi32(x,8) | _mm_slli_epi32(x,24)
                   | _mm_srli_epi32(x,8) | _mm_srli_epi32(x,24);
  // Fourth quadrant
  const __m128i y3 = _mm_shuffle_epi32(x,LE_MM_SHUFFLE(3,0,1,2))
                   | _mm_shuffle_epi32(x,LE_MM_SHUFFLE(1,2,3,0));
  // Assemble
  return halfsuper_t(y0|y1|y2|y3);
}

GEODE_EXPORT int popcount(halfsuper_t h);

} namespace geode {
template<> struct IsScalar<pentago::halfsuper_t> : public mpl::true_ {};
}
#endif
