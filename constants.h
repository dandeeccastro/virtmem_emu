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
  int status;
  int working_set;
  int *ptable;
} process_t;

typedef struct {
  int pid;
  int page;
  int last_accessed;
} frame_t;

void setup_main_memory(void);
process_t empty_process(void);
void setup_process_list(void);
void setup_process_tables();
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
bool memory_is_full();
void lru(int, int, int);
void update_frame(frame_t*, int, int, int);
void limited_lru(int, int, int);
void allocate_page(int, int, int);
void print_frames();
void setup_windows();
void print_ptable(process_t);
int* empty_ptable ();
void print_processes();
bool is_empty_frame(frame_t);
void update_ptable(int, int, int, int);
int find_process(int);
void update_log();
