// Pentago python module

#include <other/core/python/module.h>

OTHER_PYTHON_MODULE(pentago) {
  OTHER_WRAP(board)
  OTHER_WRAP(score)
  OTHER_WRAP(moves)
  OTHER_WRAP(stat)
  OTHER_WRAP(table)
  OTHER_WRAP(engine)
  OTHER_WRAP(superengine)
  OTHER_WRAP(superscore)
  OTHER_WRAP(supertable)
  OTHER_WRAP(symmetry)
  OTHER_WRAP(all_boards)
  OTHER_WRAP(count)
}
