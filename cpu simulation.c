#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
const uint32_t R0 = 0;

bool stop = false;
sem_t mutex;


int clock = 0;


void fetch_instruction(uint32_t pc){
    sem_wait(&mutex);
    if (stop) {
        sem_post(&mutex);
        return;
    }else{
    uint32_t temp = memory[pc];
    pc ++;
    clock ++;
    sem_post(&mutex);
    decode_instruction(temp);}
}

void decode_instruction(uint32_t instruction){
    uint32_t opcode = (instruction >> 26) & 0x3F;
    case (opcode):{
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
            clock += 2;
            execute_instruction(opcode, r1, r2, r3, shamt);
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
            clock += 2;
            execute_instruction(opcode, r1, r2, immediate);
            break;
        case 7:
            // j type instructions
            //stop IF
            stop = true;
            uint32_t address = instruction & 0x3FFFFFF;
            clock += 2;
            execute_instruction(opcode, address);
            break;
        otherwise:
            // invalid instruction
            printf("Invalid opcode\n");
            break;}       
    
}

void execute_instruction(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t shamt){
    //r type instructions
    switch (opcode) {
        case 0: // add
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] + R[r3]);
            break;
        case 1: // sub
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] - R[r3]);
            break;
        case 2: // mul
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] * R[r3]);
            break;
        case 5: // and
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] & R[r3]);
            break;
        case 8: //lsl;
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] << shamt);
            break;
        case 9: // lsr
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] >> shamt);
            break;
        otherwise:
        printf("wrong opcode in exec method type r\n");
        break;
                 }}
void execute_instruction(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t immediate){
    // i type instructions
    switch (opcode) {
        case 3: // movi
            clock += 2;
            mem(0, 0, 0);
            wb(r1, immediate);
            break;
        case 4: //jeq
            if (R[r1] == R[r2]) 
                pc = pc + 1 + immediate;
            clock += 2;
            mem(0, 0, 0);
            wb();
            break;
        case 6: // xori
            clock += 2;
            mem(0, 0, 0);
            wb(r1,R[r2] ^ immediate); //r2 xor immediate

            break;
        case 10: // movr
            //R[r1] = memory[R[r2] + immediate];
            clock += 2;    
            mem(2, R[r2] + immediate, r1);
            break;
        case 11: // movm
           // memory[R[r2]+immediate] = R[r1];
            clock += 2;
            mem(1, R[r2] + immediate, r1);
            wb();
            break;
        otherwise:
        printf("wrong opcode in exec method type i\n");
        break;
            } }
void execute_instruction(uint32_t opcode, uint32_t address){
    // j type instructions
    if(opcode == 7) {//jmp
        clock += 2;
        mem(0, 0, 0);
        pc  = (pc & 0xF0000000) | address);
        //activte IF
        stop = false;
        }
    else
        printf("wrong opcode in exec method type j\n");       
                 }



void mem(int write,uint32_t mem_address,uint32_t register_address){
    sem_wait(&mutex);
    case (write){
        case 0:
            clock ++;
            break;
        case 1:
            memory[mem_address] = R[register_address];
            clock ++;
            break;
        case 2:
            clock ++;
            wb(register_address, memory[mem_address]);
            break;
        otherwise:
            printf("error in mem func\n");
            break;    
    }
    sem_post(&mutex);

}

void wb(uint32_t address,uint32_t data){
    R[address] = data;
    clock ++;
}
void wb(){
    clock ++;
}

int main(int argc, char const *argv[])
{
    sem_int(&mutex, 0, 1);

    return 0;
}
