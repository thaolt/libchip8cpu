#ifndef CHIP8CPU_H_
#define CHIP8CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define C8E_OK 0
    
typedef struct chip8cpu chip8cpu;

struct chip8cpu
{
    uint8_t   memory[4096];
    uint8_t   gfx[64 * 32];
    uint8_t   V[16];        /* registers from V0 .. VF */

    uint16_t  I;            /* index register */
    uint16_t  pc;           /* program counter */
    uint16_t  opcode;

    uint8_t   delay_timer;
    uint8_t   sound_timer;

    uint16_t  stack[16];
    uint16_t  sp;           /* stack pointer */

    /* opcode handling functions, can be overrided */
    int  (*opcode_handlers[0x10])(chip8cpu *);
    
    /* API callbacks */
    void    (*draw)(chip8cpu *);
    bool    (*keystate)(chip8cpu *, uint8_t);
    void    (*beep)(chip8cpu *);
    
    /* has default callbacks */
    void    (*log)(chip8cpu *, int log_level, const char *file, int line, const char* message);
    uint8_t (*delay_get)(chip8cpu *);
    void    (*delay_set)(chip8cpu *, uint8_t);
    uint8_t (*sound_get)(chip8cpu *);
    void    (*sound_set)(chip8cpu *, uint8_t);
        
    /* random number generator */
    int (*rand)(void);
};


chip8cpu* chip8cpu_new(void);
void chip8cpu_copy(chip8cpu *cpu, chip8cpu *cpy);
void chip8cpu_free(chip8cpu*);
int chip8cpu_load_code(chip8cpu *cpu, uint8_t* code, long code_size);
int chip8cpu_load_rom(chip8cpu* cpu, const char* filename);
void chip8cpu_exec_cycle(chip8cpu *cpu);
void chip8cpu_timer_tick(chip8cpu *cpu);


#ifdef __cplusplus
}
#endif

#endif /* CHIP8CPU_H_ */
