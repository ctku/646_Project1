
/*
 * 
 * pipeline.h
 * 
 * Donald Yeung
 */

#define TRUE 1
#define FALSE 0

#define TO_LT_ENDIAN_4BYTE_IDX(x)			((x%4==0)?(x+3):((x%4==1)?(x+1):(x%4==2)?(x-1):(x-3)))
#define GET_LT_ENDIAN_4BYTE_DATA(mem,x)		((mem[x]<<24) + (mem[x+1]<<16) + (mem[x+2]<<8) + (mem[x+3]))

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
  int halt;
} state_t;

extern state_t *state_create(int *, FILE *, FILE *);

extern void writeback(state_t *, int *);
extern int execute(state_t *);
extern int decode(state_t *);
extern void fetch(state_t *);
