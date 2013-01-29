#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "tsu.h"

#define Bool   unsigned int
#define True   1
#define False  0

#define Int32   int
#define UInt32  unsigned int
#define Char    char
#define UChar   unsigned char
#define Int16   short
#define UInt16  unsigned short

#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct fp_worker{
  void (*work_fn) (void);
  uint64_t sc;

  Int32 i;
  Int32 lo;
  Int32 hi;
  Int32 h;
  Int32 d;
  Int32 last;
  uint64_t workLimit;
  Bool firstAttempt;
  UChar *block;
  Int32 *zptr;
  UInt16 *quadrant;
  uint64_t *workDone_p;
}fp_worker;

volatile Int32 tasks_created=0;
volatile Int32 tasks_consumed=0;

Int32 incs[14] = { 1, 4, 13, 40, 121, 364, 1093, 3280,
                   9841, 29524, 88573, 265720,
                   797161, 2391484 };

Bool df_fullGtU ( Int32 i, UChar *block, Int32 last, UInt16* quadrant,
		  uint64_t *workDone_p, Int32 i1, Int32 i2 )
{
  int delay = DELAY,k;
  int sum = 0;
  
  for (k = 0; k < delay; k++) sum+=(k%17);

  __sync_add_and_fetch (workDone_p, 1);

  return ((i+sum)%2 == 0);
}

void work_func (Int32 i, Int32 lo, Int32 h, Int32 hi, Int32 d,
		UChar *block, Int32 last, Int32 *zptr, Int16 *quadrant,
		uint64_t *workDone_p, uint64_t workLimit, Bool firstAttempt)
{
  Int32 v,j;

  v = zptr[i];
  j = i;

  while (df_fullGtU (i, block, last, quadrant, workDone_p, zptr[j-h]+d, v+d)) {
    zptr[j] = zptr[j-h];
    j = j - h;
    if (j <= (lo + h - 1)) break;
  }

  zptr[j] = v;
}

#define NUM_TASKS 100
#define GET_STEP(h) ((h/NUM_TASKS)+((h%NUM_TASKS)!=0))
void start_work_func ()
{
  fp_worker *fp = (fp_worker *) dfs_tload ();
  Int32 ii = fp->i;
  Int32 lo = fp->lo;
  Int32 hi = fp->hi;
  Int32 h = fp->h;
  Int32 d = fp->d;
  UChar *block = fp->block;
  Int32 last = fp->last;
  Int32 *zptr = fp->zptr;
  UInt16 *quadrant = fp->quadrant;
  uint64_t *workDone_p = fp->workDone_p;
  uint64_t workLimit = fp->workLimit;
  Bool firstAttempt = fp->firstAttempt;
  Int32 step = GET_STEP(h);

  Int32 i,v,j;

  for (i = ii; i <= hi; i+=h)
    {
      Int32 local_upper_bound = h*((i-lo)/h+1)+lo;
      for (j = i; j < i+step && j < local_upper_bound && j <= hi; j++)
	work_func (j, lo, h, hi, d, block, last, zptr, quadrant, workDone_p, workLimit, firstAttempt);
    }

  dfs_tend ();

  __sync_add_and_fetch (&tasks_consumed, 1);
  /* fprintf (stderr, "tasks_created:%d, task_consumed:%d\n", tasks_created, tasks_consumed); */
}

void df_simpleSort ( UChar *block, Int32 last, Int32 *zptr, UInt16 *quadrant,
		     uint64_t *workDone_p, uint64_t workLimit, Bool firstAttempt,
		     Int32 lo, Int32 hi, Int32 d )
{
  Int32 i, j, h, bigN, hp;
  Int32 v;

  bigN = hi - lo + 1;
  if (bigN < 2) return;

  hp = 0;
  while (incs[hp] < bigN) hp++;
  hp--;

  for (; hp >= 0; hp--) {
    h = incs[hp];

    Int32 step = GET_STEP(h);

    for (i = lo+h; i < lo+h+h; i+=step)
      {
	/* fprintf (stderr, "***** %d created\n",i); */
	tasks_created++;

	if (*workDone_p > workLimit && firstAttempt) return;

	fp_worker *fp = dfs_tcreate (1, sizeof (fp_worker), start_work_func);
	dfs_twrite_32 (fp, offsetof (fp_worker, i), i);
	dfs_twrite_32 (fp, offsetof (fp_worker, lo), lo);
	dfs_twrite_32 (fp, offsetof (fp_worker, hi), hi);
	dfs_twrite_32 (fp, offsetof (fp_worker, h), h);
	dfs_twrite_32 (fp, offsetof (fp_worker, d), d);
	dfs_twrite_32 (fp, offsetof (fp_worker, last), last);
	dfs_twrite_32 (fp, offsetof (fp_worker, firstAttempt), firstAttempt);
	dfs_twrite (fp, offsetof (fp_worker, workLimit), workLimit);
	dfs_twrite (fp, offsetof (fp_worker, block), block);
	dfs_twrite (fp, offsetof (fp_worker, zptr), zptr);
	dfs_twrite (fp, offsetof (fp_worker, quadrant), quadrant);
	dfs_twrite (fp, offsetof (fp_worker, workDone_p), workDone_p);
	dfs_tdecrease (fp);
      }
    while (tasks_created != tasks_consumed)
      {
	fflush (stdout);
      }
  }

  uint64_t waiting = 0;
  while (tasks_created != tasks_consumed)
    {
      fflush(stdout);//fprintf (stderr,"tasks_created:%d\n",tasks_created);
    }

  fprintf (stderr,"workdone:%ld\n",*workDone_p);
}

int main (int argc, char **argv)
{
  UChar *block;
  Int32 last=LAST;
  Int32 size=last+30;
  Int32 *zptr = malloc (size * sizeof (Int32));
  UInt16 *quadrant;
  uint64_t workDone=0;
  uint64_t *workDone_p=&workDone;
  uint64_t workLimit=1<<60;
  Bool firstAttempt=0;
  Int32 lo=0;
  Int32 hi=last;
  Int32 d=0;
  Int32 i;

  struct timeval *start = (struct timeval *) malloc (sizeof (struct timeval));
  struct timeval *end = (struct timeval *) malloc (sizeof (struct timeval));

  for (i = 0; i < size; i++)
    zptr[i] = i;

  gettimeofday (start, NULL);
  df_simpleSort ( block, last, zptr, quadrant, workDone_p, workLimit, firstAttempt, lo, hi, d );
  gettimeofday (end, NULL);

  fprintf (stderr, "[df_main] execution time: %.5f seconds\n", tdiff (end, start));

  #ifdef _DEBUG
  for (i = 0; i < size; i++)
    fprintf (stderr, "%d,", zptr[i]);
  fprintf (stderr,"\n");
  #endif
}
