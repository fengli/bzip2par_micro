#include <stdio.h>
#include <stdlib.h>
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

Int32 incs[14] = { 1, 4, 13, 40, 121, 364, 1093, 3280,
                   9841, 29524, 88573, 265720,
                   797161, 2391484 };

Bool df_fullGtU ( Int32 i, UChar *block, Int32 last, UInt16* quadrant,
		  uint64_t *workDone_p, Int32 i1, Int32 i2 )
{
  int delay = DELAY,k;
  int sum = 0;

  for (k = 0; k < delay; k++) sum+=(k%17);

  *workDone_p+=1;

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
    for (i = lo+h; i <= hi; i++)
      {
	work_func (i, lo, h, hi, d, block, last, zptr, quadrant, workDone_p, workLimit, firstAttempt);
	if (*workDone_p > workLimit && firstAttempt) return;
      }
  }

  fprintf (stderr, "workdone:%ld\n",*workDone_p);
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
  Bool firstAttempt;
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

  fprintf (stderr, "[main] execution time: %.5f seconds\n", tdiff (end, start));

  #ifdef _DEBUG
  for (i = 0; i < size; i++)
    fprintf (stderr, "%d,", zptr[i]);
  fprintf (stderr,"\n");
  #endif
}
