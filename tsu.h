#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include <sys/time.h>
#include <unistd.h>

#define LAST 10000
#define DELAY 1000

// #define _DEBUG

static inline double
tdiff (struct timeval *end, struct timeval *start)
{
  return (double)end->tv_sec - (double)start->tv_sec +
    (double)(end->tv_usec - start->tv_usec) / 1e6;
}

void *dfs_tload (void);
void dfs_tstore (size_t id, size_t offset, void *buffer, size_t size);
void dfs_twrite (size_t id, size_t offset, size_t value);
void dfs_twrite_32 (size_t id, size_t offset, size_t value);
void dfs_twrite_16 (size_t id, size_t offset, size_t value);
size_t dfs_tcreate (size_t sc, size_t size, void *wfn);
void dfs_tdecrease (size_t frame_id);
void dfs_tdecrease_n (size_t frame_id, size_t n);
void dfs_tend ();






