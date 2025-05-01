#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
const uint32_t R0 = 0;


sem_t mutex;


int clock = 0;


void fetch_instruction(uint32_t pc){
    sem_wait(&mutex);
    uint32_t temp = memory[pc];
    pc ++;
    sem_post(&mutex);
    decode_instruction(temp);
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
            // call execute
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
            // call execute
            break;
        case 7:
            // j type instructions
            //stop IF
            uint32_t address = instruction & 0x3FFFFFF;
            // call execute
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
            R[r1] = R[r2] + R[r3];
            break;
        case 1: // sub
            R[r1] = R[r2] - R[r3];
            break;
        case 2: // mul
            R[r1] = R[r2] * R[r3];
            break;
        case 5: // and
            R[r1] = R[r2] & R[r3];
            break;
        case 8: //lsl
            R[r1] = R[r2] << shamt;
            break;
        case 9: // lsr
            R[r1] = R[r2] >> shamt;
            break;
        otherwise:
        printf("wrong opcode in exec method type r\n");
        break;
                 }
void execute_instruction(uint32_t opcode, uint32_t r1, uint32_t r2, uint32_t immediate){
    // i type instructions
    switch (opcode) {
        case 3: // movi
            R[r1] = immediate;
            break;
        case 4: //jeq
            if (R[r1] == R[r2]) 
                pc = pc + 1 + immediate;
            break;
        case 6: // xori
            R[r1] = R[r2] ^ immediate; //r2 xor immediate
            break;
        case 10: // movr
            //call mem function to read from mem
            //R[r1] = memory[R[r2] + immediate];
            break;
        case 11: // movm
            //call mem function to write to mem
           // memory[R[r2]+immediate] = R[r1];
            break;
        otherwise:
        printf("wrong opcode in exec method type i\n");
        break;
            } 
void execute_instruction(uint32_t opcode, uint32_t address){
    // j type instructions
    if(opcode == 7) //jmp
        pc = (pc & 0xF0000000) | address;  //PC[31:28] || address;
        //activte IF
    else
        printf("wrong opcode in exec method type j\n");       
                 }


int main(int argc, char const *argv[])
{
    sem_int(&mutex, 0, 1);

    return 0;
}
