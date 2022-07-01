#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "constants.h"

process_t proclist[MAX_PROC];
frame_t primary_memory[64];

int p_index = 0;
int m_index = 0;

int main(void) {

  printf("sizeof(int) = %li\n", sizeof(int));
  printf("sizeof(u_int32_t) = %li\n", sizeof(u_int32_t));
  printf("sizeof(long int) = %li\n", sizeof(long int));

  for ( int i = 0; true; i++ ) {
    if ( i % 3 == 0 && i != 0 ) {

      for ( int j = 0; j < p_index; j++ ) {
        puts("Processos serão processados");
        u_int32_t address = generate_random_address();
        puts("Processos serão processados");
        u_int32_t pg = get_page_number_from_address(address);
        printf("%i %i\n", address, pg);

        puts("Processos serão processados");
        u_int32_t offset = get_offset_from_address(address);
        puts("Processos serão processados");

        char page_id[12];

        sprintf(page_id, "%8i.%02i", proclist[j].pid, pg);
        printf("Page access: %s\n", page_id);

        if (!is_page_on_memory(page_id)) {
          puts("puts, a página não tá na mem");
        } else {
          puts("puts, a página tá na mem");
        }
      }

      puts("3 ticks passaram, ciclo de CPU");
      if ( p_index <= MAX_PROC ) {
        puts("Cria processo");

        proclist[p_index] = spawn_new_process();
        p_index++;
      } else break;
    }
    // sleep(1);
  }


  return 0;
}

process_t spawn_new_process() {
  int pid = rand() % MAX_PID;
  int status = Ready;
  char* wsl = calloc(sizeof(char), WSL);

  return (process_t) {
    .pid = pid,
    .status = status,
    .wsl = wsl,
  };
}

u_int32_t generate_random_address() {
  u_int32_t limit = 0xc9000000;
  u_int32_t random = (u_int32_t) rand();
  while (random > limit) 
    random = (u_int32_t) rand();
  return random;
}

u_int32_t get_page_number_from_address(u_int32_t address) {
  u_int32_t result = (address & 0xff000000) >> OFFSET;
  return result;
}

// TODO: conversão de uint32 pra int não tá funcionando por algum motivo
u_int32_t get_offset_from_address(u_int32_t address) {
  u_int32_t result = (address & 0x00ffffff);
  return result;
}

bool is_page_on_memory(char* page_id) {
  bool result = false; 
  for ( int i = 0; i < m_index; i++ ) {
    if (strcmp(primary_memory[i].page_id, page_id) == 0) {
      result = true;
      break;
    }
  } return result;
}
