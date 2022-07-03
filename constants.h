#define MAX_PAGES 64

#define MAX_THREADS 20
#define MAX_PG_PER_THREAD 50
#define WORKING_SET_LIMIT 4

#define PAGE_BITS 5 
#define OFFSET_BITS (32 - PAGE_BITS)

enum STATUSES { 
  New = 0, 
  Ready, 
  Running, 
  Interrupted, 
  Ended 
};

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
} table_item_t;

typedef struct {
  int pid;
  table_item_t* entries;
} table_t;

void setup_main_memory(void);
process_t empty_process(void);
page_id_t* empty_wsl(void);
void setup_process_list(void);
void setup_process_tables();
table_t empty_table();
table_item_t* empty_entries();
table_item_t empty_table_item();
frame_t empty_frame(void);
void spawn_new_process(void);
bool is_empty_process(int);
void run_processes(int);
process_t generate_random_process(void);
int generate_random_address();
int get_page_number_from_address(int);
int get_offset_from_address(int);
bool is_page_on_memory(int,int);
void access_page(int, int , int);
