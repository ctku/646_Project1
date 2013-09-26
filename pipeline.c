
/*
 * 
 * pipeline.c
 * 
 * Donald Yeung
 */


#include <stdlib.h>
#include "fu.h"
#include "pipeline.h"


/* some utility dunction */
void store_4bytes(unsigned char *mem, int idx, int data)
{
	if (ENDIAN == LITTLE_ENDIAN) {
		mem[idx] = (data & 0xFF000000) >> 24; 
		mem[idx+1] = (data & 0xFF0000) >> 16;
		mem[idx+2] = (data & 0xFF00) >> 8;
		mem[idx+3] = (data & 0xFF);
	} else {
		mem[idx] = (data & 0xFF);
		mem[idx+1] = (data & 0xFF00) >> 8;
		mem[idx+2] = (data & 0xFF0000) >> 16;
		mem[idx+3] = (data & 0xFF000000) >> 24;
	}
}
int load_4bytes(unsigned char *mem, int idx)
{
	int data;
	char *ptr = (char *)&data;

	if (ENDIAN == LITTLE_ENDIAN) {
		*ptr = mem[idx+3];
		*(ptr+1) = mem[idx+2];
		*(ptr+2) = mem[idx+1];
		*(ptr+3) = mem[idx];
	} else {
		*ptr = mem[idx];
		*(ptr+1) = mem[idx+1];
		*(ptr+2) = mem[idx+2];
		*(ptr+3) = mem[idx+3];
	}
	return data;
}

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
					//printf("%s #%d",op_info->name,FIELD_OFFSET(instr));
					break;
				case OPERATION_JALR:
				case OPERATION_JR:
					printf("%s R%d",op_info->name,FIELD_R1(instr));
					break;
				case OPERATION_BEQZ:
				case OPERATION_BNEZ:
					;//printf("%s R%d #%d",op_info->name,FIELD_R1(instr),FIELD_IMM(instr));
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
execute_instruction(int instr, rf_int_t *rf_int, rf_fp_t *rf_fp, unsigned char *mem, unsigned long *pc, int *branch_taken)
{
	const op_info_t *op_info;
	int use_imm = 0, val = 0;
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
				perform_operation(instr, pc, operand1, operand2, &result);
				rf_int->reg_int[r2] = result.integer;
				break;
			case FU_GROUP_MEM:
				switch(op_info->data_type) {
				case DATA_TYPE_W:
					//printf("%s R%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&rf_int->reg_int[r1];
					perform_operation(instr, pc, operand1, operand2, &result);
					i = result.integer.wu;
					switch(op_info->operation) {
					case OPERATION_LOAD://LW
						rf_int->reg_int[r2].wu = load_4bytes(mem, i);
						break;
					case OPERATION_STORE://SW
						store_4bytes(mem, result.integer.wu, rf_int->reg_int[r2].wu);
						break;
					}
					break;
				case DATA_TYPE_F:
					//printf("%s F%d (%d)R%d",op_info->name,FIELD_R2(instr),FIELD_IMM(instr),FIELD_R1(instr));
					operand1 = *(operand_t *)&(imm);
					operand2 = *(operand_t *)&rf_int->reg_int[r1];
					perform_operation(instr, pc, operand1, operand2, &result);
					i = result.integer.wu;
					switch(op_info->operation) {
					case OPERATION_LOAD://L.S
						val = load_4bytes(mem, i);
						rf_fp->reg_fp[r2] = *(float *)(&val);
						break;
					case OPERATION_STORE://S.S
						store_4bytes(mem, result.integer.wu, *(int *)&rf_fp->reg_fp[r2]);
						break;
					}
					break;
				}
				break;
			case FU_GROUP_BRANCH:
				switch(op_info->operation) {
				case OPERATION_JAL:
				case OPERATION_J:
					*pc = *pc + FIELD_OFFSET(instr) - 4; // -4 is to compensate +4 in next fetch
					*branch_taken = 1;
					//printf("%s #%d",op_info->name,FIELD_OFFSET(instr));
					break;
				case OPERATION_JALR:
				case OPERATION_JR:
					//printf("%s R%d",op_info->name,FIELD_R1(instr));
					break;
				case OPERATION_BEQZ:
					if (rf_int->reg_int[r1].w==0) {
						*pc = *pc + FIELD_IMM(instr) - 4;
						*branch_taken = 1;
					} else {
						*pc = *pc;
						*branch_taken = 0;
					}
					break;
				case OPERATION_BNEZ:
					if (rf_int->reg_int[r1].w!=0) {
						*pc = *pc + FIELD_IMM(instr) - 4;
						*branch_taken = 1;
					} else {
						*pc = *pc;
						*branch_taken = 0;
					}
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
				perform_operation(instr, pc, operand1, operand2, &result);
				rf_int->reg_int[r3] = result.integer;
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				//printf("%s F%d F%d F%d",op_info->name,FIELD_R3(instr),FIELD_R1(instr),FIELD_R2(instr));
				operand1 = *(operand_t *)&rf_fp->reg_fp[r1];
				operand2 = *(operand_t *)&rf_fp->reg_fp[r2];
				perform_operation(instr, pc, operand1, operand2, &result);
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
	int cur_r1, cur_r2, cur_rd, pre_rd, i;

	cur_r1 = FIELD_R1(instr);
	cur_r2 = FIELD_R2(instr);
	cur_rd = get_dest_reg_idx(instr);
	op_info = decode_instr(instr, &use_imm);

	// check INT FU
	fu_int = state->fu_int_list;
	while ((fu_int != NULL) && (!data_hazard)) {
		stage_int = fu_int->stage_list;
		while ((stage_int != NULL) && (!data_hazard)) {
			if (stage_int->current_cycle != -1) {
				pre_rd = get_dest_reg_idx(stage_int->instr);
				// check for RAW
				if ((cur_r1 == pre_rd) || (cur_r2 == pre_rd))
					return TRUE;
				// check for WAW
				/*
				if (op_info->fu_group_num != FU_GROUP_BRANCH)
					if ((cur_rd == pre_rd) && 
						(fu_int_cycles(fu_int) < (stage_int->num_cycles - stage_int->current_cycle)))
						return TRUE;
				*/
			}
			stage_int = stage_int->prev;
		}
		fu_int = fu_int->next;
	}

	// check ADD/MULT/DIV FU
	for (i=0; i<3; i++) {
		if (i==0) fu_fp = state->fu_add_list;
		if (i==1) fu_fp = state->fu_mult_list;
		if (i==2) fu_fp = state->fu_div_list;
		while ((fu_fp != NULL) && (!data_hazard)) {
			stage_fp = fu_fp->stage_list;
			while ((stage_fp != NULL) && (!data_hazard)) {
				if (stage_fp->current_cycle != -1) {
					pre_rd = get_dest_reg_idx(stage_fp->instr);
					// check for RAW
					if ((cur_r1 == pre_rd) || (cur_r2 == pre_rd))
						return TRUE;
					// check for WAW
					/*
					if (op_info->fu_group_num != FU_GROUP_BRANCH)
						if ((cur_rd == pre_rd) && 
							(fu_fp_cycles(fu_fp) < (stage_fp->num_cycles - stage_fp->current_cycle)))
							return TRUE;
					*/
				}
				stage_fp = stage_fp->prev;
			}
			fu_fp = fu_fp->next;
		}
	}

	return FALSE;
}


void
writeback(state_t *state, int *num_insn) {
	const op_info_t *op_info;
	int use_imm, exec = 0;

	// increase num_insn
	if (state->int_wb.instr!=0)
		*num_insn += 1;
	if (state->fp_wb.instr!=0)
		*num_insn += 1;

	// execute instruction & writeback
	state->branch_taken = 0;
	execute_instruction(state->int_wb.instr, &state->rf_int, &state->rf_fp, state->mem, &state->pc, &state->branch_taken);
	execute_instruction(state->fp_wb.instr, &state->rf_int, &state->rf_fp, state->mem, &state->pc, &state->branch_taken);

	// check if control hazard can be resolved
	op_info = decode_instr(state->int_wb.instr, &use_imm);
	if ((op_info->fu_group_num == FU_GROUP_BRANCH) && (state->fetch_lock == CTRL_HAZARD)) {
		state->fetch_lock = FALSE;
		dprintf("   [W]:Control Hazard Resolved => fetch_lock = FALSE\n");
	}
	
	// check if HALT can be resolved (only branch_taken can resolve it)
	if ((op_info->fu_group_num == FU_GROUP_BRANCH) && (state->branch_taken)) {
		state->fetch_lock = FALSE;
		dprintf("   [W]:Halt Resolved => fetch_lock = FALSE\n");
	}


}


int
execute(state_t *state) {
	const op_info_t *op_info;
	int use_imm, data_hazard;

	// get data_hazard & control_hazard
	op_info = decode_instr(state->if_id.instr, &use_imm);
	data_hazard = check_data_hazard(state, state->if_id.instr);

	// stall if data hazard happen
	if (data_hazard) {
		state->fetch_lock = DATA_HAZARD;
		dprintf("   [DE]:Data Hazard => fetch_lock = DATA_HAZARD\n");
	}

	// Above: Decode stage
	// ============================================================
	// Below: Execute stage

	// check if HALT is found and all job finished
	if (fu_int_done(state->fu_int_list) &&
		fu_fp_done(state->fu_add_list) &&
		fu_fp_done(state->fu_mult_list) &&
		fu_fp_done(state->fu_div_list) &&
		(!state->int_wb.instr) && 
		(!state->fp_wb.instr) &&
		state->fetch_lock == HALT)
		return 1;

	// check if data hazard resolved
	if ((state->fetch_lock == DATA_HAZARD) && !data_hazard) {
		state->fetch_lock = FALSE;
		dprintf("   [E]:Data Hazard Resolved => fetch_lock = FALSE\n");
	}

	// advance function unit
	state->int_wb.instr = 0;
	state->fp_wb.instr = 0;
	advance_fu_int(state->fu_int_list, &state->int_wb);
	advance_fu_fp(state->fu_add_list, &state->fp_wb);
	advance_fu_fp(state->fu_mult_list, &state->fp_wb);
	advance_fu_fp(state->fu_div_list, &state->fp_wb);

	return 0;
}


int
decode(state_t *state) {
	const op_info_t *op_info;
	int use_imm, issue_ret = 0;
	int instr = state->if_id.instr;
	int control_hazard = 0, structural_hazard = 0;

	// check if HALT is found, HALT take effect only when branch is not taken in writeback
	op_info = decode_instr(instr, &use_imm);
	if (op_info->fu_group_num == FU_GROUP_HALT) {
		if (!state->branch_taken) {
			state->fetch_lock = HALT;
			dprintf("   [D]:Halt found => fetch_lock = HALT\n");
		}
	}

	// issue instruction
	if ((state->fetch_lock != HALT) && (!state->branch_taken)) {
		op_info = decode_instr(instr, &use_imm);
		switch (op_info->fu_group_num) {
		case FU_GROUP_INT:
		case FU_GROUP_MEM:
		case FU_GROUP_BRANCH:
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
		if (issue_ret == -1) {
			structural_hazard = 1;
		}
	}
	
	// check if control hazard happen
	control_hazard = (op_info->fu_group_num == FU_GROUP_BRANCH);
	if (control_hazard) {
		state->fetch_lock = CTRL_HAZARD;
		dprintf("   [D]:Control Hazard => fetch_lock = CTRL_HAZARD\n");
	}

}


void
fetch(state_t *state) {
	const op_info_t *op_info;
	int use_imm;
	int pc = state->pc;
	int instr;

	// read instructions
	instr = load_4bytes(state->mem, pc);

	// update pc & instruction
	state->pc = pc + 4;
	state->if_id.instr = instr;
}
