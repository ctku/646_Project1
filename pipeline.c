
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

	int instr = state->if_id.instr;
	unsigned long pc = state->pc;
	const op_info_t *op_info;
	int use_imm = 0;
	operand_t operand1, operand2, result;
	unsigned int r1 = 0, r2 = 0, r3 = 0, imm = 0;

	op_info = decode_instr(instr, &use_imm);
	r1 = FIELD_R1(instr);
	r2 = FIELD_R2(instr);
	r3 = FIELD_R3(instr);
	imm = FIELD_IMM(instr);

	if (use_imm) {
		if (op_info->name == NULL)
			printf("0x%.8X",instr);
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				printf("%s R%d R%d #%d",op_info->name,FIELD_R2(instr),FIELD_R1(instr),FIELD_IMM(instr));
				operand1 = *(operand_t *)&(state->rf_int.reg_int[r1]);
				operand2 = *(operand_t *)&(imm);
				perform_operation(instr, pc, operand1, operand2, &result);
				state->rf_int.reg_int[r2] = result.integer;
				break;
			case FU_GROUP_MEM:
				switch(op_info->data_type) {
				case DATA_TYPE_W:
					printf("%s R%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&state->rf_int.reg_int[r1];
					perform_operation(instr, pc, operand1, operand2, &result);
					switch(op_info->operation) {
					case OPERATION_LOAD://LW
						state->rf_int.reg_int[r2].wu = state->mem[result.integer.wu];
						break;
					case OPERATION_STORE://SW
						state->mem[result.integer.wu] = state->rf_int.reg_int[r2].wu;
						break;
					}
					break;
				case DATA_TYPE_F:
					printf("%s F%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&state->rf_int.reg_int[r1];
					perform_operation(instr, pc, operand1, operand2, &result);
					switch(op_info->operation) {
					case OPERATION_LOAD://L.S
						state->rf_fp.reg_fp[r2] = state->mem[result.integer.wu];
						break;
					case OPERATION_STORE://S.S
						state->mem[result.integer.wu] = state->rf_fp.reg_fp[r2];
						break;
					}
					break;
				}
				break;
			case FU_GROUP_BRANCH:
				switch(op_info->operation) {
				case OPERATION_JAL:
				case OPERATION_J:
					printf("%s #%d",op_info->name,FIELD_OFFSET(instr));
					break;
				case OPERATION_JALR:
				case OPERATION_JR:
					printf("%s R%d",op_info->name,FIELD_R1(instr));
					break;
				case OPERATION_BEQZ:
				case OPERATION_BNEZ:
					printf("%s R%d #%d",op_info->name,FIELD_R1(instr),FIELD_IMM(instr));
					break;
				}
				break;
			default:
				printf("%s",op_info->name);
			}
		}
	} else {
		if(op_info->name == NULL)
			printf("0x%.8X",instr);
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				printf("%s R%d R%d R%d",op_info->name,FIELD_R3(instr),FIELD_R1(instr),FIELD_R2(instr));
				operand1 = *(operand_t *)&state->rf_int.reg_int[r1];
				operand2 = *(operand_t *)&state->rf_int.reg_int[r2];
				perform_operation(instr, pc, operand1, operand2, &result);
				state->rf_int.reg_int[r3] = result.integer;
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				printf("%s F%d F%d F%d",op_info->name,FIELD_R3(instr),FIELD_R1(instr),FIELD_R2(instr));
				operand1 = *(operand_t *)&state->rf_fp.reg_fp[r1];
				operand2 = *(operand_t *)&state->rf_fp.reg_fp[r2];
				perform_operation(instr, pc, operand1, operand2, &result);
				state->rf_fp.reg_fp[r3] = result.flt;
				break;
			default:
				printf("%s",op_info->name);
			}
		}
	}
}


int
decode(state_t *state) {

	// decode to get op_info
	const op_info_t *op_info;
	int use_imm;
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
	// TBD

	// issue instruction
	// TBD
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
