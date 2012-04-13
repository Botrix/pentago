// Symmetry-aware board counting via Polya's enumeration theorem

#include "symmetry.h"
#include <other/core/array/sort.h>
#include <other/core/math/uint128.h>
#include <other/core/python/module.h>
#include <other/core/utility/Hasher.h>
#include <other/core/utility/remove_const_reference.h>
#include <tr1/unordered_map>
#include <vector>
namespace pentago {

using std::cout;
using std::endl;
using std::vector;
using std::tr1::unordered_map;

namespace {

// We want to count all board positions with the given number of white and black stones,
// ignoring all supersymmetries.  To do this, we apply Polya's theorem with |G| = 2048
// and color weight generating function f(b,w) = 1+b+w.  For details and notation, see
//     http://en.wikipedia.org/wiki/Polya_enumeration_theorem

template<int d_> struct Polynomial {
  static const int d = d_;
  unordered_map<Vector<int,d>,uint128_t,Hasher> x;

  Polynomial() {}
  
  Polynomial(uint128_t a) {
    x[Vector<int,d>()] = a;
  }
};

template<int d> Polynomial<d> monomial(const Vector<int,d>& m) {
  Polynomial<d> p;
  p.x[m]++;
  return p;
}

template<int d> Polynomial<d> operator*(const Polynomial<d>& f, const Polynomial<d>& g) {
  Polynomial<d> fg;
  for (auto& fm : f.x)
    for (auto& gm : g.x)
      fg.x[fm.first+gm.first] += fm.second*gm.second;
  return fg;
}

template<int d> Polynomial<d>& operator*=(Polynomial<d>& f, const Polynomial<d>& g) {
  f = f*g;
  return f;
}

template<int d> Polynomial<d>& operator+=(Polynomial<d>& f, const Polynomial<d>& g) {
  for (auto& m : g.x)
    f.x[m.first] += m.second; 
  return f;
}

template<int fd,class Gs> auto compose(const Polynomial<fd>& f, const Gs& g)
  -> Polynomial<remove_const_reference<decltype(g[0])>::type::d> {
  const int gd = remove_const_reference<decltype(g[0])>::type::d;
  OTHER_ASSERT(fd==(int)g.size());
  vector<vector<Polynomial<gd>>> gp(fd); // indexed by i, power of g[i]
  Polynomial<gd> fg;
  for (auto& fm : f.x) {
    Polynomial<gd> m = fm.second;
    for (int i=0;i<fd;i++)
      if (fm.first[i]) {
        while (gp[i].size()<fm.first[i])
          gp[i].push_back(gp[i].size()?g[i]*gp[i].back():g[i]); 
        m *= gp[i][fm.first[i]-1];
      }
    fg += m; 
  }
  return fg;
}

string OTHER_UNUSED str(uint128_t n) {
  uint64_t lo(n);
  OTHER_ASSERT(lo==n);
  return format("%lld",lo);
}

template<int d> string str(const Polynomial<d>& f, const char* names) {
  OTHER_ASSERT(d==strlen(names));
  Array<Vector<int,d> > ms;
  for (auto& m : f.x)
    ms.append(m.first.reversed());
  sort(ms,LexicographicCompare());
  string s;
  if (!ms.size())
    s = '0';
  for (int i=0;i<ms.size();i++) {
    auto m = ms[i].reversed();
    if (i)
      s += " + ";
    uint128_t c = f.x.find(m)->second;
    if (c!=1 || m==Vector<int,d>())
      s += str(c);
    bool need_space = false;
    for (int j=0;j<d;j++)
      if (m[j]) {
        if (need_space)
          s += ' ';
        need_space = false;
        s += names[j];
        if (m[j]>1) {
          s += '^';
          s += str(m[j]);
          need_space = true;
        }
      }
  }
  return s;
}

Polynomial<2> count_generator() {
  // Make the color generating function
  Polynomial<2> f;
  f.x[vec(0,0)]++;
  f.x[vec(1,0)]++;
  f.x[vec(0,1)]++;

  // Generate the cycle index
  Polynomial<36> Z;
  for (symmetry_t s : symmetries) {
    Vector<int,36> cycles;
    for (int i=0;i<4;i++) for (int j=0;j<9;j++) {
      side_t start = (side_t)1<<(16*i+j);
      side_t side = start;
      for (int o=1;;o++) {
        side = transform_side(s,side);
        if (side==start) {
          cycles[o-1]++;
          break;
        }
      }
    }
    for (int i=0;i<36;i++) {
      OTHER_ASSERT(cycles[i]%(i+1)==0);
      cycles[i] /= i+1;
    }
    Z.x[cycles]++;
  }

  // Compute f(b^k,w^k) for k = 1 to 36
  vector<Polynomial<2>> fk(36);
  for (int k=1;k<=36;k++)
    fk[k-1] = compose(f,vec(monomial(vec(k,0)),monomial(vec(0,k))));

  // Compose Zg and fk to get F
  auto F = compose(Z,fk);
  for (auto& m : F.x) {
    OTHER_ASSERT(!(m.second&2047));
    m.second >>= 11;
  }
  return F;
}

static uint128_t supercount_boards(int n) {
  static Polynomial<2> F;
  if (!F.x.size())
    F = count_generator();
  const int b = (n+1)/2;
  auto it = F.x.find(vec(b,n-b));
  return it!=F.x.end()?it->second:0;
}

}
}
using namespace pentago;

void wrap_count() {
  OTHER_FUNCTION(supercount_boards)  
}
