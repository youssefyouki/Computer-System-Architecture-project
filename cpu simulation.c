#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
const uint32_t R0 = 0;

bool IF;
bool ID;
bool EX;
bool MEM;
bool WB;

sem_t mutex;


int clock = 0;


uint32_t fetch_instruction(uint32_t pc){
    sem_wait(&mutex);
    uint32_t temp = memory[pc];
    pc ++;
    IF = true;
    sem_post(&mutex);
    return temp;
}

void decode_instruction(uint32_t instruction){
    if (IF){
        IF = false;
        // Decode the instruction here
        ID = true;
    }
    
}


int main(int argc, char const *argv[])
{
    sem_int(&mutex, 0, 1);

    return 0;
}
