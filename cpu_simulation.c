#include <stdio.h>
#include <stdbool.h>

int memory[2048];
int R[32];
int pc;
bool IF = false;
bool MEM = false;
int clock;

typedef struct {
    bool wb;
    bool mem;
    int write_read;  // 0: no access, 1: write to memory, 2: read from memory
    int stage;       // 0: IF, 1: ID, 2: EX, 3: MEM, 4: WB
    int temp_reg;
    int temp_address;
    int opcode;
    int r1, r2, r3;
    int shamt;       // for R-type
    int immediate;   // for I-type 
    int address;     // for J-type 
} Pipeline;

void fetch_instruction(Pipeline *pipeline) {
    pipeline->temp_reg = memory[pc];
    pc++;
}

void decode_instruction(Pipeline *pipeline) {
    pipeline->opcode = (pipeline->temp_reg >> 26) & 0x3F;

    switch (pipeline->opcode) {
        case 0: case 1: case 2: case 5: case 8: case 9:
            // R-type
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->r3 = (pipeline->temp_reg >> 11) & 0x1F;
            pipeline->shamt = (pipeline->temp_reg >> 6) & 0x1FFF;
            break;
        case 3: case 4: case 6: case 10: case 11:
            // I-type
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;

            pipeline->immediate = pipeline->temp_reg & 0x3FFFF;
            if (pipeline->immediate & (1 << 17))
                pipeline->immediate |= 0xFFFC0000;
            break;
        case 7:
            // J-type
            pipeline->address = pipeline->temp_reg & 0x0FFFFFFF;
            stop = true;
            break;
        default:
            printf("Invalid opcode\n");
            break;
    }
}

void execute_instruction(Pipeline *pipeline) {
    switch (pipeline->opcode) {
        // R-type
        case 0:  // ADD
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + R[pipeline->r3];
            break;
        case 1:  // SUB
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] - R[pipeline->r3];
            break;
        case 2:  // MUL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] * R[pipeline->r3];
            break;
        case 3:  // MOVI
                pipeline->wb = true;
                pipeline->temp_address = pipeline->r1;
                pipeline->temp_reg = pipeline->immediate;
                break;
        case 4:  // JEQ
            if (R[pipeline->r1] == R[pipeline->r2]) {
                pc = pc + 1 + pipeline->immediate;
                    }
            break;
        case 5:  // AND
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] & R[pipeline->r3];
            break;
        case 8:  // LSL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] << pipeline->shamt;
            break;
        case 9:  // LSR
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] >> pipeline->shamt;
            break;
            //i type
        case 6:  // XORI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] ^ pipeline->immediate;
            break;
        case 10:  // MOVR
            pipeline->mem = true;
            pipeline->write_read = 2;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + pipeline->immediate;
            break;
        case 11:  // MOVM
            pipeline->mem = true;
            pipeline->write_read = 1;
            pipeline->temp_address = R[pipeline->r2] + pipeline->immediate;
            break;
        case 7:  // JMP
        pc = (pc & 0xF0000000) | pipeline->address;
        break;
        default:
            printf("Invalid opcode\n");
            break;
    }
}

void mem_stage(Pipeline *pipeline) {
    if (!pipeline->mem) return;

    switch (pipeline->write_read) {
        case 1:
            memory[pipeline->temp_address] = R[pipeline->r1];
            break;
        case 2:
            pipeline->wb = true;
            pipeline->temp_reg = memory[pipeline->temp_reg];
            break;
        default:
            printf("Invalid memory operation\n");
            break;
    }

    pipeline->mem = false;
    pipeline->write_read = 0;
}

void wb_stage(Pipeline *pipeline) {
    if (!pipeline->wb) return;
    if (pipeline->temp_address != 0)
        R[pipeline->temp_address] = pipeline->temp_reg;
    pipeline->wb = false;
}

void pipeline_stages(Pipeline *pipeline) {
switch (pipeline->stage) {
case 0: // IF
    if (MEM) {
        pipeline->stage = 0;
        return;
    }
    else{
    IF = true;   
    fetch_instruction(pipeline); 
    clock ++;
    pipeline->stage = 1;
    IF = false;
    }
    break;
case 1: // ID
    decode_instruction(pipeline);
    clock += 2;
    pipeline->stage = 2;
    break;
case 2: // EX
    execute_instruction(pipeline);
    clock += 2;
    pipeline->stage = 3;
    break;
case 3: // MEM
    if(IF){
        pipeline->stage = 3;
        return;
    }else{
    MEM = true;   
    mem_stage(pipeline);
    clock ++;
    pipeline->stage = 4;
    MEM = false;
    }
    break;
case 4: // WB
    wb_stage(pipeline);
    clock ++;
    pipeline->stage = 0;
    break;
}
}


     

int main(int argc, char const *argv[]) {
    //for (int i = 0; i < NUM_REGS; i++) R[i] = 0;
    //loadMemory(NULL);
    

    return 0;
}
