//******************************************************************************
// global include files
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>



//******************************************************************************
// local include files
//******************************************************************************

#include <cskiplist.h>



//******************************************************************************
// macros
//******************************************************************************

#define STRMAX     8096
#define MAX_HEIGHT 4



//******************************************************************************
// types
//******************************************************************************

typedef struct {
  void *start;
  void *end;
} interval_t;



//******************************************************************************
// private operations
//******************************************************************************

static interval_t *
interval_new
(
  long start_l, 
  long end_l
)
{
  interval_t *result = (interval_t *) malloc(sizeof(interval_t));

  result->start = (void *) start_l;
  result->end = (void *) end_l;

  return result;
}


static int
interval_inrange
(
 void *interval_v, 
 void *address
)
{
  interval_t *interval = (interval_t *) interval_v;

  return ((address >= interval->start) && (address < interval->end));
}
   

static int
interval_compare
(
 void* lhs_v, 
 void* rhs_v
)
{
  interval_t* lhs = (interval_t*) lhs_v;
  interval_t* rhs = (interval_t*) rhs_v;

  if (lhs->start < rhs->start) return -1;

  if (lhs->start == rhs->start) return 0;

  return 1;
}


static void
interval_cskiplist_node_tostr
(
 void* node_val, 
 int node_height, 
 int max_height,
 char str[], 
 int max_cskl_str_len
)
{
  interval_t *interval = (interval_t *) node_val;
  sprintf(str," [0x%016lx, 0x%016lx)\n", interval->start, interval->end);

}


static void
build_test
(
 cskiplist_t *list, 
 int len, 
 char* s, 
 int reversed 
)
{
  printf("Filling list of length %d ...\n", len);

  printf("before ...\n");
  cskl_dump(list, interval_cskiplist_node_tostr);

  ptrdiff_t i;
  for (i = len; i > 0; i = i -10) {
    ptrdiff_t lo = reversed ? i: len - i;
    interval_t *e = interval_new((uintptr_t)(s+lo), (uintptr_t)(s+lo+8));
    cskl_insert(list, e, malloc);
  }
  
  printf("after ...\n");
  cskl_dump(list, interval_cskiplist_node_tostr);
}



//******************************************************************************
// interface operations
//******************************************************************************

int 
main
(
 int argc,
 char **argv
)
{
  cskl_init();
  
  interval_t* lsentinel = interval_new(0,0);
  interval_t* rsentinel = interval_new(UINTPTR_MAX, UINTPTR_MAX);

  cskiplist_t *cs = 
    cskl_new(lsentinel, rsentinel, MAX_HEIGHT, interval_compare, 
	     interval_inrange, malloc);

#if 1
#pragma omp parallel 
{
	int id = omp_get_thread_num();
	int lo = 10 + 10 * id;
	int hi = lo + 5;
        interval_t *e = interval_new(lo, hi);
        cskl_insert(cs, e, malloc);
}

  cskl_dump(cs, interval_cskiplist_node_tostr);

  cskl_check_dump(cs, interval_cskiplist_node_tostr);
#else

  build_test(cs, 100, 0, 4);

  cskl_check_dump(cs, interval_cskiplist_node_tostr);

#endif

  return 0;
}
