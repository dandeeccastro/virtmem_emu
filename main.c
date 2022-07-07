#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ncurses.h>

#include "constants.h"

process_t process_list[MAX_THREADS];
frame_t main_memory[MAX_PAGES];
WINDOW *memory_window, *ptable_window;

int main(void) {
  setup_main_memory();
  setup_process_list();
  setup_windows();

  for (int tick = 0; true; tick++) {
    if (tick % 3 == 0 && tick != 0) {
      run_processes(tick);
      spawn_new_process();
    } sleep(1);
    print_frames();
  }

  endwin();

  return 0;
}

void setup_main_memory(void) {
  for(int i = 0; i < MAX_PAGES; i++)
    main_memory[i] = empty_frame();
}

process_t empty_process(void) {
  return (process_t) {
    .pid = 0,
    .status = -1,
    .working_set = -1,
    .ptable = empty_ptable(),
  };
}

void setup_process_list(void) {
  for (int i = 0; i < MAX_THREADS; i++)
    process_list[i] = empty_process();
}

frame_t empty_frame(void) {
  return (frame_t) {
    .pid = 0,
    .page = -1,
    .last_accessed = -1,
  };
}

void spawn_new_process(void) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (is_empty_process(i)) {
      process_list[i] = generate_random_process();
      break;
    }
  }
}

bool is_empty_process(int index) {
  return process_list[index].pid == 0 && 
    process_list[index].status == -1;
}

process_t generate_random_process() {
  return (process_t) {
    .pid = rand() % 65535,
    .status = Ready, 
    .working_set = 0,
    .ptable = empty_ptable(),
  };
}

int* empty_ptable () {
  int* result = calloc(sizeof(int), MAX_PG_PER_THREAD);
  for ( int i = 0; i < MAX_PG_PER_THREAD; i++) 
    result[i] = -1;
  return result;
}

void run_processes(int tick) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (!is_empty_process(i)) {
      int address = generate_random_address();
      int page = get_page_number_from_address(address);
      int offset = get_offset_from_address(address);
      int pid = process_list[i].pid;

      if (is_page_on_memory(page, pid))
        access_page(page, pid, tick);
      else {
        bool working_set_full = process_list[i].working_set == 4;
        if (working_set_full) limited_lru(pid, page, tick);
        else if (memory_is_full()) lru(pid, page, tick);
        else allocate_page(pid, page, tick);
      }
      usleep(500000);
      print_ptable(process_list[i]);
    }
  }
}

int generate_random_address() {
  unsigned int limit = 0xcc000000;
  unsigned int address = rand();
  while (address > limit)
    address = rand();
  return (int) address;
}

int get_page_number_from_address(int address) {
  return (address & 0xff000000) >> OFFSET_BITS;
}

int get_offset_from_address(int address) {
  return address & 0x00ffffff;
}

bool is_page_on_memory(int page, int pid) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (process_list[i].pid == pid) {
      return process_list[i].ptable[page] != -1;
    }
  } return false;
}

void access_page(int page, int pid, int tick) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (process_list[i].pid == pid) {
      int frame = process_list[i].ptable[page];
      main_memory[frame].last_accessed = tick;
    }
  }
}

bool memory_is_full() {
  for (int i = 0; i < MAX_PAGES; i++)
    if (main_memory[i].pid == 0 && main_memory[i].page == -1 && main_memory[i].last_accessed == -1)
      return false;
  return true;
}

void lru(int pid, int page, int tick) {
  int selected = -1;
  int oldest = INT_MAX;

  for(int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].last_accessed < oldest) {
      oldest = main_memory[i].last_accessed;
      selected = i;
    }
  }

  int swappedPID = main_memory[selected].pid;
  int swappedPage = main_memory[selected].page;

  main_memory[selected].pid = pid;
  main_memory[selected].page = page;
  main_memory[selected].last_accessed = tick;

  for(int i = 0; i < MAX_THREADS; i++) {
    if (process_list[i].pid == swappedPID) {
      process_list[i].working_set--;
      process_list[i].ptable[swappedPage] = -1;
      break;
    }
  }
}

void limited_lru(int pid, int page, int tick) {
  int selected = -1;
  int oldest = INT_MAX;

  for(int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].last_accessed < oldest && main_memory[i].pid == pid) {
      oldest = main_memory[i].last_accessed;
      selected = i;
    }
  }

  int swappedPID = main_memory[selected].pid;
  int swappedPage = main_memory[selected].page;

  main_memory[selected].pid = pid;
  main_memory[selected].page = page;
  main_memory[selected].last_accessed = tick;

  for(int i = 0; i < MAX_THREADS; i++) {
    if (process_list[i].pid == swappedPID) {
      process_list[i].working_set--;
      process_list[i].ptable[swappedPage] = -1;
      break;
    }
  }
}

void allocate_page(int pid, int page, int tick) {
  for (int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].pid == 0 && main_memory[i].page == -1 && main_memory[i].last_accessed == -1) {
      main_memory[i].pid = pid;
      main_memory[i].page = page;
      main_memory[i].last_accessed = tick;
      break;
    }
  }
}

void print_ptable(process_t process) {
  wprintw(ptable_window,"[%04i]\n", process.pid);
  for(int i = 0; i < MAX_PG_PER_THREAD; i++)
    wprintw(ptable_window,"[%04i | %02i]\n", i, process.ptable[i]);
  wrefresh(ptable_window);
  // refresh();
}

void print_frames() {
  wprintw(memory_window, "[FRAM | PID.PG | LAST]\n");
  for (int i = 0; i < MAX_PAGES; i++)
    wprintw(memory_window,"[%02i | %04i.%02i | %04i]\n", 
        i, main_memory[i].pid, main_memory[i].page, main_memory[i].last_accessed);
  wrefresh(memory_window);
  // refresh();
}

void setup_windows() {
  initscr();

  ptable_window = newwin(MAX_PG_PER_THREAD,14,0,0);
  memory_window = newwin(MAX_PAGES, 24, 0, 14);
  box(ptable_window,0,0);
  box(memory_window,0,14);

  refresh();

  print_ptable(process_list[0]);
  print_frames();
}
