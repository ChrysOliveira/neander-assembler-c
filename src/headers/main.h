#ifndef __MAIN_H__
#define __MAIN_H__
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Mnemonics {
  NOP = 0x00,
  STA = 0x10,
  LDA = 0x20,
  ADD = 0x30,
  OR = 0x40,
  AND = 0x50,
  NOT = 0x60,
  JMP = 0x80,
  JN = 0x90,
  JZ = 0xA0,
  HLT = 0xF0
};

typedef struct var {
  uint8_t mem_addr;
  char name;
  uint8_t value;
  struct var *next;
} Var;

typedef struct instruction {
  uint8_t mem_addr;
  char name[4];
  uint8_t value;
  char var_name;
  struct instruction *next;
} Instruction;

void print_file(uint8_t *);
bool is_letter(char c);
// bool is_number(char c);

Var *create_var_node(char, uint8_t);
void print_var_list(Var *);

Instruction *create_instruction_node(char *, char);
Instruction *create_instruction_node_addr(char *, uint8_t);
void print_instruction_list(Instruction *);

int needs_two_bytes(const char *);

void update_var_mem();
uint8_t find_var_value(char);
void update_instruction_value();

void generate_binary_file(const char *);
uint8_t get_opcode_enum(const char *);

uint8_t find_var_mem(char);

uint8_t find_line_addr(Instruction *, int);

uint8_t extract_line_addr(char *);
#endif // !__MAIN_H__
