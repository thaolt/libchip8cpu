#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <time.h>
#include "chip8cpu.h"

/* Logging */
enum { C8E_LOG_DEBUG, C8E_LOG_INFO, C8E_LOG_WARN, C8E_LOG_ERR, C8E_LOG_FATAL };
#define _chip8cpu_log_debug(cpu, ...)   _chip8cpu_log_forward(cpu, C8E_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define _chip8cpu_log_info(cpu, ...)    _chip8cpu_log_forward(cpu, C8E_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define _chip8cpu_log_warn(cpu, ...)    _chip8cpu_log_forward(cpu, C8E_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define _chip8cpu_log_error(cpu, ...)   _chip8cpu_log_forward(cpu, C8E_LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define _chip8cpu_log_fatal(cpu, ...)   _chip8cpu_log_forward(cpu, C8E_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

static void _chip8cpu_log_forward(chip8cpu* cpu, uint8_t level, const char *file, int line, const char *fmt, ...)
{
    char message[255] = {0};

    va_list args;

    va_start(args, fmt);
    vsnprintf(message, 255, fmt, args);
    va_end(args);

    cpu->log(cpu, level, file, line, message);
}

static void _chip8cpu_dummy_logger(chip8cpu *cpu, int log_level, const char *file, int line, const char* message) {
    (void)cpu; (void)log_level; (void)file; (void)line; (void)message;
}

static int _chip8cpu_default_rand(void) {
    return rand();
}

/* opcode handling prototypes */
static int _chip8cpu_opcode_handler_0(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_1(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_2(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_3(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_4(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_5(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_6(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_7(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_8(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_9(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_A(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_B(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_C(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_D(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_E(chip8cpu* cpu);
static int _chip8cpu_opcode_handler_F(chip8cpu* cpu);

static uint8_t  _chip8cpu_timer_delay_get(chip8cpu* cpu);
static void     _chip8cpu_timer_delay_set(chip8cpu* cpu, uint8_t val);

static uint8_t  _chip8cpu_timer_sound_get(chip8cpu* cpu);
static void     _chip8cpu_timer_sound_set(chip8cpu* cpu, uint8_t val);

/* Default font set */
static uint8_t chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
  0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
  0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
  0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
  0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
  0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
  0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
  0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
  0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
  0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
  0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
  0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
  0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
  0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
  0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
  0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};


/* chip8cpu initialization */
chip8cpu *chip8cpu_new(void)
{
    chip8cpu* cpu = malloc(sizeof (chip8cpu));

    cpu->pc     = 0x200;  /* Program counter starts at 0x200 */
    cpu->opcode = 0;      /* Reset current opcode */
    cpu->I      = 0;      /* Reset index register */
    cpu->sp     = 0;      /* Reset stack pointer */

    memset(cpu->gfx, 0, 64 * 32);      /* Clear display */
    memset(cpu->stack, 0, 16 * sizeof(uint16_t));         /* Clear stack */
    memset(cpu->V, 0, 16);             /* Clear registers V0-VF */
    memset(cpu->memory, 0, 4096);      /* Clear memory */

    /* Load fontset */
    for(int i = 0; i < 80; ++i)
        cpu->memory[i] = chip8_fontset[i];

    /* Reset timers */
    cpu->delay_timer = 0;
    cpu->sound_timer = 0;

    cpu->draw = 0;
    cpu->keystate = 0;
    cpu->beep = 0;
    cpu->rand = &_chip8cpu_default_rand;
    cpu->log = &_chip8cpu_dummy_logger;
    cpu->delay_get = &_chip8cpu_timer_delay_get;
    cpu->delay_set = &_chip8cpu_timer_delay_set;
    cpu->sound_get = &_chip8cpu_timer_sound_get;
    cpu->sound_set = &_chip8cpu_timer_sound_set;

    cpu->opcode_handlers[0x0] = &_chip8cpu_opcode_handler_0;
    cpu->opcode_handlers[0x1] = &_chip8cpu_opcode_handler_1;
    cpu->opcode_handlers[0x2] = &_chip8cpu_opcode_handler_2;
    cpu->opcode_handlers[0x3] = &_chip8cpu_opcode_handler_3;
    cpu->opcode_handlers[0x4] = &_chip8cpu_opcode_handler_4;
    cpu->opcode_handlers[0x5] = &_chip8cpu_opcode_handler_5;
    cpu->opcode_handlers[0x6] = &_chip8cpu_opcode_handler_6;
    cpu->opcode_handlers[0x7] = &_chip8cpu_opcode_handler_7;
    cpu->opcode_handlers[0x8] = &_chip8cpu_opcode_handler_8;
    cpu->opcode_handlers[0x9] = &_chip8cpu_opcode_handler_9;
    cpu->opcode_handlers[0xA] = &_chip8cpu_opcode_handler_A;
    cpu->opcode_handlers[0xB] = &_chip8cpu_opcode_handler_B;
    cpu->opcode_handlers[0xC] = &_chip8cpu_opcode_handler_C;
    cpu->opcode_handlers[0xD] = &_chip8cpu_opcode_handler_D;
    cpu->opcode_handlers[0xE] = &_chip8cpu_opcode_handler_E;
    cpu->opcode_handlers[0xF] = &_chip8cpu_opcode_handler_F;

    return cpu;
}

void chip8cpu_copy(chip8cpu *cpu, chip8cpu *cpy)
{
    if (cpy != 0) {
        chip8cpu_free(cpy);
    }

    cpy = malloc(sizeof (chip8cpu));

    memcpy(cpy->memory, cpu->memory, 4096);
    memcpy(cpy->gfx, cpu->gfx, 2048);
    memcpy(cpy->V, cpu->V, 16);
    memcpy(cpy->stack, cpu->stack, 16);

    cpy->sp = cpu->sp;
    cpy->I = cpu->I;
    cpy->pc = cpu->pc;
    
    cpy->delay_timer = cpu->delay_get(cpu);
    cpy->sound_timer = cpu->sound_get(cpu);
    
    /* copy function pointers */
    cpy->opcode = cpu->opcode;
    cpy->beep = cpu->beep;
    cpy->draw = cpu->draw;
    cpy->keystate = cpu->keystate;
    cpy->log = cpu->log;
    cpy->sound_get = cpu->sound_get;
    cpy->sound_set = cpu->sound_set;
    cpy->delay_set = cpu->delay_set;
    cpy->delay_get = cpu->delay_get;
}

void chip8cpu_free(chip8cpu *cpu)
{
    free(cpu);
}


/* ******************** Opcode handling implementation ******************** */
int _chip8cpu_opcode_handler_0(chip8cpu* cpu) {
    switch (cpu->opcode) {
    case 0x00E0: /* clear screen */
        memset(cpu->gfx, 0, 64*32);
        cpu->pc += 2;
        cpu->draw(cpu);
        break;

    case 0x00EE: /* subroutine return */
        cpu->pc = cpu->stack[--cpu->sp & 0xF] + 2;
        break;

    default: /* 0NNN: call program at NNN address */
        _chip8cpu_log_error(cpu, "OpCode 0NNN is not implemented");
        break;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_1(chip8cpu* cpu) {
     /* 1NNN: absolute jump */
    cpu->pc = cpu->opcode & 0xFFF;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_2(chip8cpu* cpu) {
    /* 2NNN: call subroutine */
    cpu->stack[cpu->sp] = cpu->pc;
    ++cpu->sp;
    cpu->pc = cpu->opcode & 0x0FFF;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_3(chip8cpu* cpu) {
    /* 3XNN: Skips the next instruction if VX equals NN */
    if (cpu->V[(cpu->opcode & 0x0F00) >> 8] == (cpu->opcode & 0x00FF)) {
        cpu->pc += 4;
    } else {
        cpu->pc += 2;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_4(chip8cpu* cpu) {
    /* 4XNN: Skips the next instruction if VX doesn't equal NN */
    if (cpu->V[(cpu->opcode & 0x0F00) >> 8] != (cpu->opcode & 0x00FF)) {
        cpu->pc += 4;
    } else {
        cpu->pc += 2;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_5(chip8cpu* cpu) {
    /* 5XY0: Skips the next instruction if VX equals VY */
    if (cpu->V[(cpu->opcode & 0x0F00) >> 8] == cpu->V[(cpu->opcode & 0x00F0) >> 4]) {
        cpu->pc += 4;
    } else {
        cpu->pc += 2;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_6(chip8cpu* cpu) {
    /* 6XNN: Sets VX to NN */
    cpu->V[(cpu->opcode & 0x0F00) >> 8] = (cpu->opcode & 0x00FF);
    cpu->pc += 2;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_7(chip8cpu* cpu) {
    /* 7XNN: Adds NN to VX */
    cpu->V[(cpu->opcode & 0x0F00) >> 8] += (cpu->opcode & 0x00FF);
    cpu->pc += 2;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_8(chip8cpu* cpu) {
    switch (cpu->opcode & 0x000F) {
    case 0x0000: /* 8XY0: Vx = Vy  */
        cpu->V[(cpu->opcode & 0x0F00) >> 8] = cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0001: /* 8XY1: Vx = Vx | Vy */
        cpu->V[(cpu->opcode & 0x0F00) >> 8] |= cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0002: /* 8XY2: Vx = Vx & Vy*/
        cpu->V[(cpu->opcode & 0x0F00) >> 8] &= cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0003: /* 8XY3: Vx = Vx XOR Vy */
        cpu->V[(cpu->opcode & 0x0F00) >> 8] ^= cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0004: /* 8XY4: Vx += Vy; Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't */
        if(cpu->V[(cpu->opcode & 0x00F0) >> 4] > (0xFF - cpu->V[(cpu->opcode & 0x0F00) >> 8]))
            cpu->V[0xF] = 1; /* carry over */
        else
            cpu->V[0xF] = 0;
        cpu->V[(cpu->opcode & 0x0F00) >> 8] += cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0005: /* 8XY5: Vx -= Vy; VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't */
        if (cpu->V[(cpu->opcode & 0x00F0) >> 4] > cpu->V[(cpu->opcode & 0x0F00) >> 8]) {
            cpu->V[0xF] = 0;
        } else {
            cpu->V[0xF] = 1;
        }
        cpu->V[(cpu->opcode & 0x0F00) >> 8] -= cpu->V[(cpu->opcode & 0x00F0) >> 4];
        cpu->pc += 2;
        break;
    case 0x0006: /* 8XY6: Vx>>=1; Stores the least significant bit of VX in VF and then shifts VX to the right by 1 */
        cpu->V[0xF] = cpu->V[(cpu->opcode & 0x0F00) >> 8] & 0x1;
        cpu->V[(cpu->opcode & 0x0F00) >> 8] >>= 1;
        cpu->pc += 2;
        break;
    case 0x0007: /* 8XY7: Vx=Vy-Vx; Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't */
        if (cpu->V[(cpu->opcode & 0x0F00) >> 8] > (int)cpu->V[(cpu->opcode & 0x00F0) >> 4]) {
            cpu->V[0xF] = 0;
        } else {
            cpu->V[0xF] = 1;
        }
        cpu->V[(cpu->opcode & 0x0F00) >> 8] = cpu->V[(cpu->opcode & 0x00F0) >> 4] - cpu->V[(cpu->opcode & 0x0F00) >> 8];
        cpu->pc += 2;
        break;
    case 0x000E: /* 8XYE: Vx<<=1; Stores the most significant bit of VX in VF and then shifts VX to the left by 1 */
        cpu->V[0xF] = cpu->V[(cpu->opcode & 0x0F00) >> 8] >> 7;
        cpu->V[(cpu->opcode & 0x0F00) >> 8] <<= 1;
        cpu->pc += 2;
        break;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_9(chip8cpu* cpu) {
    /* 9XY0: Skips the next instruction if VX doesn't equal VY */
    if(cpu->V[(cpu->opcode & 0x0F00) >> 8] != cpu->V[(cpu->opcode & 0x00F0) >> 4]) {
        cpu->pc += 4;
    } else {
        cpu->pc += 2;
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_A(chip8cpu* cpu) {
    /* ANNN: Sets I to the address NNN */
    cpu->I = cpu->opcode & 0x0FFF;
    cpu->pc += 2;
    return C8E_OK;
}
int _chip8cpu_opcode_handler_B(chip8cpu* cpu) {
    /* BNNN: Jumps to the address NNN plus V0 */
    cpu->pc = (cpu->opcode & 0x0FFF) + cpu->V[0];
    return C8E_OK;
}

int _chip8cpu_opcode_handler_C(chip8cpu* cpu) {
    /* CXNN: Vx=rand() & NN */
    cpu->V[(cpu->opcode & 0x0F00) >> 8] = (cpu->rand() % (0xFF + 1)) & (cpu->opcode & 0x00FF);
    cpu->pc += 2;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_D(chip8cpu* cpu) {
    /* DXYN: draw(Vx,Vy,N); draw at X,Y width 8, height N sprite from I register */
    uint8_t xo = cpu->V[(cpu->opcode & 0x0F00) >> 8]; /* x origin */
    uint8_t yo = cpu->V[(cpu->opcode & 0x00F0) >> 4];
    uint8_t height = cpu->opcode & 0x000F;
    uint8_t sprite[0x10] = {0};

    memcpy(sprite, cpu->memory + (cpu->I * sizeof (uint8_t)), height);

    cpu->V[0xF] = 0;
    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            int dx = (xo + x) % 64; /* display x or dest x*/
            int dy = (yo + y) % 32;
            if ((sprite[y] & (0x80 >> x)) != 0) { /* 0x80 -> 10000000b */
                if (!cpu->V[0xF] && cpu->gfx[(dx + (dy * 64))])
                    cpu->V[0xF] = 1;
                cpu->gfx[dx + (dy * 64)] ^= 1;
            }
        }
    }

    cpu->draw(cpu);
    cpu->pc += 2;
    return C8E_OK;
}

int _chip8cpu_opcode_handler_E(chip8cpu* cpu) {
    switch (cpu->opcode & 0x00FF) {
    case 0x009E: /* EX9E: Skips the next instruction if the key stored in VX is pressed */
        if(cpu->keystate(cpu, cpu->V[(cpu->opcode & 0x0F00) >> 8])) {
            cpu->pc += 4;
        } else {
            cpu->pc += 2;
        }
        break;
    case 0x00A1: /* EXA1: Skips the next instruction if the key stored in VX isn't pressed */
        if(!cpu->keystate(cpu, cpu->V[(cpu->opcode & 0x0F00) >> 8])) {
            cpu->pc += 4;
        } else {
            cpu->pc += 2;
        }
        break;
    default:
        _chip8cpu_log_error(cpu, "Unknown opcode: 0x%X\n", cpu->opcode);
    }
    return C8E_OK;
}

int _chip8cpu_opcode_handler_F(chip8cpu* cpu) {
    switch (cpu->opcode & 0x00FF) {
    case 0x0007: /* FX07: Sets VX to the value of the delay timer */
        cpu->V[(cpu->opcode & 0x0F00) >> 8] = _chip8cpu_timer_delay_get(cpu);;
        cpu->pc += 2;
        break;
    case 0x000A: /* FX0A: A key press is awaited, and then stored in VX. (blocking) */
        for (uint8_t i = 0; i < 0x10; i++) {
            if (cpu->keystate(cpu, i)) {
                cpu->V[(cpu->opcode & 0x0F00) >> 8] = i;
                cpu->pc += 2;
                break;
            }
        }
        break;
    case 0x0015: /* FX15: Sets the delay timer to VX */
        _chip8cpu_timer_delay_set(cpu, cpu->V[(cpu->opcode & 0x0F00) >> 8]);
        cpu->pc += 2;
        break;
    case 0x0018: /* FX18: Sets the sound timer to VX */
        _chip8cpu_timer_sound_set(cpu, cpu->V[(cpu->opcode & 0x0F00) >> 8]);
        cpu->pc += 2;
        break;
    case 0x001E: /* FX1E: Add VX to I register */
        cpu->I += cpu->V[(cpu->opcode & 0x0F00) >> 8];
        cpu->pc += 2;
        break;
    case 0x0029: /* FX29: I=sprite_addr[Vx]; Sets I to the location of the sprite for the character in VX */
        cpu->I = cpu->V[(cpu->opcode & 0x0F00) >> 8] * 5;
        cpu->pc += 2;
        break;
    case 0x0033: /* FX33: Store a Binary Coded Decimal (BCD) of register VX to memory started from I */
        cpu->memory[cpu->I]     = cpu->V[(cpu->opcode & 0x0F00) >> 8] / 100;
        cpu->memory[cpu->I + 1] = (cpu->V[(cpu->opcode & 0x0F00) >> 8] / 10) % 10;
        cpu->memory[cpu->I + 2] = cpu->V[(cpu->opcode & 0x0F00) >> 8] % 10;
        cpu->pc += 2;
        break;
    case 0x0055: /* FX55: */
        for (int i = 0; i <= ((cpu->opcode & 0x0F00) >> 8); i++) {
            cpu->memory[cpu->I+i] = cpu->V[i];
        }
        cpu->pc += 2;
        break;
    case 0x0065: /* FX65: */
        for (int i = 0; i <= ((cpu->opcode & 0x0F00) >> 8); i++) {
            cpu->V[i] = cpu->memory[cpu->I + i];
        }
        cpu->pc += 2;
        break;
    default:
        _chip8cpu_log_error(cpu, "Unknown opcode: 0x%X\n", cpu->opcode);
    }
    return C8E_OK;
}
/* ******************** /Opcode handling implementation ******************** */

void chip8cpu_exec_cycle(chip8cpu *cpu)
{
    cpu->opcode = (uint16_t) (cpu->memory[cpu->pc] << 8 | cpu->memory[cpu->pc + 1]);

    cpu->opcode_handlers[(cpu->opcode & 0xF000) >> 12](cpu);
}


int chip8cpu_load_code(chip8cpu *cpu, uint8_t *code, long code_size)
{
    for (int i = 0; i < code_size; ++i)
      cpu->memory[i + 512] = code[i];
    return C8E_OK;
}

void chip8cpu_timer_tick(chip8cpu *cpu)
{
    uint8_t   delay_timer = cpu->delay_get(cpu);
    uint8_t   sound_timer = cpu->sound_get(cpu);
    
    if (delay_timer > 0)
        cpu->delay_set(cpu, delay_timer - 1);

    if (sound_timer > 0) {
        if (sound_timer == 1)
            cpu->beep(cpu);
        cpu->sound_set(cpu, sound_timer - 1);
    }
}

static uint8_t _chip8cpu_timer_delay_get(chip8cpu* cpu)
{
    return cpu->delay_timer;
}

static void _chip8cpu_timer_delay_set(chip8cpu* cpu, uint8_t val)
{
    cpu->delay_timer = val;
}

static uint8_t _chip8cpu_timer_sound_get(chip8cpu* cpu)
{
    return cpu->sound_timer;
}

static void _chip8cpu_timer_sound_set(chip8cpu* cpu, uint8_t val)
{
    cpu->sound_timer = val;
}
