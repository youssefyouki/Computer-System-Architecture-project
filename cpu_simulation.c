#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

int memory[2048];
int R[32];
int pc;
bool stop = false;
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
        case 6:  // XORI
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] ^ pipeline->immediate;
            break;
        case 7:  // JMP
            pc = (pc & 0xF0000000) | pipeline->address;
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

void readFromFile(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
char line[256];
    int address = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue; // Skip comments
        if (sscanf(line, "%d", &memory[address]) != 1) {
            fprintf(stderr, "Error reading memory at address %d\n", address);
            exit(EXIT_FAILURE);
        }
        address++;
    }
    if(ferror(file)) {
        perror("Error reading file");
    }
    fclose(file);
}

static int get_opcode(const char *mnem) {
    if      (strcmp(mnem, "ADD")==0)   return 0;
    else if (strcmp(mnem, "SUB")==0)   return 1;
    else if (strcmp(mnem, "MUL")==0)   return 2;
    else if (strcmp(mnem, "MOVI")==0)  return 3;
    else if (strcmp(mnem, "JEQ")==0)   return 4;
    else if (strcmp(mnem, "AND")==0)   return 5;
    else if (strcmp(mnem, "XORI")==0)  return 6;
    else if (strcmp(mnem, "JMP")==0)   return 7;
    else if (strcmp(mnem, "LSL")==0)   return 8;
    else if (strcmp(mnem, "LSR")==0)   return 9;
    else if (strcmp(mnem, "MOVR")==0)  return 10;
    else if (strcmp(mnem, "MOVM")==0)  return 11;
    fprintf(stderr, "Unknown mnemonic '%s'\n", mnem);
    exit(EXIT_FAILURE);
}

static int parse_reg(const char *tok) {
    if ((tok[0]=='R' || tok[0]=='r') && isdigit(tok[1])) {
        int r = atoi(tok+1);
        if (r >= 0 && r <= 31) return r;
    }
    fprintf(stderr, "Invalid register '%s'\n", tok);
    exit(EXIT_FAILURE);
}

static uint32_t assemble_inst(char *line) {
    char *mnem = strtok(line, " \t\n");
    if (!mnem || mnem[0]=='#') return UINT32_MAX;  // skip blank/comment

    int opc = get_opcode(mnem);
    uint32_t inst = (opc & 0xF) << 28;

    switch (opc) {
      /* R-format: OPCODE R1 R2 R3 SHAMT */
      case 0: case 1: case 2: case 5: case 8: case 9: {
        int r1 = parse_reg(strtok(NULL, " ,\t\n"));
        int r2 = parse_reg(strtok(NULL, " ,\t\n"));
        int r3 = parse_reg(strtok(NULL, " ,\t\n"));
        int sh = atoi(strtok(NULL, " ,\t\n")) & 0x1FFF;  // 13 bits
        inst |= (r1 & 0x1F) << 23
             | (r2 & 0x1F) << 18
             | (r3 & 0x1F) << 13
             |  sh;
        break;
      }
      
      case 7: {
        int addr = atoi(strtok(NULL, " ,\t\n")) & 0x0FFFFFFF;  // 28 bits
        inst |= addr;
        break;
      }
      default:
        fprintf(stderr, "Unhandled opcode %d\n", opc);
        exit(EXIT_FAILURE);
    }

    return inst;
}

void translate_assembly(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen"); exit(EXIT_FAILURE); }

    char line[256];
    int addr = 0;
    while (fgets(line, sizeof(line), f)) {
        uint32_t code = assemble_inst(line);
        if (code == UINT32_MAX) continue;  // skipped comment/blank
        if (addr >= sizeof(memory)/sizeof(memory[0])) {
            fprintf(stderr, "Program too large for memory\n");
            break;
        }
        memory[addr++] = code;
    }
    fclose(f);
}


/* (… paste your definitions of memory[], readFromFile(), etc. here …) */

int main(int argc, char *argv[]) {
    // 1. Prepare a small test file on disk
     char *test_filename = "test_memory.txt";
    FILE *tf = fopen(test_filename, "w");
    if (!tf) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    // write 5 known values, one per line
    fprintf(tf, "# sample memory contents\n");
    fprintf(tf, "42\n");
    fprintf(tf, "  -7\n");
    fprintf(tf, "12345\n");
    fprintf(tf, "0\n");
    fprintf(tf, "999\n");
    fclose(tf);

    // 2. Call readFromFile
    readFromFile(test_filename);

    // 3. Check (or print) the results
    // You can either print them:
    for (int i = 0; i < 5; i++) {
        printf("memory[%d] = %d\n", i, memory[i]);
    }

    // … or use assertions to “fail fast” on mismatch:
    assert(memory[0] == 42);
    assert(memory[1] == -7);
    assert(memory[2] == 12345);
    assert(memory[3] == 0);
    assert(memory[4] == 999);

    printf("readFromFile test passed!\n");
    return EXIT_SUCCESS;
}

