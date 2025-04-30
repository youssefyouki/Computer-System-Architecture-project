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

        // Decode the instruction here
        
    
}


int main(int argc, char const *argv[])
{
    sem_int(&mutex, 0, 1);

    return 0;
}
