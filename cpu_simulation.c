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
char **read_file_words(const char *filename, size_t *out_count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return NULL;
    }
    size_t cap = 16, cnt = 0;
    char **words = malloc(cap * sizeof(char*));
    if (!words) { perror("malloc"); fclose(fp); return NULL; }
    char buf[256];
    while (fscanf(fp, "%255s", buf) == 1) {
        size_t L = strlen(buf);
        char *dup = malloc(L+1);
        if (!dup) { perror("malloc"); break; }
        memcpy(dup, buf, L+1);
        if (cnt == cap) {
            cap *= 2;
            char **tmp = realloc(words, cap * sizeof(char*));
            if (!tmp) { perror("realloc"); free(dup); break; }
            words = tmp;
        }
        words[cnt++] = dup;
    }
    if (ferror(fp)) perror("Error reading file");
    fclose(fp);
    *out_count = cnt;
    return words;
}
#define MEM_SIZE 2048
extern int memory[MEM_SIZE];  // your global memory[]

char **read_assembly_lines(const char *filename, size_t *out_count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen"); return NULL; }
    size_t cap = 16, cnt = 0;
    char **lines = malloc(cap * sizeof(char*));
    if (!lines) { perror("malloc"); fclose(fp); return NULL; }
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        size_t L = strlen(buf);
        if (L && buf[L-1]=='\n') buf[--L] = '\0';
        if (L == 0) continue;
        char *dup = malloc(L+1);
        if (!dup) { perror("malloc"); break; }
        memcpy(dup, buf, L+1);
        if (cnt == cap) {
            cap *= 2;
            char **tmp = realloc(lines, cap * sizeof(char*));
            if (!tmp) { perror("realloc"); free(dup); break; }
            lines = tmp;
        }
        lines[cnt++] = dup;
    }
    if (ferror(fp)) perror("Error reading file");
    fclose(fp);
    *out_count = cnt;
    return lines;
}

/*––– Part 2: assembler helpers –––*/

static int get_opcode(const char *mnem) {
    if      (!strcmp(mnem,"ADD"))   return 0;
    else if (!strcmp(mnem,"SUB"))   return 1;
    else if (!strcmp(mnem,"MUL"))   return 2;
    else if (!strcmp(mnem,"MOVI"))  return 3;
    else if (!strcmp(mnem,"JEQ"))   return 4;
    else if (!strcmp(mnem,"AND"))   return 5;
    else if (!strcmp(mnem,"XORI"))  return 6;
    else if (!strcmp(mnem,"JMP"))   return 7;
    else if (!strcmp(mnem,"LSL"))   return 8;
    else if (!strcmp(mnem,"LSR"))   return 9;
    else if (!strcmp(mnem,"MOVR"))  return 10;
    else if (!strcmp(mnem,"MOVM"))  return 11;
    fprintf(stderr,"Unknown mnemonic '%s'\n",mnem);
    exit(EXIT_FAILURE);
}

static int parse_reg(const char *tok) {
    if ((tok[0]=='R'||tok[0]=='r') && isdigit((unsigned char)tok[1])) {
        int r = atoi(tok+1);
        if (r>=0 && r<32) return r;
    }
    fprintf(stderr,"Invalid register '%s'\n",tok);
    exit(EXIT_FAILURE);
}

static uint32_t assemble_inst(char *line) {
    // break out the first token (mnemonic)
    char *mnem = strtok(line," \t,");
    if (!mnem || mnem[0]=='#') return UINT32_MAX;  // skip blank/comment

    int opc = get_opcode(mnem);
    uint32_t inst = (opc & 0xF) << 28;

    switch (opc) {
      /* R-format: OPCODE R1 R2 R3 SHAMT */
      case 0: case 1: case 2: case 5: case 8: case 9: {
        int r1 = parse_reg(strtok(NULL," \t,"));
        int r2 = parse_reg(strtok(NULL," \t,"));
        int r3 = parse_reg(strtok(NULL," \t,"));
        int sh = atoi(strtok(NULL," \t,")) & 0x1FFF;
        inst |= (r1 & 0x1F) << 23
             | (r2 & 0x1F) << 18
             | (r3 & 0x1F) << 13
             |  (uint32_t)sh;
        break;
      }

      /* I-format: OPCODE R1 R2 IMM */
      case 3: case 4: case 6: case 10: case 11: {
        int r1  = parse_reg(strtok(NULL," \t,"));
        int r2  = parse_reg(strtok(NULL," \t,"));
        int imm = atoi(strtok(NULL," \t,")) & 0x3FFFF;
        inst |= (r1 & 0x1F) << 23
             | (r2 & 0x1F) << 18
             |  (uint32_t)imm;
        break;
      }

      /* J-format: OPCODE ADDRESS */
      case 7: {
        int addr = atoi(strtok(NULL," \t,")) & 0x0FFFFFFF;
        inst |= (uint32_t)addr;
        break;
      }

      default:
        fprintf(stderr,"Unhandled opcode %d\n",opc);
        exit(EXIT_FAILURE);
    }
    return inst;
}

/*––– Part 3: walk the array, assemble each line, fill memory[] –––*/

void translate_assembly_lines(char **lines, size_t count) {
    int addr = 0;
    for (size_t i = 0; i < count; i++) {
        // make a writable copy since assemble_inst() uses strtok()
        char *cpy = strdup(lines[i]);
        uint32_t code = assemble_inst(cpy);
        free(cpy);

        if (code == UINT32_MAX) continue;  // comment/blank

        if (addr >= MEM_SIZE) {
            fprintf(stderr,"Program exceeds memory size\n");
            break;
        }
        memory[addr++] = (int)code;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <asm-file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Step 0: read all words into array
    size_t nwords;
    char **words = read_file_words(argv[1], &nwords);
    if (!words) return EXIT_FAILURE;
    
    // Example use: print words
    for (size_t i = 0; i < nwords; i++) {
        printf("word[%zu] = %s\n", i, words[i]);
        free(words[i]);
    }
    free(words);
    
    // Step 1: read assembly lines
    size_t nlines;
    char **lines = read_assembly_lines(argv[1], &nlines);
    if (!lines) return EXIT_FAILURE;

    // Step 2: assemble into memory[]
    translate_assembly_lines(lines, nlines);

    // cleanup lines array
    for (size_t i = 0; i < nlines; i++) free(lines[i]);
    free(lines);

    // (pipeline execution would go here)
    return EXIT_SUCCESS;
}