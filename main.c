#include <stdio.h> 
#include <stdlib.h>

#include "constants.h"

process_t process_list[MAX_THREADS];
frame_t main_memory[MAX_PAGES];

int main(void) {
  setup_main_memory();
  setup_process_list();

  puts("Setup done, now we emulate");

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

frame_t empty_frame(void) {
  return (frame_t) {
    .last_accessed = -1,
    .page_id = (page_id_t) {
      .pid = 0,
      .frame_number = -1,
    }
  };
}
