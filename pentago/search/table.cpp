// Hashed transposition table

#include <pentago/search/table.h>
#include <pentago/search/stat.h>
#include <pentago/base/hash.h>
#include <pentago/utility/debug.h>
#include <geode/array/Array.h>
#include <geode/math/popcount.h>
#include <geode/python/wrap.h>
#include <geode/structure/Tuple.h>
#include <geode/utility/format.h>
#include <geode/utility/str.h>
namespace pentago {

using namespace geode;
using std::ostream;
using std::cout;
using std::endl;

// The current transposition table
static int table_bits = 0;
static Array<uint64_t> table;
static table_type_t table_type;

void init_table(int bits) {
  if (bits<1 || bits>30)
    THROW(ValueError,"expected 1<=bits<=30, got bits = %d",bits);
  if (64-bits+score_bits>64)
    THROW(ValueError,"bits = %d is too small, the high order hash bits won't fit",bits);
  table_bits = bits;
  cout << "initializing table: bits = "<<bits<<", size = "<<pow(2.,double(bits-20+3))<<"MB"<<endl;
  table = Array<uint64_t>(); // Allocate memory lazily in set_table_type
  table_type = blank_table;
}

static ostream& operator<<(ostream& output, table_type_t type) {
  return output<<(type==blank_table?"blank"
                 :type==normal_table?"normal"
                 :type==simple_table?"simple"
                 :"unknown");
}

void set_table_type(table_type_t type) {
  if (table_bits<10)
    THROW(RuntimeError,"transposition table not initialized: table_bits = %d",table_bits);
  if (table_type==blank_table)
    table_type = type;
  if (table_type!=type)
    THROW(RuntimeError,"transposition table already set to type %s, must reinitialize before changing to type %s",str(table_type),str(type));

  // Allocate table if we haven't already
  GEODE_ASSERT(!table.size() || table.size()==(1<<table_bits));
  if (!table.size())
    table = Array<uint64_t>(1<<table_bits);
}

score_t lookup(board_t board) {
  assert(table_type!=blank_table);
  STAT(total_lookups++);
  uint64_t h = hash_board(board);
  uint64_t entry = table[h&((1<<table_bits)-1)];
  if (entry>>score_bits==h>>table_bits) {
    STAT(successful_lookups++);
    return entry&score_mask;
  }
  return score(0,1);
}

void store(board_t board, score_t score) {
  assert(table_type!=blank_table);
  uint64_t h = hash_board(board);
  uint64_t& entry = table[h&((1<<table_bits)-1)];
  if (entry>>score_bits==h>>table_bits || uint16_t(entry&score_mask)>>2 <= score>>2)
    entry = h>>table_bits<<score_bits|score;
}

Tuple<Array<board_t>,Array<score_t>> read_table(int max_count, int min_depth) {
  GEODE_ASSERT(table_bits>=10 && table_type!=blank_table);
  const int size = (1<<table_bits)-1;
  GEODE_ASSERT(table.size()==size);
  Array<board_t> boards;
  Array<score_t> scores;
  for (int h=0;h<size;h++) {
    uint64_t entry = table[h];
    if (!entry)
      continue;
    score_t score = entry&score_mask;
    if (score>>2 < min_depth)
      continue;
    board_t board = inverse_hash_board(entry>>score_bits<<table_bits|h);
    if (count_stones(board) > max_count)
      continue;
    boards.append(board);
    scores.append(score);
  }
  return tuple(boards,scores);
}

}
using namespace pentago;
using namespace geode::python;

void wrap_table() {
  GEODE_FUNCTION(hash_board)
  GEODE_FUNCTION(inverse_hash_board)
  GEODE_FUNCTION(init_table)
  GEODE_FUNCTION(read_table)
}
