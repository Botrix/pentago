// Aligned array allocation

#include <pentago/utility/aligned.h>
#include <pentago/utility/char_view.h>
#include <pentago/utility/debug.h>
#include <other/core/array/Array2d.h>
#include <other/core/python/module.h>
#include <vector>
namespace pentago {

using std::vector;

namespace {

struct aligned_buffer_t : public boost::noncopyable
{
  OTHER_DECLARE_TYPE // Declare pytype
  PyObject_HEAD // Reference counter and pointer to type object
  void* start; // Owning pointer to start of block
};

static void free_buffer(PyObject* object) {
  aligned_buffer_t* buffer = (aligned_buffer_t*)object;
  free(buffer->start);
  delete buffer;
}

PyTypeObject aligned_buffer_t::pytype = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                          // ob_size
  "pentago.aligned_buffer_t", // tp_name
  sizeof(aligned_buffer_t),   // tp_basicsize
  0,                          // tp_itemsize
  free_buffer,                // tp_dealloc
  0,                          // tp_print
  0,                          // tp_getattr
  0,                          // tp_setattr
  0,                          // tp_compare
  0,                          // tp_repr
  0,                          // tp_as_number
  0,                          // tp_as_sequence
  0,                          // tp_as_mapping
  0,                          // tp_hash 
  0,                          // tp_call
  0,                          // tp_str
  0,                          // tp_getattro
  0,                          // tp_setattro
  0,                          // tp_as_buffer
  Py_TPFLAGS_DEFAULT,         // tp_flags
  "Aligned memory buffer",    // tp_doc
  0,                          // tp_traverse
  0,                          // tp_clear
  0,                          // tp_richcompare
  0,                          // tp_weaklistoffset
  0,                          // tp_iter
  0,                          // tp_iternext
  0,                          // tp_methods
  0,                          // tp_members
  0,                          // tp_getset
  0,                          // tp_base
  0,                          // tp_dict
  0,                          // tp_descr_get
  0,                          // tp_descr_set
  0,                          // tp_dictoffset
  0,                          // tp_init
  0,                          // tp_alloc
  0,                          // tp_new
  0,                          // tp_free
};

// All necessary aligned_buffer_t::pytpe fields are filled in, so no PyType_Ready is needed

}

Tuple<void*,PyObject*> aligned_buffer_helper(size_t alignment, size_t size) {
  if (!size)
    return tuple((void*)0,(PyObject*)0);
  auto* buffer = (aligned_buffer_t*)malloc(sizeof(aligned_buffer_t));
  (void)PyObject_INIT(buffer,&buffer->pytype);
#ifndef __APPLE__
  if (!posix_memalign(&buffer->start,alignment,size))
    THROW(bad_alloc);
  return tuple(buffer->start,(PyObject*)buffer);
#else
  // Mac OS 10.7.4 has a buggy version of posix_memalign, so do our own alignment at the cost of one extra element
  buffer->start = malloc(size+alignment-1);
  if (!buffer->start)
    THROW(bad_alloc);
  size_t p = (size_t)buffer->start;
  p = (p+alignment-1)&~(alignment-1);
  return tuple((void*)p,(PyObject*)buffer);
#endif
}

// Most useful when run under valgrind to test for leaks
static void aligned_test() {
  // Check empty buffer
  struct large_t { char data[64]; };
  aligned_buffer<large_t>(0);

  vector<Array<uint8_t>> buffers;
  for (int i=0;i<100;i++) {
    // Test 1D
    auto x = aligned_buffer<int>(10);
    x.zero();
    x[0] = 1;
    x.last() = 2;
    OTHER_ASSERT(x.sum()==3);
    buffers.push_back(char_view_own(x));

    // Test 2D
    auto y = aligned_buffer<float>(vec(4,5));
    y.flat.zero();
    y(0,0) = 1;
    y(0,4) = 2;
    y(3,0) = 3;
    y(3,4) = 4;
    OTHER_ASSERT(y.sum()==10);
    buffers.push_back(char_view_own(y.flat));
  }

  for (auto& x : buffers)
    OTHER_ASSERT((((long)x.data())&15)==0);
}

}
using namespace pentago;

void wrap_aligned() {
  OTHER_FUNCTION(aligned_test)
}