#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <ncurses.h>

#include "constants.h"

process_t process_list[MAX_THREADS];
frame_t main_memory[MAX_PAGES];

WINDOW *memory_window, *ptable_window, *processes_window, *log_window;
FILE *log_file;
const int lines = 49;
const int columns = 193;

int process_index = 0;
int running_processes = 0;

int main(void) {
  setup_main_memory();
  setup_process_list();
  setup_windows();
  setup_logging();

  for (int tick = 0; tick < 300; tick++) {
    if (tick % 3 == 0 && tick != 0) {
      // run_processes(tick);
      spawn_new_process();
    } 

    if (tick > 3) run_process(tick);

    print_processes();
    print_frames();

    update_log();
    wclear(log_window);
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
      running_processes++;
      break;
    }
  }
}

bool is_empty_process(int index) {
  return process_list[index].pid == 0 && 
    process_list[index].status == -1 &&
    process_list[index].working_set == -1;
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

      wprintw(log_window,"[run_processes] Process %6i will access page %2i\n", pid, page);
      fprintf(log_file,"[%li](run_processes){access,%06i,%02i}\n",time(NULL),pid, page);
      fflush(log_file);

      bool working_set_full = process_list[i].working_set == WORKING_SET_LIMIT;
      if (working_set_full)
        limited_lru(pid, page, tick);
      else {
        if (is_page_on_memory(page, pid)) access_page(page, pid, tick);
        else if (memory_is_full()) lru(pid, page, tick);
        else allocate_page(pid, page, tick);
      }

      print_ptable(process_list[i]);
      usleep(750000);
    }
  }
}

void run_process(int tick) {
  int address = generate_random_address();
  int page = get_page_number_from_address(address);
  int pid = process_list[process_index].pid;

  if (process_list[process_index].working_set == WORKING_SET_LIMIT)
    limited_lru(pid, page, tick);
  else {
    if (is_page_on_memory(page, pid)) access_page(page, pid, tick);
    else if (memory_is_full()) lru(pid, page, tick);
    else allocate_page(pid, page, tick);
  }

  print_ptable(process_list[process_index]);
  usleep(750000);

  if (process_index + 1 >= running_processes)
    process_index = 0;
  else process_index++;
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
  int pindex = find_process(pid);
  int frame = process_list[pindex].ptable[page];
  fprintf(log_file,"[%li](access_page){access,%06i,%02i,%03i}\n",time(NULL),pid, page, tick);
  fflush(log_file);
  update_frame(main_memory + frame, pid, page, tick);
}

bool memory_is_full() {
  for (int i = 0; i < MAX_PAGES; i++)
    if (is_empty_frame(main_memory[i]))
      return false;
  return true;
}

int find_oldest_process_from_pid(int pid) {
  int oldest = INT_MAX;
  int result;
  for (int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].last_accessed < oldest && pid == main_memory[i].pid) {
      oldest = main_memory[i].last_accessed;
      result = i;
    }
  }
  return result;
}

int find_oldest_process() {
  int oldest = INT_MAX;
  int result;
  for (int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].last_accessed < oldest) {
      oldest = main_memory[i].last_accessed;
      result = i;
    }
  }
  return result;
}

void lru(int pid, int page, int tick) {
  int selected = find_oldest_process();

  fprintf(log_file,"[%li](lru){allocate,%06i,%02i,%03i}\n",time(NULL),pid, page, tick);
  fflush(log_file);
  update_ptable(main_memory[selected].pid, main_memory[selected].page, -1, -1);
  update_frame(main_memory + selected, pid, page, tick);
  update_ptable(pid, page, 1, selected);
}

void update_frame(frame_t* frame, int pid, int page, int tick) {
  frame->pid = pid;
  frame->page = page;
  frame->last_accessed = tick;
}

void update_ptable(int pid, int page, int wsl_inc, int frame) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (process_list[i].pid == pid) {
      process_list[i].working_set += wsl_inc;
      process_list[i].ptable[page] = frame;
      break;
    }
  }
}

void limited_lru(int pid, int page, int tick) {
  int selected = find_oldest_process_from_pid(pid);

  fprintf(log_file,"[%li](limited_lru){allocate,%06i,%02i,%03i}\n",time(NULL),pid, page, tick);
  fflush(log_file);

  update_ptable(main_memory[selected].pid, main_memory[selected].page, -1, -1);
  update_frame(main_memory + selected, pid, page, tick);
  update_ptable(pid, page, 1, selected);
}

void allocate_page(int pid, int page, int tick) {
  int frame = -1;
  fprintf(log_file,"[%li](allocate_page){allocate,%06i,%02i,%03i}\n",time(NULL),pid, page, tick);
  for (int i = 0; i < MAX_PAGES; i++) {
    if (is_empty_frame(main_memory[i])) {
      update_frame(main_memory + i, pid, page, tick);
      frame = i;
      break;
    }
  }

  update_ptable(pid, page, 1, frame);
}

void print_ptable(process_t process) {
  wclear(ptable_window);
  wprintw(ptable_window, "+--------------------------+\n");
  wprintw(ptable_window, "|  PTABLE     %6i       |\n", process.pid);
  wprintw(ptable_window, "+--------------------------+\n");
  for(int i = 0; i < (int) MAX_PG_PER_THREAD / 2; i++)
    wprintw(ptable_window,"| %4i | %2i      %4i | %2i |\n", 
        i, process.ptable[i], (int) MAX_PG_PER_THREAD / 2 + i, process.ptable[(int) MAX_PG_PER_THREAD / 2 + i] );
  wprintw(ptable_window, "+--------------------------+\n");
  wrefresh(ptable_window);
}

void print_frames() {
  wclear(memory_window);
  wprintw(memory_window, "+-----------------------------------------------------+\n");
  wprintw(memory_window, "| FRAM | PID.PG | LAST                                |\n");
  wprintw(memory_window, "+-----------------------------------------------------+\n");
  for (int i = 0; i < (int) MAX_PAGES / 2; i++)
    wprintw(memory_window, "|  %2i | %6i.%2i | %4i       %2i | %6i.%2i | %4i  |\n", 
        i, main_memory[i].pid, main_memory[i].page, main_memory[i].last_accessed,
        (int) MAX_PAGES / 2 + i, main_memory[(int) MAX_PAGES / 2 + i].pid, 
        main_memory[(int) MAX_PAGES / 2 + i].page, main_memory[(int) MAX_PAGES / 2 + i].last_accessed);
  wprintw(memory_window, "+-----------------------------------------------------+\n");
  wrefresh(memory_window);
}

void print_processes() { 
  wclear(processes_window);
  wprintw(processes_window, "+----------------------------+\n");
  wprintw(processes_window, "| PROCESS LIST               |\n");
  wprintw(processes_window, "+----------------------------+\n");
  for (int i = 0; i < (int) MAX_THREADS / 2; i++)
    wprintw(processes_window, "| %5i | %2i      %5i | %2i |\n", 
        process_list[i].pid, process_list[i].working_set,
        process_list[(int) MAX_THREADS / 2 + i].pid, process_list[(int) MAX_THREADS / 2 + i].working_set);
  wprintw(processes_window, "+----------------------------+\n");
  wrefresh(processes_window);
}

void setup_windows() {
  initscr();

  int ptable_h = (int) MAX_PG_PER_THREAD / 2 + 4;
  int ptable_w = (int) columns / 5;
  int ptable_ypos = 0;
  int ptable_xpos = 0;

  int pwin_h = (int) MAX_THREADS / 2 + 4;
  int pwin_w = (int) columns / 5;
  int pwin_ypos = 0;
  int pwin_xpos = (int) columns / 5;

  int memwin_h = (int) MAX_PAGES / 2 + 4;
  int memwin_w = (int) 2 * columns / 5;
  int memwin_ypos = 0;
  int memwin_xpos = (int) 2 * columns / 5;

  int logwin_h = (int) MAX_PG_PER_THREAD / 2;
  int logwin_w = (int) columns / 5;
  int logwin_ypos = (int) MAX_PG_PER_THREAD / 2 + 4;
  int logwin_xpos = 0;

  ptable_window = newwin(ptable_h, ptable_w, ptable_ypos, ptable_xpos);
  processes_window = newwin(pwin_h, pwin_w, pwin_ypos, pwin_xpos);
  memory_window = newwin(memwin_h, memwin_w, memwin_ypos, memwin_xpos);
  log_window = newwin(logwin_h, logwin_w, logwin_ypos, logwin_xpos);

  refresh();

  print_ptable(process_list[0]);
  print_processes();
  print_frames();
}

bool is_empty_frame(frame_t frame) {
  return frame.last_accessed == -1 && frame.page == -1 && frame.pid == 0;
}

int find_process(int pid) {
  for (int i = 0; i < MAX_THREADS; i++)
    if (process_list[i].pid == pid)
      return i;
  return -1;
}

void update_log() { wrefresh(log_window); }

void setup_logging() { 
  char filename[40];
  sprintf(filename, "mem.%li.log", time(NULL));
  log_file = fopen(filename, "wa"); 
}
