#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MEMORY_SIZE 2048
#define INSTR_MEM_SIZE 1024

// Helper function to convert an integer to a binary string
void int_to_binary_str(int num, char *str, int bits) {
    str[bits] = '\0';
    for (int i = bits - 1; i >= 0; i--) {
        str[bits - 1 - i] = ((num >> i) & 1) ? '1' : '0';
    }
}

// Global system state variables
int memory[MEMORY_SIZE];
int R[32];
int pc;
bool IF = false;
bool MEM = false;
int clock_cycle;
int total_instructions = 0;
int fetch_counter;
bool branch_taken;
int branch_pc = 0;
int branch_clock = 0;
int instructions_completed;

// Pipeline structure
typedef struct {
    int stage_clock;
    bool wb;
    bool mem;
    int write_read;
    int stage;           // -1=inactive, 0=IF, 1=ID, 2=EX, 3=MEM, 4=WB
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

// Array of instruction names for debug output
const char* opcode_names[] = {
    "ADD", "SUB", "MUL", "MOVI", "JEQ", "AND",
    "XORI", "JMP", "LSL", "LSR", "MOVR", "MOVM"
};

// Initialize the CPU and memory system
void initialize_system() {
    memset(memory, 0, sizeof(memory));
    memset(R, 0, sizeof(R));
    R[0] = 0;  // Zero register
    pc = 0;
    clock_cycle = 0;
    fetch_counter = 0;
    branch_taken = false;
    instructions_completed = 0;

    // Initialize all pipeline slots
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

// Load assembly instructions from file into memory
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

// Fetch instruction from memory
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
            printf("Cycle %d: Decoded R-type instruction: r1=%d, r2=%d, r3=%d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, pipeline->r3);
            break;

        case 3: case 4: case 6: // I-type with sign extension
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->immediate = pipeline->temp_reg & 0x3FFFF;
            // Sign extension
            if (pipeline->immediate & (1 << 17)) {
                pipeline->immediate |= 0xFFFC0000;
            }
            break;
            
        case 10: case 11: // MOVR, MOVM (no sign extension)
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->immediate = pipeline->temp_reg & 0x3FFFF;
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
    int r2_value = 0;
    int r3_value = 0;
    
    // Safety bounds checking
    if (pipeline->r2 >= 0 && pipeline->r2 < 32) {
        r2_value = R[pipeline->r2];
    } else {
        printf("Cycle %d: Error - Invalid register R%d access\n", clock_cycle, pipeline->r2);
    }
    
    if (pipeline->r3 >= 0 && pipeline->r3 < 32) {
        r3_value = R[pipeline->r3];
    } else {
        printf("Cycle %d: Error - Invalid register R%d access\n", clock_cycle, pipeline->r3);
    }

    switch (pipeline->opcode) {
        case 0: // ADD
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value + r3_value;
            printf("Cycle %d: ADD instruction R%d = R%d(%d) + R%d(%d) = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->r3, r3_value, pipeline->temp_reg);
            break;
              case 1: // SUB
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value - r3_value;
            printf("Cycle %d: SUB instruction R%d = R%d(%d) - R%d(%d) = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->r3, r3_value, pipeline->temp_reg);
            break;
              case 2: // MUL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value * r3_value;
            printf("Cycle %d: MUL instruction R%d = R%d(%d) * R%d(%d) = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->r3, r3_value, pipeline->temp_reg);
            break;
            
        case 3: // MOVI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = pipeline->immediate;
            break;
              case 4: // JEQ
            if (r2_value == r3_value) {
                branch_taken = true;
                branch_pc = pipeline->current_pc + 1 + pipeline->immediate;
                branch_clock = clock_cycle;
                printf("Cycle %d: Branch taken to address %d\n", clock_cycle, branch_pc);
            }
            break;
              case 5: // AND
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value & r3_value;
            printf("Cycle %d: AND instruction R%d = R%d(%d) & R%d(%d) = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->r3, r3_value, pipeline->temp_reg);
            break;
              case 6: // XORI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value ^ pipeline->immediate;
            printf("Cycle %d: XORI instruction R%d = R%d(%d) ^ %d = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->immediate, pipeline->temp_reg);
            break;        case 7: // JMP
            branch_taken = true;
            branch_pc = pipeline->address % INSTR_MEM_SIZE; // Ensure address is within bounds of instruction memory
            branch_clock = clock_cycle;
            printf("Cycle %d: Jumping to address %d\n", clock_cycle, branch_pc);
            break;
              case 8: // LSL
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = r2_value << pipeline->shamt;
            printf("Cycle %d: LSL instruction R%d = R%d(%d) << %d = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->shamt, pipeline->temp_reg);
            break;
              case 9: // LSR
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = (unsigned)r2_value >> pipeline->shamt;
            printf("Cycle %d: LSR instruction R%d = R%d(%d) >> %d = %d\n", 
                  clock_cycle, pipeline->r1, pipeline->r2, r2_value, 
                  pipeline->shamt, pipeline->temp_reg);
            break;            case 10: // MOVR
            pipeline->mem = true;
            pipeline->write_read = 2;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = (R[pipeline->r2] + pipeline->immediate) % MEMORY_SIZE; // Ensure address is within bounds
            printf("Cycle %d: MOVR instruction - Will read from Memory[%d] to R%d\n", 
                  clock_cycle, pipeline->temp_reg, pipeline->temp_address);
            break;            case 11: // MOVM
            pipeline->mem = true;
            pipeline->write_read = 1;
            pipeline->temp_address = (R[pipeline->r2] + pipeline->immediate) % MEMORY_SIZE; // Ensure address is within bounds
            pipeline->temp_reg = R[pipeline->r1];
            printf("Cycle %d: MOVM instruction - Will store R%d(%d) to Memory[%d]\n", 
                  clock_cycle, pipeline->r1, pipeline->temp_reg, pipeline->temp_address);
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
            if (pipeline->temp_address >= 0 && pipeline->temp_address < MEMORY_SIZE) {
                memory[pipeline->temp_address] = pipeline->temp_reg;
                printf("Cycle %d: Memory[%d] updated to %d\n", 
                      clock_cycle, pipeline->temp_address, pipeline->temp_reg);
            } else {
                printf("Cycle %d: ERROR - Memory write access out of bounds: %d\n", 
                      clock_cycle, pipeline->temp_address);
            }
            break;
            
        case 2: // Read from memory
            pipeline->wb = true;
            if (pipeline->temp_reg >= 0 && pipeline->temp_reg < MEMORY_SIZE) {
                pipeline->temp_reg = memory[pipeline->temp_reg];
            } else {
                printf("Cycle %d: ERROR - Memory read access out of bounds: %d\n", 
                      clock_cycle, pipeline->temp_reg);
                pipeline->temp_reg = 0;
            }
            break;
            
        default:
            printf("Invalid memory operation\n");
            break;
    }

    // Reset memory operation flags
    pipeline->mem = false;
    pipeline->write_read = 0;
}

// Write-back stage
void wb_stage(Pipeline *pipeline) {
    if (!pipeline->wb) return;
    
    if (pipeline->temp_address < 0 || pipeline->temp_address >= 32) {
        printf("Cycle %d: ERROR - Invalid register write attempt to R%d\n", 
              clock_cycle, pipeline->temp_address);
    }
    else if (pipeline->temp_address == 0) {
        printf("Cycle %d: Attempt to write to R0 (Zero Register) - Ignored\n", clock_cycle);
    } 
    else {
        R[pipeline->temp_address] = pipeline->temp_reg;
        printf("Cycle %d: R%d updated to %d\n", 
              clock_cycle, pipeline->temp_address, pipeline->temp_reg);
    }
    
    pipeline->wb = false;
    instructions_completed++;
}

void pipeline_stages(Pipeline *pipeline) {
    switch (pipeline->stage) {
        case 0: // IF - Instruction Fetch stage
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
            
        case 1: // ID - Instruction Decode stage (2 cycles)
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

bool advance_pipeline() {
    // Handle branch/jump flushing
    if (branch_taken && clock_cycle >= branch_clock + 2) {
        pc = branch_pc;
        printf("Cycle %d: Flushing pipeline after branch/jump to PC=%d\n", clock_cycle, pc);
        
        for (int i = 0; i < 4; i++) {
            if (pipelines[i].stage < 2) {  // Only flush IF and ID stages
                memset(&pipelines[i], 0, sizeof(Pipeline));
                pipelines[i].stage = -1;
            }
        }
        branch_taken = false;
    }

    // Process stages in reverse order to avoid conflicts
    for (int i = 3; i >= 0; i--) {
        if (pipelines[i].stage != -1 && pipelines[i].stage != 0) { // Skip IF stage here
            pipeline_stages(&pipelines[i]);
        }
    }

    // Start new pipeline every 2 cycles and immediately fetch the instruction
    if (pc < INSTR_MEM_SIZE && memory[pc] != 0) {
        // Find the appropriate pipeline slot based on clock cycle
        int pipeline_slot = -1;
        if (clock_cycle % 2 == 1) { // Only start new pipelines on odd cycles
            for (int i = 0; i < 4; i++) {
                if (pipelines[i].stage == -1) {
                    pipeline_slot = i;
                    break;
                }
            }
        }        // Start new pipeline if slot found and not blocked by MEM stage
        if (pipeline_slot != -1 && !MEM) {
            // Initialize the pipeline slot
            memset(&pipelines[pipeline_slot], 0, sizeof(Pipeline));
            
            // Immediately fetch the instruction in the same cycle if possible
            if (!(MEM || pc >= INSTR_MEM_SIZE || memory[pc] == 0)) {
                IF = true;
                fetch_instruction(&pipelines[pipeline_slot]);
                pipelines[pipeline_slot].stage = 1; // Move directly to ID stage
                pipelines[pipeline_slot].stage_clock = 0;
                IF = false;
                fetch_counter = 0;
                printf("Cycle %d: Starting new pipeline in slot %d and fetched instruction\n", clock_cycle, pipeline_slot);
            } else {
                pipelines[pipeline_slot].stage = 0;
                printf("Cycle %d: Starting new pipeline in slot %d\n", clock_cycle, pipeline_slot);
            }
        }    }// Only increment the clock cycle if at least one pipeline stage is active
    bool any_active_pipeline = false;
    for (int i = 0; i < 4; i++) {
        if (pipelines[i].stage != -1) {
            any_active_pipeline = true;
            break;
        }
    }
    
    // If we have no active pipelines but there are still instructions to fetch
    if (!any_active_pipeline && pc < INSTR_MEM_SIZE && memory[pc] != 0) {
        any_active_pipeline = true;
    }
    
    if (any_active_pipeline) {
        clock_cycle++;
        return true; // Return true if pipeline advanced
    }
    
    return false; // Return false if no activity
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
        
        // Show instruction details if available
        if (pipelines[i].opcode >= 0 && pipelines[i].opcode < 12) {
            printf(" (%s)", opcode_names[pipelines[i].opcode]);
            // Show register operands for instructions in ID or EX stages
            if (pipelines[i].stage >= 1 && pipelines[i].stage <= 2) {
                printf(" R%d,R%d", pipelines[i].r1, pipelines[i].r2);
                // Show third register for R-type instructions
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
    char binary[33];
    for (int i = 0; i < INSTR_MEM_SIZE; i++) {
        if (memory[i] != 0) {
            int_to_binary_str(memory[i], binary, 32);
            printf("[%d]: %s\n", i, binary);
        }
    }
    
    printf("\nData Memory Contents (non-zero):\n");
    for (int i = INSTR_MEM_SIZE; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            int_to_binary_str(memory[i], binary, 32);
            printf("[%d]: %s\n", i, binary);
        }
    }
    
    printf("\nTotal Clock Cycles: %d\n", clock_cycle);
    printf("Total Instructions Executed: %d\n", instructions_completed);
}

int main(int argc, char const *argv[]) {

    initialize_system();
    load_instructions("program.txt");
    bool done = false;
    bool activity;
    
    while (!done) {
        activity = advance_pipeline();
          bool pipeline_empty = true;
        for (int i = 0; i < 4; i++) {
            if (pipelines[i].stage != -1) {
                pipeline_empty = false;
                break;
            }
        }
        
        if (pipeline_empty && (pc >= INSTR_MEM_SIZE || memory[pc] == 0)) {
            if (!activity) {
                done = true;
                printf("No active pipelines and no more instructions to fetch. Simulation complete.\n");
                break;
            }
        }
        
        print_pipeline_state();
    }

    print_final_state();
    return 0;
}