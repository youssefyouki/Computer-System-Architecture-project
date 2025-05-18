#include <stdint.h>
#include <stdbool.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
const uint32_t R0 = 0;

bool stop = false;
int clock;

typedef struct {
    bool wb;
    bool mem;
    int write_read; // 0:no ,1: read, 2: write
    int stage;  // 0: IF, 1: ID, 2: EX, 3: MEM, 4: WB
    uint32_t temp_address;
    uint8_t opcode;
    uint8_t r1, r2, r3;
    uint16_t shamt;       // for R-type
    int32_t immediate;    // for I-type
    uint32_t address;     // for J-type
    uint32_t temp_reg;

} Pipeline;

void fetch_instruction(Pipeline *pipeline){
    pipeline->temp_reg = memory[pc];
    pc ++;
    
}

void decode_instruction(Pipeline *pipeline){
    pipeline->opcode = (pipeline->temp_reg >> 26) & 0x3F;
    switch (pipeline->opcode){
        case 0:
        case 1:
        case 2:
        case 5:
        case 8:
        case 9:
            //r type instructions
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->r3 = (pipeline->temp_reg >> 11) & 0x1F;
            pipeline->shamt = (pipeline->temp_reg >> 6) & 0x1FFF;
            break;
        case 3:
        case 4:
        case 6:
        case 10:
        case 11:
            // i type instructions
            pipeline->r1 = (pipeline->temp_reg >> 21) & 0x1F;
            pipeline->r2 = (pipeline->temp_reg >> 16) & 0x1F;
            pipeline->immediate = pipeline->temp_reg & 0xFFFF;
            break;
        case 7:
            // j type instructions
            //stop IF
            stop = true;
            pipeline-> address = pipeline->temp_reg & 0x3FFFFFF;
            break;
        default:
            // invalid instruction
            printf("Invalid opcode\n");
            break;}       
    
}

void execute_instruction_r(Pipeline *pipeline){
    //r type instructions
    switch (pipeline->opcode){ 
        case 0: // add
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + R[pipeline->r3];
            break;
        case 1: // sub
            
        pipeline->wb = true;
        pipeline->temp_address = pipeline->r1;
        pipeline->temp_reg = R[pipeline->r2] - R[pipeline->r3];
            break;
        case 2: // mul
            
        pipeline->wb = true;
        pipeline->temp_address = pipeline->r1;
        pipeline->temp_reg = R[pipeline->r2] * R[pipeline->r3];
            break;
        case 5: // and
            
        pipeline->wb = true;
        pipeline->temp_address = pipeline->r1;
        pipeline->temp_reg = R[pipeline->r2] & R[pipeline->r3];
            break;
        case 8: //lsl;
        pipeline->wb = true;
        pipeline->temp_address = pipeline->r1;
        pipeline->temp_reg = R[pipeline->r2] << pipeline->shamt;
            break;
        case 9: // lsr
        pipeline->wb = true;
        pipeline->temp_address = pipeline->r1;
        pipeline->temp_reg = R[pipeline->r2] >> pipeline->shamt;
            break;
        otherwise:
        printf("wrong opcode in exec method type r\n");
        break;
                 }}
void execute_instruction_i(Pipeline *pipeline){
    // i type instructions
    switch (pipeline->opcode){ 
        case 3: // movi
            
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = pipeline->immediate;
            break;
        case 4: //jeq
            if (R[pipeline->r1] == R[pipeline->r2]){ 
                pc = pc + 1 + pipeline->immediate;}
            break;
        case 6: // xori
            pipeline->wb = true;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] ^ pipeline->immediate;//r2 xor immediate

            break;
        case 10: // movr
            //R[r1] = memory[R[r2] + immediate];
            pipeline->mem = true;
            pipeline->write_read = 2;
            pipeline->temp_address = pipeline->r1;
            pipeline->temp_reg = R[pipeline->r2] + pipeline->immediate;
            break;
        case 11: // movm
           // memory[R[r2]+immediate] = R[r1];
            
           pipeline->mem = true;
           pipeline->write_read = 1;
           pipeline->temp_address = R[pipeline->r2] + pipeline->immediate;
           pipeline->r1 = pipeline->r1;
            break;
        otherwise:
        printf("wrong opcode in exec method type i\n");
        break;
            } }
void execute_instruction_j(Pipeline *pipeline){
    // j type instructions
    if(pipeline->opcode == 7) {//jmp
        pc  = (pc & 0xF0000000) | pipeline->address; }
    else
        printf("wrong opcode in exec method type j\n");       
                 }



void mem(Pipeline *pipeline){
    if(pipeline->mem == false){
        return;
    }
    switch (pipeline->write_read){
        case 0:
            
            break;
        case 1:
            memory[pipeline->temp_address] = pipeline->temp_reg;
            
            break;
        case 2:
            pipeline->wb = true;
            pipeline->temp_reg =memory[pipeline->temp_address];
            pipeline->temp_address = pipeline->r1;
            break;
        default:
            printf("error in mem func\n");
            break;    
    }
    pipeline->mem = false;
    pipeline->write_read = 0;

}

void wb(Pipeline *pipeline){
    if(pipeline->wb == false){
        return;
    }
    R[pipeline->temp_address] = pipeline->temp_reg;
    pipeline->wb = false;
    
}


int main(int argc, char const *argv[]){
    //loaddata();
    pc = 0;
    clock = 0;
    



    return 0;
}
