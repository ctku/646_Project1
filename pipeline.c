
/*
 * 
 * pipeline.c
 * 
 * Donald Yeung
 */


#include <stdlib.h>
#include "fu.h"
#include "pipeline.h"

/* get destination register idx */
int
get_dest_reg_idx(int instr)
{
	const op_info_t *op_info;
	int use_imm = 0;
	int idx = -1;

	op_info = decode_instr(instr, &use_imm);
	if (use_imm) {
		if (op_info->name == NULL)
			;
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				idx = FIELD_R2(instr);
				break;
			case FU_GROUP_MEM:
				switch(op_info->data_type) {
				case DATA_TYPE_W:
					switch(op_info->operation) {
					case OPERATION_LOAD://LW
						idx = FIELD_R2(instr);
						break;
					case OPERATION_STORE://SW
						break;
					}
					break;
				case DATA_TYPE_F:
					switch(op_info->operation) {
					case OPERATION_LOAD://L.S
						idx = FIELD_R2(instr);
						break;
					case OPERATION_STORE://S.S
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
			}
		}
	} else {
		if(op_info->name == NULL)
			;
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				idx = FIELD_R3(instr);
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				idx = FIELD_R3(instr);
				break;
			}
		}
	}

}


/* execute an instruction */
void
execute_instruction(int instr, rf_int_t *rf_int, rf_fp_t *rf_fp, unsigned char *mem)
{
	const op_info_t *op_info;
	int use_imm = 0;
	operand_t operand1, operand2, result;
	unsigned int r1 = 0, r2 = 0, r3 = 0, imm = 0, i;

	op_info = decode_instr(instr, &use_imm);
	r1 = FIELD_R1(instr);
	r2 = FIELD_R2(instr);
	r3 = FIELD_R3(instr);
	imm = FIELD_IMM(instr);

	// perform operation
	if (use_imm) {
		if (op_info->name == NULL)
			;//printf("0x%.8X",instr);
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				//printf("%s R%d R%d #%d",op_info->name,FIELD_R2(instr),FIELD_R1(instr),FIELD_IMM(instr));
				operand1 = *(operand_t *)&(rf_int->reg_int[r1]);
				operand2 = *(operand_t *)&(imm);
				perform_operation(instr, operand1, operand2, &result);
				rf_int->reg_int[r2] = result.integer;
				break;
			case FU_GROUP_MEM:
				switch(op_info->data_type) {
				case DATA_TYPE_W:
					//printf("%s R%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&rf_int->reg_int[r1];
					perform_operation(instr, operand1, operand2, &result);
					i = result.integer.wu;
					switch(op_info->operation) {
					case OPERATION_LOAD://LW
						rf_int->reg_int[r2].wu = (mem[i]<<24) + (mem[i+1]<<16) + (mem[i+2]<<8) + (mem[i+3]);
						break;
					case OPERATION_STORE://SW
						mem[result.integer.wu] = rf_int->reg_int[r2].wu;
						break;
					}
					break;
				case DATA_TYPE_F:
					//printf("%s F%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&rf_int->reg_int[r1];
					perform_operation(instr, operand1, operand2, &result);
					i = result.integer.wu;
					switch(op_info->operation) {
					case OPERATION_LOAD://L.S
						rf_fp->reg_fp[r2] = (mem[i]<<24) + (mem[i+1]<<16) + (mem[i+2]<<8) + (mem[i+3]);
						break;
					case OPERATION_STORE://S.S
						mem[result.integer.wu] = rf_fp->reg_fp[r2];
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
				;//printf("%s",op_info->name);
			}
		}
	} else {
		if(op_info->name == NULL)
			;//printf("0x%.8X",instr);
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				//printf("%s R%d R%d R%d",op_info->name,FIELD_R3(instr),FIELD_R1(instr),FIELD_R2(instr));
				operand1 = *(operand_t *)&rf_int->reg_int[r1];
				operand2 = *(operand_t *)&rf_int->reg_int[r2];
				perform_operation(instr, operand1, operand2, &result);
				rf_int->reg_int[r3] = result.integer;
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				//printf("%s F%d F%d F%d",op_info->name,FIELD_R3(instr),FIELD_R1(instr),FIELD_R2(instr));
				operand1 = *(operand_t *)&rf_fp->reg_fp[r1];
				operand2 = *(operand_t *)&rf_fp->reg_fp[r2];
				perform_operation(instr, operand1, operand2, &result);
				rf_fp->reg_fp[r3] = result.flt;
				break;
			default:
				;//printf("%s",op_info->name);
			}
		}
	}

}

int
check_data_hazard(state_t *state, int instr) {

	// RAW:
	// scan unfinished fu
	//    check if (instr(R1) or instr(R2) == unfinished_instr(R3) in fu)
	// WAW:
	// scan unfinished fu
	//    check if (instr(RD) == unfinished_instr(RD) in fu)
	//          && (instr.fu_cycle < unifinished_instr.remain_cycle)
	//          && (no structural hazrd for instr)

	// decode to get op_info
	const op_info_t *op_info;
	int use_imm;
	fu_int_t *fu_int;
	fu_fp_t *fu_fp;
	fu_int_stage_t *stage_int;
	fu_fp_stage_t *stage_fp;
	int data_hazard = 0;

	op_info = decode_instr(instr, &use_imm);
    switch (op_info->fu_group_num) {
	case FU_GROUP_INT:
	case FU_GROUP_MEM:
	    fu_int = state->fu_int_list;
		while ((fu_int != NULL) && (!data_hazard)) {
			stage_int = fu_int->stage_list;
			while ((stage_int != NULL) && (!data_hazard)) {
				if (stage_int->current_cycle != -1) {
					int cur_r1 = FIELD_R1(instr);
					int cur_r2 = FIELD_R2(instr);
					int cur_rd = get_dest_reg_idx(instr);
					int pre_rd = get_dest_reg_idx(stage_int->instr);
					// check for RAW
					if ((cur_r1 == pre_rd) || (cur_r2 == pre_rd))
						return TRUE;
					// check for WAW
					if ((cur_rd == pre_rd) && 
						(fu_int_cycles(fu_int) < (stage_int->num_cycles - stage_int->current_cycle)))
						return TRUE;
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
		while ((fu_fp != NULL) && (!data_hazard)) {
			stage_fp = fu_fp->stage_list;
			while ((stage_fp != NULL) && (!data_hazard)) {
				if (stage_fp->current_cycle != -1) {
					// check for RAW
					int cur_r1 = FIELD_R1(instr);
					int cur_r2 = FIELD_R2(instr);
					int cur_rd = get_dest_reg_idx(instr);
					int pre_rd = get_dest_reg_idx(stage_fp->instr);
					if ((cur_r1 == pre_rd) || (cur_r2 == pre_rd))
						return TRUE;
					// check for WAW
					if ((cur_rd == pre_rd) && 
						(fu_fp_cycles(fu_fp) < (stage_int->num_cycles - stage_int->current_cycle)))
						return TRUE;
				}
				stage_fp = stage_fp->prev;
			}
			fu_fp = fu_fp->next;
	    }
	default:
		break;
	}

	return FALSE;
}


void
writeback(state_t *state, int *num_insn) {

	// execute instruction & writeback
	execute_instruction(state->int_wb.instr, &state->rf_int, &state->rf_fp, state->mem);
	execute_instruction(state->fp_wb.instr, &state->rf_int, &state->rf_fp, state->mem);

	// clear instruction
	state->int_wb.instr = 0;
	state->fp_wb.instr = 0;
}


void
execute(state_t *state) {

	const op_info_t *op_info;
	int use_imm = 0;
	op_info = decode_instr(state->if_id.instr, &use_imm);

	// check if data hazard resolved
	if (check_data_hazard(state, state->if_id.instr))
		state->fetch_lock = TRUE;
	else
		state->fetch_lock = FALSE;

	// advance function unit
	switch(op_info->fu_group_num) {
	case FU_GROUP_INT:
	case FU_GROUP_MEM:
		advance_fu_int(state->fu_int_list, &state->int_wb);
		break;
	case FU_GROUP_ADD:
		advance_fu_fp(state->fu_add_list, &state->fp_wb);
		break;
	case FU_GROUP_MULT:
		advance_fu_fp(state->fu_mult_list, &state->fp_wb);
		break;
	case FU_GROUP_DIV:
		advance_fu_fp(state->fu_div_list, &state->fp_wb);
		break;
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
	int issue_ret = 0;
	int structural_hazard = 0;

	op_info = decode_instr(instr, &use_imm);

#if 0
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
#endif



    // check control hazard
	// TBD

	// issue instruction
	switch (op_info->fu_group_num) {
	case FU_GROUP_INT:
	case FU_GROUP_MEM:
		issue_ret = issue_fu_int(state->fu_int_list, instr);
		break;
	case FU_GROUP_ADD:
		issue_ret = issue_fu_fp(state->fu_add_list, instr);
		break;
	case FU_GROUP_MULT:
		issue_ret = issue_fu_fp(state->fu_mult_list, instr);
		break;
	case FU_GROUP_DIV:
		issue_ret = issue_fu_fp(state->fu_div_list, instr);
		break;
	}
	if (issue_ret == -1)
		structural_hazard = 1;


}


void
fetch(state_t *state) {

	// update pc & instruction
	int pc = state->pc;
	int instr = (state->mem[pc]<<24) + (state->mem[pc+1]<<16) + (state->mem[pc+2]<<8) + (state->mem[pc+3]);
	state->if_id.pc = pc;
	state->if_id.instr = instr;

	// advane pc by 4
	state->pc += 4;

	// stall if data hazard happen
	if (check_data_hazard(state, state->if_id.instr))
		state->fetch_lock = TRUE;
}
