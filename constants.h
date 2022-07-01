#define MAX_PROC 20
#define PG_PER_PROC 50
#define WSL 4

#define PG_SECTION 7
#define OFFSET (32 - PG_SECTION)

#define MAX_PID 65535

enum STATUSES { New = 0, Ready, Running, Interrupted, Ended };

typedef struct process_t {
  int pid;
  int status;
  char* wsl[WSL];
} process_t;

typedef struct table_item_t {
  char* page_id;
  int frame;
} table_item_t;

typedef struct table_t {
  int pid;
  table_item_t entries[PG_PER_PROC];
} table_t; 

typedef struct frame_t {
  char* page_id;
  int accessed_at;
} frame_t;

process_t spawn_new_process();
unsigned int generate_random_address();
u_int32_t get_page_number_from_address(u_int32_t);
u_int32_t get_offset_from_address(u_int32_t);
bool is_page_on_memory(char*);
