#pragma once
#include <deque>
#include <iostream>
#include <cstdio>

using namespace std;

extern int MAX_VPAGES;

typedef struct
{
  unsigned int present : 1;
  unsigned int referenced : 1;
  unsigned int modified : 1;
  unsigned int write_protect : 1;
  unsigned int file_map : 1;
  unsigned int pageout : 1;
  unsigned int frameIndex : 16;
  unsigned int empty : 10;
} pte_t;

typedef struct
{
    int pid;
    int vpage;
    int index;
    bool mapped = false;
    unsigned int age : 32;
    unsigned int lastUseTime = 0;
} frame_t;

struct VMA {
    unsigned int start_vpage;
    unsigned int end_vpage;
    bool write_protected;
    bool file_mapped;

    VMA(int svp, int evp, int wp, int fm)
    : start_vpage(svp), end_vpage(evp), write_protected(wp), file_mapped(fm) {}
};

class Process
{
public:
  int pid = 0;
  unsigned long unmaps = 0;
  unsigned long maps = 0;
  unsigned long ins = 0;
  unsigned long outs = 0;
  unsigned long fins = 0;
  unsigned long fouts = 0;
  unsigned long zeros = 0;
  unsigned long segv = 0;
  unsigned long segprot = 0;
  deque<VMA> vmalist;
  pte_t *page_table;

  Process();
  Process(int id);
  void display();
  VMA *getAccessibleVMA(int vpage);
};

Process::Process(int id)
{
  this->pid = id;
  this->page_table = new pte_t[MAX_VPAGES];
  for (int i = 0; i < MAX_VPAGES; i++)
  {
    page_table[i].frameIndex = 0;
    page_table[i].file_map = 0;
    page_table[i].modified = 0;
    page_table[i].pageout = 0;
    page_table[i].present = 0;
    page_table[i].referenced = 0;
    page_table[i].write_protect = 0;
    page_table[i].empty = 0;
  }
}

Process::Process()
{
  this->page_table = new pte_t[MAX_VPAGES];
  for (int i = 0; i < MAX_VPAGES; i++)
  {
    page_table[i].frameIndex = 0;
    page_table[i].file_map = 0;
    page_table[i].modified = 0;
    page_table[i].pageout = 0;
    page_table[i].present = 0;
    page_table[i].referenced = 0;
    page_table[i].write_protect = 0;
    page_table[i].empty = 0;
  }
}

void Process::display()
{
  for (const VMA &v : vmalist)
  {
    printf("%d %d %d %d\n", v.start_vpage, v.end_vpage, v.write_protected, v.file_mapped);
  }
}
VMA *Process::getAccessibleVMA(int vpage)
{
  for (auto &vma : this->vmalist)
  {
    if (vma.start_vpage <= vpage && vma.end_vpage >= vpage)
    {
      return &vma;
    }
  }
  return NULL;
}
