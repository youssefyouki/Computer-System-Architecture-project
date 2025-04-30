#include <stdint.h>
#include <stdbool.h>

uint32_t memory[2048];
uint32_t R[31];
uint32_t pc;
uint32_t R0 = 0;

bool IF;
bool ID;
bool EX;
bool MEM;
bool WB;

int clock = 0;