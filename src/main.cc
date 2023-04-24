#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctype.h>
#include <cstdio>
#include <unistd.h>
#include "process.h"
#include "pager.h"

char * infile;
char * randfile;

int MAX_FRAMES = 128;
int MAX_VPAGES = 64;
int totalRandomNum;
int currentRandomNum = 0;

std::vector<int> randvals;
std::vector<std::pair<char, int>> instructions;
std::vector<Process> processes;
Pager *THE_PAGER;
pte_t *page_table = new pte_t[MAX_VPAGES];
frame_t *frame_table;
deque<int> freelist;

bool verbose = false;
bool pagetableOpt = false;
bool frametableOpt = false;
bool summaryOpt = false;
long inst_count = 0;
long ctx_switches = 0;
long process_exits = 0;
long rw_count = 0;
long long cost = 0;

char operation;
int vpage;

void parse_args(int argc, char **argv);
void setup_frame_table();
void read_random_numbers(char *fname);
int myrandom(int num);
void read_input_file(char *filename, vector<Process> &processes, vector<pair<char, int>> &instructions);
frame_t *allocate_frame_from_free_list();
frame_t *get_frame();
void simulation();
void print_page_table();
void print_frame_table();
void print_summary();


int main(int argc, char **argv)
{
  parse_args(argc, argv);
  setup_frame_table();

  read_random_numbers(randfile);
  read_input_file(infile, processes, instructions);
  simulation();

  if (pagetableOpt)
  {
    print_page_table();
  }
  if (frametableOpt)
  {
    print_frame_table();
  }
  if (summaryOpt)
  {
    print_summary();
  }
  return 0;
}

void parse_args(int argc, char **argv)
{
    char *schedspec = NULL;
    int c;
    while ((c = getopt(argc, argv, "f:a:o:")) != -1)
    {
        switch (c)
        {
        case 'f':
            MAX_FRAMES = stoi(optarg);
            break;
        case 'a':
        {
          string type(optarg);
          switch (type[0])
          {
          case 'f':{THE_PAGER = new FIFO; break;}
          case 'r':{THE_PAGER = new Random; break;}
          case 'c':{THE_PAGER = new Clock; break;}
          case 'e':{THE_PAGER = new NRU; break;}
          case 'a':{THE_PAGER = new Aging; break;}
          case 'w':{THE_PAGER = new WorkingSet; break;}
          default:
            break;
          }
          break;
        }
        case 'o':
        {
            string flag = optarg;
            for (char &ch : flag)
            {
                switch (ch)
                {
                case 'O':
                {
                    verbose = true;
                    break;
                }
                case 'P':
                {
                    pagetableOpt = true;
                    break;
                }
                case 'F':
                {
                    frametableOpt = true;
                    break;
                }
                case 'S':
                {
                    summaryOpt = true;
                    break;
                }
                }
            }

            break;
        }
        case '?':
            fprintf(stderr, "\n");
        default:
            abort();
        }
    }

    infile = argv[optind];
    randfile = argv[optind + 1];
}

void setup_frame_table()
{
    frame_table = new frame_t[MAX_FRAMES];
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        frame_table[i].index = i;
        frame_table[i].mapped = 0;
        frame_table[i].pid = -1;
        frame_table[i].vpage = -1;
        frame_table[i].lastUseTime = 0;
        freelist.emplace_back(i);
    }
}

void read_random_numbers(char *fname)
{
  ifstream fin(fname);
  string line;
  int i = 0;
  fin >> totalRandomNum;
  while (i < totalRandomNum)
  {
    fin >> line;
    randvals.emplace_back(atoi(line.c_str()));
    ++i;
  }

  fin.close();
}

int myrandom(int num)
{
  return randvals[currentRandomNum++] % num;
}

void read_input_file(char *filename, std::vector<Process> &processes, std::vector<std::pair<char, int>> &instructions)
{
  ifstream fin(filename);
  string line;
  int start_vpage, end_vpage, write_protected, file_mapped;

  getline(fin, line);
  while (line[0] == '#')
    getline(fin, line);
  int processNum = stoi(line);
  for (int i = 0; i < processNum; i++)
  {
    getline(fin, line);
    while (line[0] == '#')
      getline(fin, line);
    int vmaNum = stoi(line);
    Process p(i);
    for (int j = 0; j < vmaNum; j++)
    {
      getline(fin, line);
      sscanf(line.c_str(), "%d %d %d %d", &start_vpage, &end_vpage, &write_protected, &file_mapped);
      VMA vma(start_vpage, end_vpage, write_protected, file_mapped);
      p.vmalist.emplace_back(vma);
    }
    processes.emplace_back(p);
  }

  while (line[0] == '#')
    getline(fin, line);
  while (getline(fin, line))
  {
    if (line[0] == '#')
      continue;
    char cmd;
    int page;
    sscanf(line.c_str(), "%c %d", &cmd, &page);
    instructions.emplace_back(std::pair<char, int>({cmd, page}));
  }
}

frame_t *allocate_frame_from_free_list()
{
  if (freelist.empty())
    return NULL;
  else
  {
    int fid = freelist.front();
    freelist.pop_front();
    return &frame_table[fid];
  }
}

frame_t *get_frame()
{
  frame_t *frame = allocate_frame_from_free_list();
  if (frame == NULL)
    frame = THE_PAGER->select_victim_frame();
  return frame;
}

void simulation()
{
  for (int i = 0; i < instructions.size(); i++)
  {
    operation = instructions[i].first;
    vpage = instructions[i].second;

    ++inst_count;

    if (verbose)
      printf("%d: ==> %c %d\n", i, operation, vpage);

    Process *current_process;

    if (operation == 'c')
    {
      current_process = &processes[vpage];
      ++ctx_switches;
    }
    else if (operation == 'e')
    {
      ++process_exits;
      printf("EXIT current process %d\n", current_process->pid);
      for (int i = 0; i < MAX_VPAGES; i++)
      {
        pte_t *p = &current_process->page_table[i];
        if (p->present)
        {
          if (verbose)
            printf(" UNMAP %d:%d\n", current_process->pid, i);
          ++current_process->unmaps;
          if (p->modified && p->file_map)
          {
            if (verbose)
              printf(" FOUT\n");
            current_process->fouts++;
          }
          frame_table[p->frameIndex].mapped = 0;
          frame_table[p->frameIndex].pid = -1;
          frame_table[p->frameIndex].vpage = -1;
          freelist.emplace_back(frame_table[p->frameIndex].index);
        }
        p->present = 0;
        p->referenced = 0;
        p->modified = 0;
        p->write_protect = 0;
        p->pageout = 0;
        p->file_map = 0;
      }
      current_process = NULL;
      continue;
    }
    else
    {
      ++rw_count;
      pte_t *pte = &current_process->page_table[vpage];
      if (!pte->present)
      {
        VMA *vma_t = current_process->getAccessibleVMA(vpage);
        if (vma_t)
        {
          pte->file_map = vma_t->file_mapped;
          pte->write_protect = vma_t->write_protected;
        }
        else
        {
          if (verbose)
            printf(" SEGV\n");
          current_process->segv++;
          continue;
        }

        frame_t *newframe = get_frame();
        if (newframe->mapped)
        {
          if (verbose)
            printf(" UNMAP %d:%d\n", newframe->pid, newframe->vpage);
          processes[newframe->pid].unmaps++;
          pte_t *_pte = &processes[newframe->pid].page_table[newframe->vpage];
          if (_pte->modified)
          {
            if (_pte->file_map)
            {
              if (verbose)
                printf(" FOUT\n");
              processes[newframe->pid].fouts++;
            }
            else
            {
              if (verbose)
                printf(" OUT\n");
              processes[newframe->pid].outs++;
              _pte->pageout = 1;
            }
          }
          _pte->present = 0;
          _pte->referenced = 0;
          _pte->modified = 0;
          _pte->frameIndex = 0;
        }

        if (pte->pageout)
        {
          if (verbose)
            printf(" IN\n");
          current_process->ins++;
        }
        else if (pte->file_map)
        {
          if (verbose)
            printf(" FIN\n");
          current_process->fins++;
        }
        else
        {
          if (verbose)
            printf(" ZERO\n");
          current_process->zeros++;
        }
        pte->frameIndex = newframe->index;
        pte->present = 1;

        newframe->pid = current_process->pid;
        newframe->vpage = vpage;
        newframe->mapped = 1;
        if (verbose)
          printf(" MAP %d\n", pte->frameIndex);
        current_process->maps++;
        newframe->age = 0;
        newframe->lastUseTime = inst_count;
      }

      if (operation == 'r')
      {
        pte->referenced = 1;
      }
      else if (operation == 'w')
      {
        pte->referenced = 1;
        if (pte->write_protect)
        {
          if (verbose)
            printf(" SEGPROT\n");
          current_process->segprot++;
        }
        else
        {
          pte->modified = 1;
        }
      }
    }
  }
}

void print_page_table()
{
  for (int i = 0; i < processes.size(); i++)
  {
    Process *proc = &processes[i];
    printf("PT[%d]:", proc->pid);
    for (int j = 0; j < MAX_VPAGES; j++)
    {
      pte_t *pte = &proc->page_table[j];
      printf(" ");
      if (pte->present)
      {
        printf("%d:%c%c%c", j, pte->referenced ? 'R' : '-', pte->modified ? 'M' : '-', pte->pageout ? 'S' : '-');
      }
      else
      {
        printf("%c", pte->pageout ? '#' : '*');
      }
    }
    printf("\n");
  }
}

void print_frame_table()
{
  printf("FT:");
  for (int i = 0; i < MAX_FRAMES; i++)
  {
    if (frame_table[i].mapped)
    {
      printf(" %d:%d", frame_table[i].pid, frame_table[i].vpage);
    }
    else
    {
      printf(" *");
    }
  }
  printf("\n");
}

void print_summary()
{
  for (int i = 0; i < processes.size(); i++)
  {
    Process *proc = &processes[i];
    printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
           proc->pid,
           proc->unmaps, proc->maps, proc->ins, proc->outs,
           proc->fins, proc->fouts, proc->zeros,
           proc->segv, proc->segprot);
    cost += proc->maps * 350 + proc->unmaps * 410 + proc->ins * 3200 + proc->outs * 2750 + proc->fins * 2350 + proc->fouts * 2800 + proc->zeros * 150 + proc->segv * 440 + proc->segprot * 410;
  }
  cost += rw_count * 1 + ctx_switches * 130 + process_exits * 1230;
  printf("TOTALCOST %lu %lu %lu %llu %lu\n", inst_count, ctx_switches, process_exits, cost, sizeof(pte_t));
}
