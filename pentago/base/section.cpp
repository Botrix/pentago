// Board enumeration

#include <pentago/base/section.h>
#include <pentago/utility/convert.h>
#include <pentago/base/symmetry.h>
#include <pentago/utility/debug.h>
#include <geode/array/NdArray.h>
#include <geode/python/wrap.h>
#include <geode/random/Random.h>
#include <geode/utility/str.h>
namespace pentago {

using namespace geode;
using std::cout;
using std::endl;
using std::vector;

section_t count(board_t board) {
  return section_t(vec(count(quadrant(board,0)),count(quadrant(board,1)),count(quadrant(board,2)),count(quadrant(board,3))));
}

Tuple<RawArray<const quadrant_t>,int> rotation_minimal_quadrants(int black, int white) {
  GEODE_ASSERT(0<=black && 0<=white && black+white<=9);
  const int i = ((black*(21-black))>>1)+white;
  const uint16_t lo = rotation_minimal_quadrants_offsets[i],
                 hi = rotation_minimal_quadrants_offsets[i+1];
  const int moved = rotation_minimal_quadrants_reflect_moved[i];
  return tuple(RawArray<const quadrant_t>(hi-lo,rotation_minimal_quadrants_flat+lo),moved);
}

Tuple<RawArray<const quadrant_t>,int> rotation_minimal_quadrants(Vector<uint8_t,2> counts) {
  return rotation_minimal_quadrants(counts.x,counts.y);
}

RawArray<const quadrant_t> safe_rmin_slice(Vector<uint8_t,2> counts, int lo, int hi) {
  RawArray<const quadrant_t> rmin = rotation_minimal_quadrants(counts).x;
  GEODE_ASSERT(0<=lo && lo<=hi && (unsigned)hi<=(unsigned)rmin.size());
  return rmin.slice(lo,hi);
}

RawArray<const quadrant_t> safe_rmin_slice(Vector<uint8_t,2> counts, Range<int> range) {
  return safe_rmin_slice(counts,range.lo,range.hi);
}

Vector<int,4> section_t::shape() const {
  return vec(rotation_minimal_quadrants(counts[0]).x.size(),
             rotation_minimal_quadrants(counts[1]).x.size(),
             rotation_minimal_quadrants(counts[2]).x.size(),
             rotation_minimal_quadrants(counts[3]).x.size());
}

uint64_t section_t::size() const {
  const auto s = shape();
  return (uint64_t)s[0]*s[1]*s[2]*s[3];
}

section_t section_t::child(int quadrant) const {
  GEODE_ASSERT((unsigned)quadrant<4 && counts[quadrant].sum()<9);
  section_t child = *this;
  child.counts[quadrant][sum()&1]++;
  return child;
}

section_t section_t::parent(int quadrant) const {
  GEODE_ASSERT((unsigned)quadrant<4);
  section_t parent = *this;
  auto& count = parent.counts[quadrant][!(sum()&1)];
  GEODE_ASSERT(count);
  count--;
  GEODE_ASSERT(parent.child(quadrant)==*this);
  return parent;
}

section_t section_t::transform(uint8_t global) const {
  const int r = global&3;
  static uint8_t source[4][4] = {{0,1,2,3},{1,3,0,2},{3,2,1,0},{2,0,3,1}};
  section_t t(vec(counts[source[r][0]],counts[source[r][1]],counts[source[r][2]],counts[source[r][3]]));
  if (global&4)
    swap(t.counts[0],t.counts[3]);
  return t;
}

template<int symmetries> Tuple<section_t,uint8_t> section_t::standardize() const {
  static_assert(symmetries==1 || symmetries==4 || symmetries==8,"");
  section_t best = *this;
  uint8_t best_g = 0;
  for (int g=1;g<symmetries;g++) {
    section_t t = transform(g);
    if (best > t) {
      best = t;
      best_g = g;
    }
  }
  return tuple(best,best_g);
}

template Tuple<section_t,uint8_t> section_t::standardize<4>() const;
template Tuple<section_t,uint8_t> section_t::standardize<8>() const;

Vector<int,4> section_t::quadrant_permutation(uint8_t symmetry) {
  GEODE_ASSERT(symmetry<8);
  typedef Vector<uint8_t,2> CV;
  section_t s = section_t(vec(CV(0,0),CV(1,0),CV(2,0),CV(3,0))).transform(symmetry);
  Vector<int,4> p;
  for (int i=0;i<4;i++)
    p[i] = s.counts.find(CV(i,0));
  return p;
}

// For python exposure
static Vector<int,4> section_shape(section_t s) {
  return s.shape();
}

// For python exposure
static section_t standardize_section(section_t s, int symmetries) {
  GEODE_ASSERT(symmetries==1 || symmetries==4 || symmetries==8);
  return symmetries==1?s:symmetries==4?s.standardize<4>().x:s.standardize<8>().x;
}

bool section_t::valid() const {
  for (int i=0;i<4;i++)
    if ((int)counts[i].x+counts[i].y > 9)
      return false;
  const int black = counts[0].x+counts[1].x+counts[2].x+counts[3].x,
            white = counts[0].y+counts[1].y+counts[2].y+counts[3].y;
  if (black<white || black>white+1)
    return false;
  return true;
}

bool section_valid(Vector<Vector<uint8_t,2>,4> s) {
  return section_t(s).valid();
}

ostream& operator<<(ostream& output, section_t section) {
  output << section.sum() << '-';
  for (int i=0;i<4;i++)
    for (int j=0;j<2;j++)
      output << (int)section.counts[i][j];
  return output;
}

PyObject* to_python(const section_t& section) {
  return to_python(section.counts);
}

#ifdef GEODE_PYTHON
} namespace geode {
pentago::section_t FromPython<pentago::section_t>::convert(PyObject* object) {
  pentago::section_t s(from_python<Vector<Vector<uint8_t,2>,4>>(object));
  if (!s.valid())
    THROW(ValueError,"invalid section %s",str(s));
  return s;
}
} namespace pentago {
#endif

board_t random_board(Random& random, const section_t& section) {
  board_t board = 0;
  int permutation[9] = {0,1,2,3,4,5,6,7,8};
  for (int q=0;q<4;q++) {
    for (int i=0;i<8;i++)
      swap(permutation[i],permutation[random.uniform<int>(i,9)]);
    quadrant_t side0 = 0, side1 = 0;
    int b = section.counts[q][0],
        w = section.counts[q][1];
    for (int i=0;i<b;i++)
      side0 |= 1<<permutation[i];
    for (int i=b;i<b+w;i++)
      side1 |= 1<<permutation[i];
    board |= (uint64_t)pack(side0,side1)<<16*q;
  }
  return board;
}

Tuple<quadrant_t,uint8_t> rotation_standardize_quadrant(quadrant_t q) {
  quadrant_t minq = q;
  uint8_t minr = 0;
  side_t s[4][2];
  s[0][0] = unpack(q,0);
  s[0][1] = unpack(q,1);
  for (int r=0;r<3;r++) {
    for (int i=0;i<2;i++)
      s[r+1][i] = rotations[s[r][i]][0];
    quadrant_t rq = pack(s[r+1][0],s[r+1][1]);
    if (minq > rq) {
      minq = rq;
      minr = r+1;
    }
  }
  return tuple(minq,minr);
}

static void rmin_test() {
  // Check inverse
  for (quadrant_t q=0;q<quadrant_count;q++) {
    const auto standard = rotation_standardize_quadrant(q);
    const int ir = rotation_minimal_quadrants_inverse[q];
    const auto rmin = rotation_minimal_quadrants(count(q)).x;
    GEODE_ASSERT(rmin.valid(ir/4) && rmin[ir/4]==standard.x);
    GEODE_ASSERT(transform_board(symmetry_t(0,ir&3),standard.x)==q);
  }

  // Check that all quadrants changed by reflection occur in pairs in the first part of the array
  for (uint8_t b=0;b<=9;b++)
    for (uint8_t w=0;w<=9-b;w++) {
      const auto rmin_moved = rotation_minimal_quadrants(vec(b,w));
      const auto rmin = rmin_moved.x;
      const int moved = rmin_moved.y;
      GEODE_ASSERT((moved&1)==0 && (!moved || moved<rmin.size()));
      for (int i=0;i<rmin.size();i++) {
        const quadrant_t q = rmin[i];
        const quadrant_t qr = pack(reflections[unpack(q,0)],reflections[unpack(q,1)]);
        const int ir = rotation_minimal_quadrants_inverse[qr]/4;
        GEODE_ASSERT((i^(i<moved))==ir);
      }
    }
}

static NdArray<section_t> board_section(NdArray<const board_t> boards) {
  for (const auto board : boards.flat)
    check_board(board);
  NdArray<section_t> sections(boards.shape,uninit);
  for (int i=0;i<boards.flat.size();i++)
    sections.flat[i] = count(boards.flat[i]);
  return sections;
}

static string show_quadrant_rmins(const quadrant_t quadrant) {
  const auto c = count(quadrant);
  const auto rmin = rotation_minimal_quadrants(c);
  const int ir = rotation_minimal_quadrants_inverse[quadrant];
  return format("%d%d-%03d-%d%c",c.x,c.y,ir/4,ir&3,ir/4<rmin.y?"ur"[(ir/4)&1]:'i');
}

string show_board_rmins(const board_t board) {
  return format("%s %s\n%s %s",show_quadrant_rmins(quadrant(board,1)),
                               show_quadrant_rmins(quadrant(board,3)),
                               show_quadrant_rmins(quadrant(board,0)),
                               show_quadrant_rmins(quadrant(board,2)));
}

static Array<const quadrant_t> rmin_slice_py(const Vector<uint8_t,2> counts, const int lo, const int hi) {
  return safe_rmin_slice(counts,lo,hi).copy();
}

static Tuple<Array<const quadrant_t>,int> rotation_minimal_quadrants_py(const Vector<uint8_t,2> counts) {
  const auto p = rotation_minimal_quadrants(counts);
  return tuple(p.x.copy().const_(),p.y);
}

}
using namespace pentago;
using namespace geode::python;

void wrap_section() {
  GEODE_FUNCTION(section_shape)
  GEODE_FUNCTION(section_valid)
  GEODE_FUNCTION_2(show_section,static_cast<string(*)(const section_t&)>(str))
  GEODE_FUNCTION(standardize_section)
  GEODE_FUNCTION(rmin_test)
  GEODE_FUNCTION(board_section)
  GEODE_FUNCTION_2(rmin_slice,rmin_slice_py)
  GEODE_FUNCTION_2(rotation_minimal_quadrants,rotation_minimal_quadrants_py)
}
