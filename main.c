#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "constants.h"

process_t process_list[MAX_THREADS];
frame_t main_memory[MAX_PAGES];
table_t process_tables[MAX_THREADS];

int main(void) {
  setup_main_memory();
  setup_process_list();
  setup_process_tables();

  for (int tick = 0; true; tick++) {
    if (tick % 3 == 0 && tick != 0) {
      run_processes(tick);
      spawn_new_process();
    } sleep(1);
  }

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
    .wsl = empty_wsl(),
  };
}

page_id_t* empty_wsl(void) {
  page_id_t* result = calloc(sizeof(page_id_t), WORKING_SET_LIMIT);
  for (int i = 0; i < WORKING_SET_LIMIT; i++)
    result[i] = (page_id_t) { .pid = 0, .frame_number = -1,};
  return result;
}

void setup_process_list(void) {
  for (int i = 0; i < MAX_THREADS; i++)
    process_list[i] = empty_process();
}

void setup_process_tables() {
  for (int i = 0; i < MAX_THREADS; i++)
    process_tables[i] = empty_table();
}

table_t empty_table() {
  return (table_t) {
    .pid = 0,
    .entries = empty_entries(),
  };
}

table_item_t* empty_entries() {
  table_item_t* result = calloc(sizeof(table_item_t), MAX_PG_PER_THREAD);
  for (int i = 0; i < MAX_PG_PER_THREAD; i++)
    result[i] = empty_table_item();
  return result;
}

table_item_t empty_table_item() {
  return (table_item_t) {
    .page_id = (page_id_t) {
      .pid = 0,
      .frame_number = -1,
    },
  };
}

frame_t empty_frame(void) {
  return (frame_t) {
    .last_accessed = -1,
    .page_id = (page_id_t) {
      .pid = 0,
      .frame_number = -1,
    }
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
    .wsl = empty_wsl(),
  };
}

void run_processes(int tick) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (!is_empty_process(i)) {
      int address = generate_random_address();
      int page = get_page_number_from_address(address);
      int offset = get_offset_from_address(address);
      int pid = process_list[i].pid;

      if (is_page_on_memory(page, pid)) {
        access_page(page, pid, tick);
      } else {
        puts("Página não tá na memória, vou ter que adicionar");
      }

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
    if (process_tables[i].pid == pid) {
      for (int j = 0; j < MAX_PG_PER_THREAD; j++) {
        if (process_tables[i].entries[j].page_id.frame_number == page)
          return true;
      }
    }
  }
  return false;
}

void access_page(int page, int pid, int tick) {
  for (int i = 0; i < MAX_PAGES; i++) {
    if (main_memory[i].page_id.pid == pid && main_memory[i].page_id.frame_number == page) {
      main_memory[i].last_accessed = tick;
      break;
    }
  }
}
