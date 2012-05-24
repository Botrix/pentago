// Instantiation necessary python conversions

#include "superscore.h"
#include "all_boards.h"
#include <other/core/array/convert.h>
#include <other/core/vector/convert.h>
namespace other {

using namespace pentago;

// Since there's no 256 bit dtype, map super_t to 4 uint64_t's
template<> struct NumpyDescr<super_t> : public NumpyDescr<uint64_t> {};
template<> struct NumpyIsStatic<super_t> : public mpl::true_ {};
template<> struct NumpyRank<super_t> : public mpl::int_<1> {};
template<> struct NumpyInfo<super_t> { static void dimensions(npy_intp* dimensions) {
  dimensions[0] = 4;
}};

// section_t is just a thin wrapper around a Vector
typedef Vector<Vector<uint8_t,2>,4> CV;
template<> struct NumpyDescr<section_t> : public NumpyDescr<CV> {};
template<> struct NumpyIsStatic<section_t> : public NumpyIsStatic<CV> {};
template<> struct NumpyRank<section_t> : public NumpyRank<CV> {};
template<> struct NumpyInfo<section_t> : public NumpyInfo<CV> {};

VECTOR_CONVERSIONS(2,uint8_t)
VECTOR_CONVERSIONS(4,uint16_t)
VECTOR_CONVERSIONS(4,Vector<uint8_t,2>)
ARRAY_CONVERSIONS(1,super_t)
ARRAY_CONVERSIONS(1,section_t)

}