
/*
 * 
 * pipeline.h
 * 
 * Donald Yeung
 */

//#define DEBUG
#ifdef DEBUG
#define dprintf(fmt,...) fprintf(stdout,fmt,__VA_ARGS__)
#else
#define dprintf(fmt,...)
#endif

#define ENDIAN	LITTLE_ENDIAN
#define BIG_ENDIAN	1
#define LITTLE_ENDIAN	2

#define CTRL_HAZARD 4
#define DATA_HAZARD 3
#define HALT 2
#define TRUE 1
#define FALSE 0



/* fetch/decode pipeline register */
typedef struct _if_id_t {
  int instr;
  unsigned long pc;
} if_id_t;


/* Register state */
typedef struct _rf_int_t {
  int_t reg_int[NUMREGS];
} rf_int_t;

typedef struct _rf_fp_t {
  float reg_fp[NUMREGS];
} rf_fp_t;


/* Overall processor state */
typedef struct _state_t {
  /* memory */
  unsigned char mem[MAXMEMORY];

  /* register files */
  rf_int_t rf_int;
  rf_fp_t rf_fp;

  /* pipeline registers */
  unsigned long pc;
  if_id_t if_id;

  fu_int_t *fu_int_list;
  fu_fp_t *fu_add_list;
  fu_fp_t *fu_mult_list;
  fu_fp_t *fu_div_list;

  wb_t int_wb;
  wb_t fp_wb;

  int fetch_lock;
  int branch_taken;
} state_t;

extern state_t *state_create(int *, FILE *, FILE *);

extern void writeback(state_t *, int *);
extern int execute(state_t *);
extern int decode(state_t *);
extern void fetch(state_t *);
