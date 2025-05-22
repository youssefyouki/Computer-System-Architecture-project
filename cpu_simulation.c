#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MEMORY_SIZE 2048
#define INSTR_MEM_SIZE 1024

int memory[MEMORY_SIZE];
int R[32];
int pc ;
bool IF = false;
bool MEM = false;
int clock_cycle;
int total_instructions = 0;
int fetch_counter;
bool branch_taken;
int branch_pc = 0;
int branch_clock = 0;
int instructions_completed;

typedef struct {
    int stage_clock;
    bool wb;
    bool mem;
    int write_read;
    int stage;
    int temp_reg;
    int temp_address;
    int opcode;
    int r1, r2, r3;
    int shamt;
    int immediate;
    int address;
    int current_pc;
} Pipeline;
Pipeline pipelines[4];

const char* opcode_names[] = {
    "ADD", "SUB", "MUL", "MOVI", "JEQ", "AND",
    "XORI", "JMP", "LSL", "LSR", "MOVR", "MOVM"
};

void initialize_system() {
    memset(memory, 0, sizeof(memory));
    memset(R, 0, sizeof(R));
    R[0] = 0; // Zero register
    pc = 0;
    clock_cycle = 0;
    fetch_counter = 0;
    branch_taken = false;
    instructions_completed = 0;

    for (int i = 0; i < 4; i++) {
        pipelines[i].stage = -1;
        pipelines[i].stage_clock = 0;
        pipelines[i].wb = false;
        pipelines[i].mem = false;
        pipelines[i].write_read = 0;
        pipelines[i].temp_reg = 0;
        pipelines[i].current_pc = 0;
    }
}

void load_instructions(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }

    char line[256];
    int address = 0;
    while (fgets(line, sizeof(line), file) && address < INSTR_MEM_SIZE) {
        line[strcspn(line, "\n")] = 0;
        
        if (line[0] == '#' || line[0] == '\0') continue;

        char opcode_str[16];
        int r1, r2, r3, imm, shamt, addr;
        
        if (sscanf(line, "%s", opcode_str) != 1) continue;

        if (strcmp(opcode_str, "ADD") == 0) {
            sscanf(line, "ADD R%d R%d R%d", &r1, &r2, &r3);
            memory[address] = (0 << 26) | (r1 << 21) | (r2 << 16) | (r3 << 11);
        }
        else if (strcmp(opcode_str, "SUB") == 0) {
            sscanf(line, "SUB R%d R%d R%d", &r1, &r2, &r3);
            memory[address] = (1 << 26) | (r1 << 21) | (r2 << 16) | (r3 << 11);
        }
        else if (strcmp(opcode_str, "MUL") == 0) {
            sscanf(line, "MUL R%d R%d R%d", &r1, &r2, &r3);
            memory[address] = (2 << 26) | (r1 << 21) | (r2 << 16) | (r3 << 11);
        }
        else if (strcmp(opcode_str, "MOVI") == 0) {
            sscanf(line, "MOVI R%d %d", &r1, &imm);
            memory[address] = (3 << 26) | (r1 << 21) | (imm & 0x3FFFF);
        }
        else if (strcmp(opcode_str, "JEQ") == 0) {
            sscanf(line, "JEQ R%d R%d %d", &r1, &r2, &imm);
            memory[address] = (4 << 26) | (r1 << 21) | (r2 << 16) | (imm & 0x3FFFF);
        }
        else if (strcmp(opcode_str, "AND") == 0) {
            sscanf(line, "AND R%d R%d R%d", &r1, &r2, &r3);
            memory[address] = (5 << 26) | (r1 << 21) | (r2 << 16) | (r3 << 11);
        }
        else if (strcmp(opcode_str, "XORI") == 0) {
            sscanf(line, "XORI R%d R%d %d", &r1, &r2, &imm);
            memory[address] = (6 << 26) | (r1 << 21) | (r2 << 16) | (imm & 0x3FFFF);
        }
        else if (strcmp(opcode_str, "JMP") == 0) {
            sscanf(line, "JMP %d", &addr);
            memory[address] = (7 << 26) | (addr & 0x0FFFFFFF);
        }
        else if (strcmp(opcode_str, "LSL") == 0) {
            sscanf(line, "LSL R%d R%d %d", &r1, &r2, &shamt);
            memory[address] = (8 << 26) | (r1 << 21) | (r2 << 16) | (shamt & 0x1FFF);
        }
        else if (strcmp(opcode_str, "LSR") == 0) {
            sscanf(line, "LSR R%d R%d %d", &r1, &r2, &shamt);
            memory[address] = (9 << 26) | (r1 << 21) | (r2 << 16) | (shamt & 0x1FFF);
        }
        else if (strcmp(opcode_str, "MOVR") == 0) {
            sscanf(line, "MOVR R%d R%d %d", &r1, &r2, &imm);
            memory[address] = (10 << 26) | (r1 << 21) | (r2 << 16) | (imm & 0x3FFFF);
        }
        else if (strcmp(opcode_str, "MOVM") == 0) {
            sscanf(line, "MOVM R%d R%d %d", &r1, &r2, &imm);
            memory[address] = (11 << 26) | (r1 << 21) | (r2 << 16) | (imm & 0x3FFFF);
        }
        else {
            printf("Unknown instruction: %s\n", line);
            continue;
        }
        
        address++;
        total_instructions++;
    }
    fclose(file);
}

void fetch_instruction(Pipeline *pipeline) {
    if (pc >= INSTR_MEM_SIZE || memory[pc] == 0) return;
    
    pipeline->temp_reg = memory[pc];
    pipeline->current_pc = pc;
    pc++;
    printf("Cycle %d: Fetched instruction from address %d\n", clock_cycle, pipeline->current_pc);
}

void decode_instruction(Pipeline *pipeline) {
    pipeline->opcode = (pipeline->temp_reg >> 26) & 0x3F;

    switch (pipeline->opcode) {
        case 0: case 1: case 2: case 5: case 8: case 9: // R-type
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->r3 = (pipeline->temp_reg >> 11) & 0x1F;
            pipeline->shamt = (pipeline->temp_reg >> 6) & 0x1FFF;
            break;
            
        case 3: case 4: case 6: case 10: case 11: // I-type
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->immediate = pipeline->temp_reg & 0x3FFFF;
            if (pipeline->immediate & (1 << 17)) {
                pipeline->immediate |= 0xFFFC0000;
            }
            break;
            
        case 7: // J-type
            pipeline->address = pipeline->temp_reg & 0x0FFFFFFF;
            break;
            
        default:
            printf("Invalid opcode: %d\n", pipeline->opcode);
            break;
    }
    
    printf("Cycle %d: Decoded %s instruction\n", clock_cycle, opcode_names[pipeline->opcode]);
}

void execute_instruction(Pipeline *pipeline) {
    switch (pipeline->opcode) {
        case 0: // ADD
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + R[pipeline->r3];
            break;
            
        case 1: // SUB
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] - R[pipeline->r3];
            break;
            
        case 2: // MUL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] * R[pipeline->r3];
            break;
            
        case 3: // MOVI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = pipeline->immediate;
            break;
            
        case 4: // JEQ
            if (R[pipeline->r1] == R[pipeline->r2]) {
                branch_taken = true;
                branch_pc = pipeline->current_pc + 1 + pipeline->immediate;
                branch_clock = clock_cycle;
                printf("Cycle %d: Branch taken to address %d\n", clock_cycle, branch_pc);
            }
            break;
            
        case 5: // AND
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] & R[pipeline->r3];
            break;
            
        case 6: // XORI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] ^ pipeline->immediate;
            break;
            
        case 7: // JMP
            branch_taken = true;
            branch_pc = (pipeline->current_pc & 0xF0000000) | pipeline->address;
            branch_clock = clock_cycle;
            printf("Cycle %d: Jumping to address %d\n", clock_cycle, branch_pc);
            break;
            
        case 8: // LSL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] << pipeline->shamt;
            break;
            
        case 9: // LSR
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = (unsigned)R[pipeline->r2] >> pipeline->shamt;
            break;
            
        case 10: // MOVR
            pipeline->mem = true;
            pipeline->write_read = 2;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + pipeline->immediate;
            break;
            
        case 11: // MOVM
            pipeline->mem = true;
            pipeline->write_read = 1;
            pipeline->temp_address = R[pipeline->r2] + pipeline->immediate;
            pipeline->temp_reg = R[pipeline->r1];
            break;
            
        default:
            printf("Invalid opcode in execute: %d\n", pipeline->opcode);
            break;
    }
}

void mem_stage(Pipeline *pipeline) {
    if (!pipeline->mem) return;

    switch (pipeline->write_read) {
        case 1: // Write to memory
            memory[pipeline->temp_address] = pipeline->temp_reg;
            printf("Cycle %d: Memory[%d] updated to %d\n", 
                  clock_cycle, pipeline->temp_address, pipeline->temp_reg);
            break;
            
        case 2: // Read from memory
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
    
    if (pipeline->temp_address == 0) {
        printf("Cycle %d: Attempt to write to R0 (Zero Register) - Ignored\n", clock_cycle);
    } else {
        R[pipeline->temp_address] = pipeline->temp_reg;
        printf("Cycle %d: R%d updated to %d\n", 
              clock_cycle, pipeline->temp_address, pipeline->temp_reg);
    }
    pipeline->wb = false;
    instructions_completed++;
}

void pipeline_stages(Pipeline *pipeline) {
    switch (pipeline->stage) {
        case 0: // IF
            if (MEM || pc >= INSTR_MEM_SIZE || memory[pc] == 0) {
                printf("Cycle %d: IF stalled\n", clock_cycle);
                return;
            }
            IF = true;
            fetch_instruction(pipeline);
            pipeline->stage = 1;
            pipeline->stage_clock = 0;
            IF = false;
            fetch_counter = 0;
            break;
            
        case 1: // ID (2 cycles)
            if (pipeline->stage_clock < 1) {
                decode_instruction(pipeline);
                pipeline->stage_clock++;
            } else {
                pipeline->stage = 2;
                pipeline->stage_clock = 0;
            }
            break;
            
        case 2: // EX (2 cycles)
            if (pipeline->stage_clock < 1) {
                execute_instruction(pipeline);
                pipeline->stage_clock++;
            } else {
                pipeline->stage = 3;
                pipeline->stage_clock = 0;
            }
            break;
            
        case 3: // MEM (1 cycle)
            if (IF) {
                printf("Cycle %d: MEM stalled due to IF\n", clock_cycle);
                return;
            }
            MEM = true;
            mem_stage(pipeline);
            pipeline->stage = 4;
            MEM = false;
            break;
            
        case 4: // WB (1 cycle)
            wb_stage(pipeline);
            pipeline->stage = -1;
            break;
    }
}

void advance_pipeline() {
    // Handle branch/jump flushing
    if (branch_taken && clock_cycle >= branch_clock + 2) {
        pc = branch_pc;
        printf("Cycle %d: Flushing pipeline after branch/jump to PC=%d\n", clock_cycle, pc);
        
        for (int i = 0; i < 4; i++) {
            if (pipelines[i].stage < 2) {
                memset(&pipelines[i], 0, sizeof(Pipeline));
                pipelines[i].stage = -1;
            }
        }
        branch_taken = false;
    }

    // Process stages in reverse order
    for (int i = 3; i >= 0; i--) {
        if (pipelines[i].stage != -1) {
            pipeline_stages(&pipelines[i]);
        }
    }

    // Fetch new instruction every 2 cycles
    if (fetch_counter % 2 == 0 && !MEM && pc < INSTR_MEM_SIZE && memory[pc] != 0) {
        for (int i = 0; i < 4; i++) {
            if (pipelines[i].stage == -1) {
                memset(&pipelines[i], 0, sizeof(Pipeline));
                pipelines[i].stage = 0;
                break;
            }
        }
    }

    clock_cycle++;
    fetch_counter++;
}

void print_pipeline_state() {
    printf("\n=== Cycle %d ===\n", clock_cycle);
    printf("PC: %d\n", pc);
    
    for (int i = 0; i < 4; i++) {
        printf("Pipeline %d: ", i);
        switch(pipelines[i].stage) {
            case -1: printf("INACTIVE"); break;
            case 0: printf("IF"); break;
            case 1: printf("ID (cycle %d)", pipelines[i].stage_clock+1); break;
            case 2: printf("EX (cycle %d)", pipelines[i].stage_clock+1); break;
            case 3: printf("MEM"); break;
            case 4: printf("WB"); break;
        }
        
        if (pipelines[i].opcode >= 0 && pipelines[i].opcode < 12) {
            printf(" (%s)", opcode_names[pipelines[i].opcode]);
            if (pipelines[i].stage >= 1 && pipelines[i].stage <= 2) {
                printf(" R%d,R%d", pipelines[i].r1, pipelines[i].r2);
                if (pipelines[i].opcode != 3 && pipelines[i].opcode != 4 && 
                    pipelines[i].opcode != 6 && pipelines[i].opcode != 7 &&
                    pipelines[i].opcode != 10 && pipelines[i].opcode != 11) {
                    printf(",R%d", pipelines[i].r3);
                }
            }
        }
        printf("\n");
    }
}

void print_final_state() {
    printf("\n=== Final State ===\n");
    
    printf("\nRegister Values:\n");
    for (int i = 0; i < 32; i++) {
        printf("R%d: %d\n", i, R[i]);
    }
    
    printf("\nInstruction Memory Contents (non-zero):\n");
    for (int i = 0; i < INSTR_MEM_SIZE; i++) {
        if (memory[i] != 0) {
            printf("[%d]: %08x\n", i, memory[i]);
        }
    }
    
    printf("\nData Memory Contents (non-zero):\n");
    for (int i = INSTR_MEM_SIZE; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            printf("[%d]: %d\n", i, memory[i]);
        }
    }
    
    printf("\nTotal Clock Cycles: %d\n", clock_cycle - 1);
    printf("Total Instructions Executed: %d\n", instructions_completed);
}

int main(int argc, char const *argv[]) {

    initialize_system();
    load_instructions("program.txt");

    while (1) {
        
        advance_pipeline();
        print_pipeline_state();

        // Check termination conditions
        bool pipeline_empty = true;
        for (int i = 0; i < 4; i++) {
            if (pipelines[i].stage != -1) {
                pipeline_empty = false;
                break;
            }
        }
        
        if (pipeline_empty && (pc >= INSTR_MEM_SIZE || memory[pc] == 0)) {
            break;
        }


    }

    print_final_state();
    return 0;
}