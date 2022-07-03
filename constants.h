#define MAX_PAGES 64

#define MAX_THREADS 20
#define MAX_PG_PER_THREAD 50
#define WORKING_SET_LIMIT 4

#define PAGE_BITS 5 
#define OFFSET_BITS (32 - PAGE_BITS)

typedef struct {
  int pid;
  int frame_number;
} page_id_t;

typedef struct { 
  int pid;
  int status;
  page_id_t* wsl;
} process_t;

typedef struct {
  page_id_t page_id;
  int last_accessed;
} frame_t;

typedef struct {
  page_id_t page_id;
  int frame;
} table_item_t;

typedef struct {
  int pid;
  table_item_t entries[MAX_PG_PER_THREAD];
} table_t;

void setup_main_memory(void);
process_t empty_process(void);
page_id_t* empty_wsl(void);
void setup_process_list(void);
frame_t empty_frame(void);
