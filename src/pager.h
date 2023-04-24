#include "process.h"
#include <vector>
#include <stdio.h>

typedef struct {
  int pid;
  int vpage;

  // // other stuff you probably want to add
  bool mapped = false;
  int index;
  unsigned int age : 32;
  unsigned int lastUseTime = 0;
} frame_t;

class Pager
{
public:
  virtual frame_t* select_victim_frame() = 0;   // virtual base class
};

class FCFS: public Pager
{
public:
  int hand;

  FCFS();
  frame_t* select_victim_frame();
};

class Random: public Pager
{
public:
  int hand;

  Random();
  frame_t* select_victim_frame();
};

class Clock: public Pager
{
public:
  int hand;

  Clock();
  frame_t* select_victim_frame();
};

class NRU: public Pager
{
public:
  int hand;
  long lastCount=0;
  int period=50;

  NRU();
  frame_t* select_victim_frame();
};

class Aging: public Pager
{
public:
  int hand;

  Aging();
  frame_t* select_victim_frame();
};

class WorkingSet: public Pager
{
public:
  int hand;
  int tau;

  WorkingSet();
  frame_t* select_victim_frame();
};



extern int MAX_FRAMES;
extern frame_t *frame_table;
extern vector<Process> processes;
extern int myrandom(int num);
extern long inst_count;
FCFS::FCFS()
{
    int hand = 0;
}

frame_t* FCFS::select_victim_frame(){
    frame_t* victim = &frame_table[hand];
    hand = (hand+1)%MAX_FRAMES;
    return victim;
}

Random::Random(){
    int hand = 0;
}

frame_t* Random::select_victim_frame (){
    hand = myrandom(MAX_FRAMES);
    frame_t* victim = &frame_table[hand];
    return victim;
}

Clock::Clock()
{
    int hand=0;
}

frame_t* Clock::select_victim_frame(){
    while (true) {
        frame_t* f=&frame_table[hand];
        Process p=processes[f->pid];
        unsigned int R = p.page_table[f->vpage].referenced;
        if (R==0) {
            hand = (hand+1)%MAX_FRAMES;
            return f;
        }
        else {
            processes[f->pid].page_table[f->vpage].referenced=0;
            hand = (hand+1)%MAX_FRAMES;
        }
    }
}

NRU::NRU()
{
    long lastCount=0;
    int period=50;
    int hand = 0;
}

frame_t* NRU::select_victim_frame(){
    vector<int> classes(4, -1);
    bool reset = inst_count-lastCount >= period;
    if (reset) {
        lastCount = inst_count;
    }
    for (int i=0; i < MAX_FRAMES; i++) {
        frame_t f=frame_table[hand];
        Process p = processes[f.pid];
        pte_t *pte = &p.page_table[f.vpage];
        int c=pte->referenced * 2 + pte->modified;
        if (classes[c]==-1) {
            classes[c] = hand;
        }
        if (reset) {
            pte->referenced=0;
        }else if (!c) {
            break;
        }
        hand = (hand+1)%MAX_FRAMES;
    }
    for (int i=0; i < 4; i++) {
        if (classes[i] >= 0) {
            hand = (classes[i] + 1) % MAX_FRAMES;
            return &frame_table[classes[i]];
        }
    }
    return NULL;
}

Aging::Aging(){
    hand=0;
}

frame_t* Aging::select_victim_frame(){
    frame_t *victum=NULL;
    for (int i=0; i < MAX_FRAMES; i++) {
        frame_t* f=&frame_table[hand];
        f->age >>= 1;
        if (processes[f->pid].page_table[f->vpage].referenced) {
            f->age |= 0x80000000;
            processes[f->pid].page_table[f->vpage].referenced = 0;
        }
        if (!f->age) {
            return f;
        }
        if (!victum || f->age < victum->age) {
            victum = f;
        }
        hand = (hand+1)%MAX_FRAMES;
    }
    hand = (victum->index + 1)%MAX_FRAMES;
    return victum;
}

WorkingSet::WorkingSet(){
    hand=0;
    tau=49;
}

frame_t* WorkingSet::select_victim_frame(){
    frame_t* victum;
    frame_t* oldestFrame=NULL;
    for (int i=0; i < MAX_FRAMES+1; i++) {
        frame_t *f = &frame_table[hand];
        pte_t* p = &processes[f->pid].page_table[f->vpage];
        if (p->referenced) {
            f->lastUseTime=inst_count;
            p->referenced=0;
        }
        else
        {
            if (inst_count - f->lastUseTime > tau) {
                victum=f;
                hand = (hand+1)%MAX_FRAMES;
                return f;
            }
            else if(!oldestFrame||f->lastUseTime < oldestFrame->lastUseTime){

                oldestFrame=f;
            }
        }
        hand = (hand+1)%MAX_FRAMES;
    }
    hand = (oldestFrame->index+1)%MAX_FRAMES;
    return oldestFrame;
}