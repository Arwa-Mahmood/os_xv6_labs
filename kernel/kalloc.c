// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// ---- COW reference counting ----
#define NPHYPAGES ((PHYSTOP - KERNBASE) / PGSIZE)
#define PA2IDX(pa) (((uint64)(pa) - KERNBASE) / PGSIZE)

static struct {
  struct spinlock lock;
  int count[NPHYPAGES];
} kref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock, "kref");     	
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE){
    kref.count[PA2IDX(p)] = 1;      // ← preset so kfree() drops it to 0
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int c;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Decrement refcount; bail out if anyone else still references the page.
  acquire(&kref.lock);
  c = --kref.count[PA2IDX(pa)];
  release(&kref.lock);

  if (c > 0)
    return;
  if (c < 0)
    panic("kfree: refcount went negative");

  // Last reference gone — actually reclaim the page.

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}


// LAB IMPLEMENTATION

// Increment refcount — used by uvmcopy() when sharing a page with a child.
void
kaddref(void *pa)
{
  acquire(&kref.lock);
  kref.count[PA2IDX(pa)]++;
  release(&kref.lock);
}

// Read refcount — used by cowfault()'s "sole owner?" fast path.
int
krefcount(void *pa)
{
  int c;
  acquire(&kref.lock);
  c = kref.count[PA2IDX(pa)];
  release(&kref.lock);
  return c;
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r){
    memset((char *)r, 5, PGSIZE); // fill with junk
    acquire(&kref.lock);
    kref.count[PA2IDX(r)] = 1;      // fresh page: one owner
    release(&kref.lock); 
  }
   return (void *)r;
}
