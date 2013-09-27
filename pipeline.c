
/*
 * 
 * pipeline.c
 * 
 * Donald Yeung
 */


#include <stdlib.h>
#include "fu.h"
#include "pipeline.h"
#include <string.h>


/* some utility dunction */
void store_4bytes(unsigned char *mem, int idx, int data)
{
	mem[idx] = (data & 0xFF000000) >> 24; 
	mem[idx+1] = (data & 0xFF0000) >> 16;
	mem[idx+2] = (data & 0xFF00) >> 8;
	mem[idx+3] = (data & 0xFF);

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

void
get_operands(int instr, rf_int_t *rf_int, rf_fp_t *rf_fp, operand_t *op1, operand_t *op2)
{
	const op_info_t *op_info;
	int use_imm = 0;
	unsigned int r1 = 0, r2 = 0, r3 = 0, imm = 0;

	op_info = decode_instr(instr, &use_imm);
	r1 = FIELD_R1(instr);
	r2 = FIELD_R2(instr);
	imm = FIELD_IMM(instr);

	// perform operation
	if (use_imm) {
		if (op_info->name == NULL)
			;
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				*op1 = *(operand_t *)&rf_int->reg_int[r1];
				*op2 = *(operand_t *)&(imm);
				break;
			case FU_GROUP_MEM:
				*op1 = *(operand_t *)&rf_int->reg_int[r1];
				*op2 = *(operand_t *)&(imm);
				break;
			case FU_GROUP_BRANCH:
				switch(op_info->operation) {
				case OPERATION_J:
					break;
				case OPERATION_JR:
					*op1 = *(operand_t *)&rf_int->reg_int[r1];
				case OPERATION_JAL:
					break;
				case OPERATION_JALR:
					*op1 = *(operand_t *)&rf_int->reg_int[r1];
					break;
				case OPERATION_BEQZ:
					*op1 = *(operand_t *)&rf_int->reg_int[r1];
					break;
				case OPERATION_BNEZ:
					*op1 = *(operand_t *)&rf_int->reg_int[r1];
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
				*op1 = *(operand_t *)&rf_int->reg_int[r1];
				*op2 = *(operand_t *)&rf_int->reg_int[r2];
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				*op1 = *(operand_t *)&rf_fp->reg_fp[r1];
				*op2 = *(operand_t *)&rf_fp->reg_fp[r2];
				break;
			}
		}
	}
}


/* execute an instruction */
void
execute_instruction(int instr, operand_t op1, operand_t op2, rf_int_t *rf_int, rf_fp_t *rf_fp, unsigned char *mem, unsigned long *pc, int *branch)
{
	const op_info_t *op_info;
	int use_imm = 0, val = 0;
	operand_t result;
	unsigned int r1 = 0, r2 = 0, r3 = 0, imm = 0, i;

	op_info = decode_instr(instr, &use_imm);
	r1 = FIELD_R1(instr);
	r2 = FIELD_R2(instr);
	r3 = FIELD_R3(instr);
	imm = FIELD_IMM(instr);

	// perform operation
	if (use_imm) {
		if (op_info->name == NULL)
			;
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				perform_operation(instr, pc, op1, op2, &result);
				rf_int->reg_int[r2] = result.integer;
				break;
			case FU_GROUP_MEM:
				switch(op_info->data_type) {
				case DATA_TYPE_W:
					perform_operation(instr, pc, op1, op2, &result);
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
					perform_operation(instr, pc, op1, op2, &result);
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
				// -4 is to compensate +4 in previous fetch, because we always advance pc by 4 in fetch
				//*pc = *pc - 4;
				switch(op_info->operation) {
				case OPERATION_J:
					*pc = *pc + FIELD_OFFSET(instr) + 4;
					*branch = TAKEN;
					break;
				case OPERATION_JR:
					*pc = op1.integer.wu;
					*branch = TAKEN;
					break;
				case OPERATION_JAL:
					rf_int->reg_int[31].wu = *pc;
					*pc = *pc + FIELD_OFFSET(instr) + 4;
					*branch = TAKEN;
					break;
				case OPERATION_JALR:
					rf_int->reg_int[31].wu = *pc;
					*pc = op1.integer.wu;
					*branch = TAKEN;
					break;
				case OPERATION_BEQZ:
					if (op1.integer.w == 0) {
						*pc = *pc + FIELD_IMM(instr) + 4;
						*branch = TAKEN;
					} else {
						*pc = *pc + 4;                       
						*branch = NOT_TAKEN;
					}
					break;
				case OPERATION_BNEZ:
					if (op1.integer.w != 0) {
						*pc = *pc + FIELD_IMM(instr) + 4;
						*branch = TAKEN;
					} else {
						*pc = *pc + 4;
						*branch = NOT_TAKEN;
					}
					break;
				}
				// if TAKEN, it implies there was a controld hazard and resolved right now, ex 
				// (n-1):W -> E -> D -> F(pc+4)
				// (n  ):W -> E    x    x
				// ...
				// (n+k):W*-> E -> D -> F**
				// where W* create shift_pc for F**, it can be seen that at (n-1) pc is advanced by 4 wrongly,
				// so in order to create coorect shift_pc for F**, we need to subtract 4 here for TAKEN case
				if (*branch == TAKEN)
					*pc = *pc - 4;

				// (n+k):W*-> E -> D -> F**(1)(2)(3)
				// In F**, we will 
				// (1) advaced pc by pc_shift created from here
				// (2) then fetch instruction
				// (3) then advance pc by 4 for next cycle
				// because the above result (new pc) is already advanced by 4 which is used for next cycle in (3) 
				// and what (2) need (fetch in F**) is the pc before advance 4, so 4 should be subtracted from pc here
				*pc = *pc - 4; 
				break;
			}
		}
	} else {
		if(op_info->name == NULL)
			;//printf("0x%.8X",instr);
		else {
			switch(op_info->fu_group_num) {
			case FU_GROUP_INT:
				perform_operation(instr, pc, op1, op2, &result);
				rf_int->reg_int[r3] = result.integer;
				break;
			case FU_GROUP_ADD:
			case FU_GROUP_MULT:
			case FU_GROUP_DIV:
				perform_operation(instr, pc, op1, op2, &result);
				rf_fp->reg_fp[r3] = result.flt;
				break;
			}
		}
	}
}

int
check_data_hazard(state_t *state) {

	// Table: check RAW hazard
	//
	//   \(cur)|   INT    | ADD  MULT DIV  |     MEM     |          BRANCH
	//    \    |n-imm imm |                | lw sw ls ss | j jr jal jalr beqz bnez
	//(pre)\   |  R1   R1 |  F1   F1   F1  | R1 R1 R1 R1 | - R1  -   R1   R1   R1
	//      \  |  R2   -  |  F2   F2   F2  | -  R2 -  F2 | -  -  -   -    -    -
	// ------\-+----------+----------------+-------------+-------------------------
	//  INT(ls)|  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    INT  |  RD   RD |  -    -    -   | RD RD RD RD | - RD  -   RD   RD   RD
	//    ADD  |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    MULT |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    DIV  |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -

	// Table: check WAW hazard
	//
	//   \(cur)| ADD MULT  DIV |     MEM     
	//    \    |               | lw sw ls ss
	//(pre)\   |  RD   RD   RD | -  -  RD - 
	// -----\--+---------------+------------
	//    ADD  |  RD   RD   RD | -  -  RD - 
	//    MULT |  RD   RD   RD | -  -  RD - 
	//    DIV  |  RD   RD   RD | -  -  RD - 

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
	int done = 0, data_hazard = 0, check_FP = FALSE, i;
	int cur_r1 = -1, cur_r2 = -2, cur_rd = -3, pre_rd = -4;
	int cur_f1 = -5, cur_f2 = -6, cur_fd = -7, pre_fd = -8;
	int instr = state->if_id.instr;

	op_info = decode_instr(instr, &use_imm);
	switch (op_info->fu_group_num) {
	case FU_GROUP_INT:
		cur_r1 = FIELD_R1(instr);
		if (!use_imm)
			cur_r2 = FIELD_R2(instr);
		break;
	case FU_GROUP_ADD:
	case FU_GROUP_MULT:
	case FU_GROUP_DIV:
		cur_f1 = FIELD_R1(instr);
		cur_f2 = FIELD_R2(instr);
		check_FP = TRUE;
		break;
	case FU_GROUP_MEM:
		cur_r1 = FIELD_R1(instr);
		if (strcmp(op_info->name, "SW") == 0)
			cur_r2 = FIELD_R2(instr);
		if (strcmp(op_info->name, "S.S") == 0) {
			cur_f2 = FIELD_R2(instr);
			check_FP = TRUE;
		}
		break;
	case FU_GROUP_BRANCH:
		if ((op_info->operation == OPERATION_JR) ||
			(op_info->operation == OPERATION_JALR) ||
			(op_info->operation == OPERATION_BEQZ) ||
			(op_info->operation == OPERATION_BNEZ))
			cur_r1 = FIELD_R1(instr);
		else
			done = 1;
		break;
	default:
		done = 1;
	}

	// no need to handle
	if (done == 1)
		return FALSE;

	// everyone need to check INT FU
	fu_int = state->fu_int_list;
	while ((fu_int != NULL) && (!data_hazard)) {
		stage_int = fu_int->stage_list;
		while ((stage_int != NULL) && (!data_hazard)) {
			if (stage_int->current_cycle != -1) {
				const op_info_t *pre_op_info;
				int use_imm = 0;
				pre_op_info = decode_instr(stage_int->instr, &use_imm);
				pre_rd = get_dest_reg_idx(stage_int->instr);
				// check for RAW
				if (strcmp(pre_op_info->name, "L.S") == 0) {
					if ((cur_f1 == pre_rd) || (cur_f2 == pre_rd))
						return TRUE;
				} else {
					if ((cur_r1 == pre_rd) || (cur_r2 == pre_rd))
						return TRUE;
				}
			}
			stage_int = stage_int->prev;
		}
		fu_int = fu_int->next;
	}
	
	// check ADD/MULT/DIV FU
	if (check_FP) {
		for (i=0; i<3; i++) {
			if (i==0) fu_fp = state->fu_add_list;
			if (i==1) fu_fp = state->fu_mult_list;
			if (i==2) fu_fp = state->fu_div_list;
			while ((fu_fp != NULL) && (!data_hazard)) {
				stage_fp = fu_fp->stage_list;
				while ((stage_fp != NULL) && (!data_hazard)) {
					if (stage_fp->current_cycle != -1) {
						const op_info_t *pre_op_info;
						int use_imm = 0;
						pre_op_info = decode_instr(stage_fp->instr, &use_imm);
						pre_fd = get_dest_reg_idx(stage_fp->instr);
						// check for RAW
						if ((cur_f1 == pre_fd) || (cur_f2 == pre_fd))
							return TRUE;
					}
					stage_fp = stage_fp->prev;
				}
				fu_fp = fu_fp->next;
			}
		}
	}

	return FALSE;
}


int
check_struct_hazard(state_t *state) {

	// Table: check 2nd type Structural hazard
	//
	//    \(cur)|   INT    | ADD  MULT DIV  |     MEM     |          BRANCH
	//     \    |n-imm imm |                | lw sw ls ss | j jr jal jalr beqz bnez
	//(pre) \   |  RD   RD |  FD   FD   FD  | RD -  FD -  | -  -  RD  RD   -    -
	// ------\--+----------+----------------+-------------+-------------------------
	//  INT(ls) |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    INT   |  RD   RD |  -    -    -   | RD RD RD RD | - RD  -   RD   RD   RD
	//    ADD   |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    MULT  |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -
	//    DIV   |  -    -  |  FD   FD   FD  | -  -  -  FD | -  -  -   -    -    -

	const op_info_t *op_info;
	int use_imm, struct_hazard = 0;
	int expect_cycles = 0;


	op_info = decode_instr(state->if_id.instr, &use_imm);
	switch (op_info->fu_group_num) {
	case FU_GROUP_ADD:
		expect_cycles = fu_fp_all_cycles(state->fu_add_list);
		break;
	case FU_GROUP_MULT:
		expect_cycles = fu_fp_all_cycles(state->fu_mult_list);
		break;
	case FU_GROUP_DIV:
		expect_cycles = fu_fp_all_cycles(state->fu_div_list);
		break;
	case FU_GROUP_INT:
	default:
		expect_cycles = fu_int_all_cycles(state->fu_int_list);
		break;
	}

	switch (op_info->fu_group_num) {
	case FU_GROUP_ADD:
	case FU_GROUP_MULT:
	case FU_GROUP_DIV:
		struct_hazard = fu_fp_match_cycles(state->fu_add_list, expect_cycles);
		struct_hazard |= fu_fp_match_cycles(state->fu_mult_list, expect_cycles);
		struct_hazard |= fu_fp_match_cycles(state->fu_div_list, expect_cycles);
		break;
	case FU_GROUP_INT:
		struct_hazard = fu_int_match_cycles(state->fu_int_list, expect_cycles);
		break;
	case FU_GROUP_MEM:
		if (strcmp(op_info->name, "LW") == 0)
			struct_hazard = fu_int_match_cycles(state->fu_int_list, expect_cycles);
		if (strcmp(op_info->name, "L.S") == 0) {
			struct_hazard = fu_fp_match_cycles(state->fu_add_list, expect_cycles);
			struct_hazard |= fu_fp_match_cycles(state->fu_mult_list, expect_cycles);
			struct_hazard |= fu_fp_match_cycles(state->fu_div_list, expect_cycles);
		}
	}

	return struct_hazard;
}


void
writeback(state_t *state, int *num_insn) {
	const op_info_t *op_info;
	int use_imm, exec = 0;
	unsigned long pc = state->pc;

	// increase num_insn
	if (state->int_wb.instr!=0)
		*num_insn += 1;
	if (state->fp_wb.instr!=0)
		*num_insn += 1;

	// execute instruction & writeback
	state->branch = NONE;
	execute_instruction(state->int_wb.instr, state->int_wb.op1, state->int_wb.op2, &state->rf_int, &state->rf_fp, state->mem, &pc, &state->branch);
	execute_instruction(state->fp_wb.instr, state->fp_wb.op1, state->fp_wb.op2, &state->rf_int, &state->rf_fp, state->mem, &pc, &state->branch);
	state->pc_shift = pc - state->pc;
	dprintf("   [W]:newpc 0x%X  oldpc 0x%X  pc_shift %d\n", pc, state->pc, state->pc_shift);

	// check if control hazard can be resolved
	op_info = decode_instr(state->int_wb.instr, &use_imm);
	if ((op_info->fu_group_num == FU_GROUP_BRANCH) && (state->fetch_lock == CTRL_HAZARD)) {
		state->fetch_lock = FALSE;
		dprintf("   [W]:Control Hazard Resolved => fetch_lock = FALSE\n");
	}
	
	// check if HALT can be resolved (only branch_taken can resolve it)
	if ((op_info->fu_group_num == FU_GROUP_BRANCH) && (state->branch == TAKEN)) {
		state->fetch_lock = FALSE;
		dprintf("   [W]:Halt Resolved => fetch_lock = FALSE\n");
	}
}


int
execute(state_t *state) {
	const op_info_t *op_info;
	int use_imm, data_hazard, struct_hazard;

	// get data_hazard & struct_hazard
	op_info = decode_instr(state->if_id.instr, &use_imm);
	data_hazard = check_data_hazard(state);
	struct_hazard = check_struct_hazard(state);

	// stall if data hazard happen
	if (data_hazard && (state->fetch_lock == FALSE)) {
		state->fetch_lock = DATA_HAZARD;
		//state->pc = state->pc - 4;
		dprintf("   [DE]:Data Hazard => fetch_lock = DATA_HAZARD\n");
	}

	// stall if structural_hazard
	if (struct_hazard && (state->fetch_lock == FALSE)) {
		state->fetch_lock = STRUCT_HAZARD;
		dprintf("   [DE]:Struct Hazard => fetch_lock = STRUCT_HAZARD\n");
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

	// check if structural hazard resolved
	if ((state->fetch_lock == STRUCT_HAZARD) && !struct_hazard) {
		state->fetch_lock = FALSE;
		dprintf("   [E]:Structural Hazard Resolved => fetch_lock = FALSE\n");
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
	operand_t op1, op2;

	// check if HALT is found, HALT take effect only when branch is not taken in writeback
	op_info = decode_instr(instr, &use_imm);
	if (op_info->fu_group_num == FU_GROUP_HALT) {
		if (state->branch != TAKEN) {
			state->fetch_lock = HALT;
			dprintf("   [D]:Halt found => fetch_lock = HALT\n");
		}
	}

	// get current operands
	get_operands(instr, &state->rf_int, &state->rf_fp, &op1, &op2);

	// issue instruction with current operands
	if ((state->fetch_lock != HALT) && (state->branch != TAKEN)) {
		op_info = decode_instr(instr, &use_imm);
		switch (op_info->fu_group_num) {
		case FU_GROUP_INT:
		case FU_GROUP_MEM:
		case FU_GROUP_BRANCH:
			issue_ret = issue_fu_int(state->fu_int_list, instr, op1, op2);
			break;
		case FU_GROUP_ADD:
			issue_ret = issue_fu_fp(state->fu_add_list, instr, op1, op2);
			break;
		case FU_GROUP_MULT:
			issue_ret = issue_fu_fp(state->fu_mult_list, instr, op1, op2);
			break;
		case FU_GROUP_DIV:
			issue_ret = issue_fu_fp(state->fu_div_list, instr, op1, op2);
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
	int pc = state->pc;
	int instr;

	// advance pc for branch TAKEN case
	if (state->branch == TAKEN) {
		dprintf("   [F][pre]:pc=0x%X (before)\n",pc);
		pc += state->pc_shift;
		dprintf("   [F][pre]:pc=0x%X (after)\n",pc);
	}

	// read instructions
	instr = load_4bytes(state->mem, pc);

	// update pc & instruction
	pc += 4;
	state->pc = pc;
	state->if_id.instr = instr;
	dprintf("   [F][pos]:pc+4=0x%X\n",pc);
}
