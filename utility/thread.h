// Thread utilities
#pragma once

#include <pentago/utility/job.h>
#include <pentago/utility/wall_time.h>
#include <other/core/array/Array.h>
#include <other/core/python/Ptr.h>
#include <other/core/python/Object.h>
#include <other/core/python/ExceptionValue.h>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <pthread.h>
#include <vector>
namespace pentago {

using namespace other;
using std::vector;
using boost::function;
struct time_entry_t;

/* Our goals are to make timing (1) as fast as possible and (2) easy
 * to share across different ranks in an MPI run.  Therefore, we make
 * the questionable and perhaps temporary decision to use a fixed
 * enum for the kinds of operations which can be timed.  This lets us
 * store timing information in a simple array.
 */

enum time_kind_t {
  compress_kind,
  decompress_kind,
  snappy_kind,
  unsnappy_kind,
  filter_kind,
  copy_kind,
  schedule_kind,
  wait_kind,
  mpi_kind,
  partition_kind,
  compute_kind,
  accumulate_kind,
  count_kind,
  meaningless_kind,
  read_kind,
  write_kind,
  write_sections_kind,
  write_counts_kind,
  write_sparse_kind,
  allocate_line_kind,
  request_send_kind,
  response_send_kind,
  response_recv_kind,
  wakeup_kind,
  output_send_kind,
  output_recv_kind,
  master_idle_kind,
  cpu_idle_kind,
  io_idle_kind,
  // Internal use only
  master_missing_kind,
  cpu_missing_kind,
  io_missing_kind,
  // Count the number of kinds
  _time_kinds
};

// Convert time_kind_t to string
OTHER_EXPORT vector<const char*> time_kind_names();

// The top 3 bits are for the event kind
typedef uint64_t event_t;
const event_t unevent = 0;
const event_t block_ekind       = event_t(1)<<61;
const event_t line_ekind        = event_t(2)<<61;
const event_t block_line_ekind  = event_t(3)<<61;
const event_t block_lines_ekind = event_t(5)<<61; // Used to be 4, but the format changed
const event_t ekind_mask        = event_t(7)<<61;

class thread_time_t : public boost::noncopyable {
  time_entry_t* entry;
public:
  OTHER_EXPORT thread_time_t(time_kind_t kind, event_t event);
  OTHER_EXPORT ~thread_time_t();
  OTHER_EXPORT void stop(); // Stop early
};

// Extract local times and reset them to zero
OTHER_EXPORT Array<wall_time_t> clear_thread_times();

// Extract total thread times
OTHER_EXPORT Array<wall_time_t> total_thread_times();

// Print a timing report
OTHER_EXPORT void report_thread_times(RawArray<const wall_time_t> times, const string& name="");

enum thread_type_t { MASTER=0, CPU=1, IO=2 };
OTHER_EXPORT thread_type_t thread_type();

// Initialize thread pools
OTHER_EXPORT void init_threads(int cpu_threads, int io_threads);

// Grab thread counts: cpu count, io count
OTHER_EXPORT Vector<int,2> thread_counts();

// Schedule a job.  Schedule at the back of the queue if !soon, or the front if soon.
OTHER_EXPORT void threads_schedule(thread_type_t type, job_t&& f, bool soon=false);

// Wait for all jobs to complete
OTHER_EXPORT void threads_wait_all();

// Join the CPU thread pool until all jobs complete
OTHER_EXPORT void threads_wait_all_help();

// A historical event
struct history_t {
  event_t event;
  wall_time_t start, end;

  history_t()
    : event(0), start(0), end(0) {}

  history_t(event_t event, wall_time_t start, wall_time_t end)
    : event(event), start(start), end(end) {}
};

// Operations on tracked history.  If history is untracked, these are trivial.
OTHER_EXPORT bool thread_history_enabled();
OTHER_EXPORT vector<vector<Array<const history_t>>> thread_history();
OTHER_EXPORT void write_thread_history(const string& filename);

}