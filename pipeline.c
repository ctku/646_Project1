
/*
 * 
 * pipeline.c
 * 
 * Donald Yeung
 */


#include <stdlib.h>
#include "fu.h"
#include "pipeline.h"


void
writeback(state_t *state, int *num_insn) {
}


void
execute(state_t *state) {
}


int
decode(state_t *state) {

	// decode to get op_info
	const op_info_t *op_info;
	int use_imm;
	operand_t result;
	fu_int_t *fu_int;
	fu_fp_t *fu_fp;
	fu_int_stage_t *stage_int;
	fu_fp_stage_t *stage_fp;
	int instr = state->if_id.instr;
	int exist_struct = 0;
	int exist_RAW = 0;
    int exist_WAW = 0;
	op_info = decode_instr(instr, &use_imm);

	// check structural hazard
    switch (op_info->fu_group_num) {
	case FU_GROUP_INT:
	    exist_struct &= !fu_int_done(state->fu_int_list);
	    break;
	case FU_GROUP_ADD:
	    exist_struct &= !fu_fp_done(state->fu_add_list);
	    break;
	case FU_GROUP_MULT:
	    exist_struct &= !fu_fp_done(state->fu_mult_list);
	    break;
	case FU_GROUP_DIV:
	    exist_struct &= !fu_fp_done(state->fu_div_list);
	    break;
	default:
		break;
    }

	// check data hazard
	// RAW:
	// scan unfinished fu
	//    check if (instr(R1) or instr(R2) == unfinished_instr(R3) in fu)
	// WAW:
	// scan unfinished fu
	//    check if (instr(RD) == unfinished_instr(RD) in fu)
	//          && (instr.fu_cycle < unifinished_instr.remain_cycle)
	//          && (no structural hazrd for instr)
    switch (op_info->fu_group_num) {
	case FU_GROUP_INT:
	    fu_int = state->fu_int_list;
		while ((fu_int != NULL) && (!exist_RAW) && (!exist_RAW)) {
			stage_int = fu_int->stage_list;
			while ((stage_int != NULL) && (!exist_RAW) && (!exist_RAW)) {
				if (stage_int->current_cycle != -1) {
				// check for RAW
				if ((FIELD_R1(instr) == FIELD_R3(stage_int->instr)) ||
					(FIELD_R2(instr) == FIELD_R3(stage_int->instr)))
					exist_RAW = 1;
				// check for WAW
				if ((FIELD_R3(instr) == FIELD_R3(stage_int->instr)) && 
					!exist_struct &&
					(fu_int_cycles(fu_int) < (stage_int->num_cycles - stage_int->current_cycle)))
					exist_WAW = 1;
				}
				stage_int = stage_int->prev;
			}
			fu_int = fu_int->next;
	    }
		break;
	case FU_GROUP_ADD:
		fu_fp = state->fu_add_list;
	case FU_GROUP_MULT:
		fu_fp = state->fu_mult_list;
	case FU_GROUP_DIV:
	    fu_fp = state->fu_div_list;
		while ((fu_fp != NULL) && (!exist_RAW) && (!exist_RAW)) {
			stage_fp = fu_fp->stage_list;
			while ((stage_fp != NULL) && (!exist_RAW) && (!exist_RAW)) {
				if (stage_fp->current_cycle != -1) {
				// check for RAW
				if ((FIELD_R1(instr) == FIELD_R3(stage_fp->instr)) ||
					(FIELD_R2(instr) == FIELD_R3(stage_fp->instr)))
					exist_RAW = 1;
				// check for WAW
				if ((FIELD_R3(instr) == FIELD_R3(stage_fp->instr)) && 
					!exist_struct &&
					(fu_fp_cycles(fu_fp) < (stage_fp->num_cycles - stage_fp->current_cycle)))
					exist_WAW = 1;
				}
				stage_fp = stage_fp->prev;
			}
			fu_fp = fu_fp->next;
	    }
	default:
		break;
	}

    // check control hazard
}


void
fetch(state_t *state) {
    // update pc & instruction
    unsigned long pc = state->pc;
    state->if_id.pc = pc;
    state->if_id.instr = (state->mem[pc]<<24) + (state->mem[pc+1]<<16) + (state->mem[pc+2]<<8) + (state->mem[pc+3]);
    
    // advane pc by 4
    state->pc += 4;
}
