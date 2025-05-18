#include <stdint.h>
#include <stdbool.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
const uint32_t R0 = 0;

bool stop = false;
int clock;

typedef struct {
    int stage;  // 0: IF, 1: ID, 2: EX, 3: MEM, 4: WB
    uint8_t opcode;
    uint8_t r1, r2, r3;
    uint16_t shamt;       // for R-type
    int32_t immediate;    // for I-type
    uint32_t address;     // for J-type
    uint32_t temp_reg;

} Pipeline;

void fetch_instruction(uint32_t pc){
    uint32_t temp = memory[pc];
    pc ++;
    
}

void decode_instruction(uint32_t instruction){
    uint32_t opcode = (instruction >> 26) & 0x3F;
    switch (opcode){
        case 0:
        case 1:
        case 2:
        case 5:
        case 8:
        case 9:
            //r type instructions
            uint32_t r1 = (instruction >> 21) & 0x1F;
            uint32_t r2 = (instruction >> 16) & 0x1F;
            uint32_t r3 = (instruction >> 11) & 0x1F;
            uint32_t shamt = (instruction >> 6) & 0x1FFF;
            
            execute_instruction_r(opcode, r1, r2, r3, shamt);
            break;
        case 3:
        case 4:
        case 6:
        case 10:
        case 11:
            // i type instructions
            uint32_t r1 = (instruction >> 21) & 0x1F;
            uint32_t r2 = (instruction >> 16) & 0x1F;
            uint32_t immediate = instruction & 0xFFFF;
            
            execute_instruction_i(opcode, r1, r2, immediate);
            break;
        case 7:
            // j type instructions
            //stop IF
            stop = true;
            uint32_t address = instruction & 0x3FFFFFF;
            
            execute_instruction_j(opcode, address);
            break;
        default:
            // invalid instruction
            printf("Invalid opcode\n");
            break;}       
    
}

void execute_instruction_r(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t shamt){
    //r type instructions
    switch (opcode) {
        case 0: // add
            
            mem(0, 0, 0);
            wb(r1,R[r2] + R[r3]);
            break;
        case 1: // sub
            
            mem(0, 0, 0);
            wb(r1,R[r2] - R[r3]);
            break;
        case 2: // mul
            
            mem(0, 0, 0);
            wb(r1,R[r2] * R[r3]);
            break;
        case 5: // and
            
            mem(0, 0, 0);
            wb(r1,R[r2] & R[r3]);
            break;
        case 8: //lsl;
            
            mem(0, 0, 0);
            wb(r1,R[r2] << shamt);
            break;
        case 9: // lsr
            wb(r1,R[r2] >> shamt);
            break;
        otherwise:
        printf("wrong opcode in exec method type r\n");
        break;
                 }}
void execute_instruction_i(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t immediate){
    // i type instructions
    switch (opcode) {
        case 3: // movi
            
            mem(0, 0, 0);
            wb(r1, immediate);
            break;
        case 4: //jeq
            if (R[r1] == R[r2]) 
                pc = pc + 1 + immediate;
            
            mem(0, 0, 0);
            //wb();
            break;
        case 6: // xori
            
            mem(0, 0, 0);
            wb(r1,R[r2] ^ immediate); //r2 xor immediate

            break;
        case 10: // movr
            //R[r1] = memory[R[r2] + immediate];
                
            mem(2, R[r2] + immediate, r1);
            break;
        case 11: // movm
           // memory[R[r2]+immediate] = R[r1];
            
            mem(1, R[r2] + immediate, r1);
            //wb();
            break;
        default:
        printf("wrong opcode in exec method type i\n");
        break;
            } }
void execute_instruction_j(uint32_t opcode, uint32_t address){
    // j type instructions
    if(opcode == 7) {//jmp
        
        mem(0, 0, 0);
        pc  = (pc & 0xF0000000) | address;
        //activte IF
        stop = false;
        }
    else
        printf("wrong opcode in exec method type j\n");       
                 }



void mem(int write,uint32_t mem_address,uint32_t register_address){
    switch (write){
        case 0:
            
            break;
        case 1:
            memory[mem_address] = R[register_address];
            
            break;
        case 2:
            
            wb(register_address, memory[mem_address]);
            break;
        default:
            printf("error in mem func\n");
            break;    
    }

}

void wb(uint32_t address,uint32_t data){
    R[address] = data;
    
}
void wb(){
    
}

int main(int argc, char const *argv[]){
    loaddata();
    pc = 0;
    clock = 0;
    



    return 0;
}
