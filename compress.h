// Unified interface to compression libraries

#include <other/core/array/Array.h>
#include <boost/function.hpp>
namespace pentago {

using namespace other;
using boost::function;

Array<uint8_t> compress(RawArray<const uint8_t> data, int level);
Array<uint8_t> decompress(Array<const uint8_t> compressed, size_t uncompressed_size);
void decompress(Array<const uint8_t> compressed, size_t uncompressed_size, const function<void(Array<uint8_t>)>& cont);

size_t compress_memusage(int level);

}