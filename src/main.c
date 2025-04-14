#include "./headers/main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

long file_size = 0;
Var *var_l = NULL;
Instruction *instruction_l = NULL;
uint8_t start_mem = 0x00;

// TODO: change read var value to hex instead of int
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *file_name = argv[1];

  FILE *file = fopen(file_name, "rb");
  if (!file) {
    perror("Error opening file");
    return EXIT_FAILURE;
  }

  // file size
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  rewind(file);

  if (file_size <= 0) {
    fprintf(stderr, "Error: file is empty or size is invalid\n");
    fclose(file);
    return EXIT_FAILURE;
  }

  uint8_t *bytes = malloc(file_size);
  if (!bytes) {
    perror("Memory allocation failed");
    fclose(file);
    return EXIT_FAILURE;
  }

  size_t read_bytes = fread(bytes, 1, file_size, file);
  if (read_bytes != (size_t)file_size) {
    fprintf(stderr, "Error reading file\n");
    free(bytes);
    fclose(file);
    return EXIT_FAILURE;
  }

  fclose(file);

  // Process

  bool jmp_comment = false;
  bool data_scope = false;
  bool code_scope = false;
  char scope[5];
  for (size_t i = 0; i < file_size; i++) {

    if (jmp_comment) {
      while (bytes[i + 1] != '\n') {
        i++;
      }

      jmp_comment = false;

    } else if (data_scope) {
      while (bytes[i + 1] != '.') {

        if (bytes[i] == 'D' && bytes[i + 1] == 'B') {
          i += 3;
        }

        if (is_letter(bytes[i])) {
          char v[2];
          v[0] = bytes[i + 5];
          v[1] = bytes[i + 6];

          /* uint8_t n_v = strtol(v, NULL, 16); */
          uint8_t n_v = atoi(v);

          var_l = create_var_node(bytes[i], n_v);

          while (bytes[i] != '\n') {
            i++;
          }
        }
        i++;
      }

      data_scope = false;

    } else if (code_scope) {
      char line[16];
      int ntb = 0;

      do {

        int j = i;
        int k = 0;
        bool value = false;

        while (bytes[j] != '\n' && k < (sizeof(line) - 1)) {
          if (value && bytes[j] == ' ') {
            break;
          } else if (bytes[j] == ' ') {
            value = true;
          }

          line[k] = bytes[j];
          /* printf("Lendo: \"%c\" da linha: \"%s\"\n", bytes[j], line); */
          k++;
          j = i + k;
        }

        line[k] = '\0';

        ntb = needs_two_bytes(line);

        if (ntb == -1)
          break;

        char name[4];
        k = 0;
        while (line[k] != ' ' && line[k] != '\n' && k < 3) {
          name[k] = line[k];
          k++;
        }
        name[k] = '\0';

        /* printf("LINE:%s BYTES:%d\n", line, ntb); */
        /* printf("NAME:%s\n", name); */

        if (strcmp(name, "JN") == 0 || strcmp(name, "JZ") == 0 ||
            strcmp(name, "JMP") == 0) {
          printf("Creating %s with value %d\n", name, extract_line_addr(line));
          instruction_l =
              create_instruction_node_addr(name, extract_line_addr(line));
          start_mem += 0x02;
        } else if (ntb == 1) {
          instruction_l = create_instruction_node(name, ' ');
          start_mem += 0x01;
        } else if (ntb == 2) {
          instruction_l = create_instruction_node(name, line[4]);
          start_mem += 0x02;
        }

        while (bytes[i] != '\n') {
          i++;
        }

        i++;
      } while (ntb == 1 || ntb == 2);

      code_scope = false;
    } else {
      switch (bytes[i]) {
      case ';':
        jmp_comment = true;
        continue;
        break;

      case '.':
        for (int j = 0; j < 4; j++) {
          scope[j] = bytes[i + j + 1];
        }
        scope[4] = '\0';

        if (strcmp(scope, "DATA") == 0) {
          i += 5;
          data_scope = true;
        } else if (strcmp(scope, "CODE") == 0) {
          i += 6;

          char org[4];
          org[0] = bytes[i + 1]; // +1 to skip the dot in .ORG
          org[1] = bytes[i + 2];
          org[2] = bytes[i + 3];
          org[3] = '\0';

          if (strcmp(org, "ORG") == 0) {
            char v[2];
            v[0] = bytes[i + 5];
            v[1] = bytes[i + 6];

            /* start_mem = strtol(v, NULL, 16); */
            start_mem = atoi(v);
            printf("START MEMORY (hex): %x\n\n", start_mem);
          }

          while (bytes[i] != '\n') {
            i++;
          }

          code_scope = true;
        }
        break;
      default:
        // file characters not filtered
        /* printf("%c", bytes[i]); */
        break;
      }
    }
  }

  update_var_mem();
  update_instruction_value();
  printf("VARIABLES: \n");
  print_var_list(var_l);
  printf("\n");
  printf("INSTRUCTIONS: \n");
  print_instruction_list(instruction_l);
  generate_binary_file("binary.mem");
  free(bytes);

  return EXIT_SUCCESS;
}

void print_file(uint8_t *bytes) {
  for (size_t i = 0; i < file_size; i++) {
    printf("%c", bytes[i]);
  }
  printf("\n");
}

bool is_letter(char c) {
  regex_t regex;
  int reti;
  char pattern[11];

  sprintf(pattern, "^[a-zA-Z]$");

  reti = regcomp(&regex, pattern, REG_EXTENDED);
  if (reti) {
    fprintf(stderr, "Could not compile regex\n");
    exit(1);
  }

  char str[2] = {c, '\0'};

  reti = regexec(&regex, str, 0, NULL, 0);
  regfree(&regex);

  // Return 1 if it's a letter, 0 otherwise
  return (reti == 0);
}

/* bool is_number(char c) { */
/*   regex_t regex; */
/*   int reti; */
/*   char pattern[10]; */
/**/
/*   sprintf(pattern, "^[0-9]$"); */
/**/
/*   reti = regcomp(&regex, pattern, REG_EXTENDED); */
/*   if (reti) { */
/*     fprintf(stderr, "Could not compile regex\n"); */
/*     exit(1); */
/*   } */
/**/
/*   char str[2] = {c, '\0'}; */
/**/
/*   reti = regexec(&regex, str, 0, NULL, 0); */
/*   regfree(&regex); */
/**/
/*   // Return 1 if it's a number, 0 otherwise */
/*   return (reti == 0); */
/* } */

Var *create_var_node(char name, uint8_t value) {
  Var *new = (Var *)malloc(sizeof(Var));
  if (!new) {
    perror("Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  new->mem_addr = 0x00;
  new->name = name;
  new->value = value;
  new->next = NULL;

  if (var_l == NULL) {
    var_l = new;
  } else {
    Var *temp = var_l;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = new;
  }

  return var_l;
}

void print_var_list(Var *l) {

  Var *temp = l;

  while (temp != NULL) {
    printf(
        "{memory_addr: %02x | name: %c | value(hex): %02x | next: %02x} %s\n",
        temp->mem_addr, temp->name, temp->value,
        (temp->next != NULL ? temp->next->mem_addr : 0x00),
        (temp->next != NULL ? "->" : ""));

    temp = temp->next;
  }

  printf("\n");
}

Instruction *create_instruction_node(char *name, char var_name) {
  Instruction *new = (Instruction *)malloc(sizeof(Instruction));
  if (!new) {
    perror("Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  new->mem_addr = start_mem;
  strcpy(new->name, name);
  new->var_name = var_name;
  new->next = NULL;

  if (instruction_l == NULL) {
    instruction_l = new;
  } else {
    Instruction *temp = instruction_l;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = new;
  }

  return instruction_l;
}

Instruction *create_instruction_node_addr(char *name, uint8_t addr) {
  Instruction *new = (Instruction *)malloc(sizeof(Instruction));
  if (!new) {
    perror("Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  new->mem_addr = start_mem;
  strcpy(new->name, name);
  new->value = addr;
  new->var_name = ' ';
  new->next = NULL;

  if (instruction_l == NULL) {
    instruction_l = new;
  } else {
    Instruction *temp = instruction_l;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = new;
  }

  return instruction_l;
}

void print_instruction_list(Instruction *l) {

  Instruction *temp = l;

  while (temp != NULL) {
    printf(
        "{memory_addr: %02x | name: %03s | value(hex): %02x | var_name: %c | "
        "next: %02x} "
        "%s \n",
        temp->mem_addr, temp->name, temp->value, temp->var_name,
        (temp->next != NULL ? temp->next->mem_addr : 0x00),
        (temp->next != NULL ? "->" : ""));

    temp = temp->next;
  }

  printf("\n");
}

int needs_two_bytes(const char *line) {
  regex_t regex_two_bytes, regex_one_byte;
  int result;

  regcomp(&regex_two_bytes,
          "^(STA|LDA|ADD|OR|AND)\\s+[A-Z]$|^(JMP|JN|JZ)\\s+[0-9]+$",
          REG_EXTENDED);

  regcomp(&regex_one_byte, "^(NOP|NOT|HLT)$", REG_EXTENDED);

  result = regexec(&regex_two_bytes, line, 0, NULL, 0);
  if (result == 0) {
    regfree(&regex_two_bytes);
    regfree(&regex_one_byte);
    return 2; // Needs 2 bytes
  }

  result = regexec(&regex_one_byte, line, 0, NULL, 0);
  if (result == 0) {
    regfree(&regex_two_bytes);
    regfree(&regex_one_byte);
    return 1; // Needs 1 byte
  }

  // Free regex memory
  regfree(&regex_two_bytes);
  regfree(&regex_one_byte);

  return -1; // Unknown instruction
}

void update_var_mem() {
  Var *temp = var_l;
  while (temp != NULL) {
    temp->mem_addr = start_mem;
    start_mem += 0x01;

    temp = temp->next;
  }
}

uint8_t find_var_value(char n) {
  Var *temp = var_l;
  while (temp != NULL) {
    if (temp->name == n) {
      return temp->value;
    }
    temp = temp->next;
  }
  return 0;
}

void update_instruction_value() {
  Instruction *temp = instruction_l;

  while (temp != NULL) {

    if (strcmp(temp->name, "JN") == 0 || strcmp(temp->name, "JZ") == 0 ||
        strcmp(temp->name, "JMP") == 0) {
      temp = temp->next;
      continue;
    }

    if (temp->var_name == ' ') {
      temp->value = 0x00;
      temp = temp->next;
      continue;
    }
    temp->value = find_var_value(temp->var_name);
    temp = temp->next;
  }
}

void generate_binary_file(const char *output_filename) {
  FILE *bin_file = fopen(output_filename, "wb");
  if (!bin_file) {
    perror("Error creating binary file");
    exit(EXIT_FAILURE);
  }

  uint8_t sign[] = {0x03, 0x4E, 0x44, 0x52};
  uint8_t white_space = 0x00;
  fwrite(sign, sizeof(uint8_t), 4, bin_file);

  size_t byte_count = 4;

  Instruction *temp_i = instruction_l;

  // this loop is used to put N white spaces (0x00) before the .ORG value
  for (int i = 0; i < temp_i->mem_addr; i++) {
    fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
    fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
  }

  while (temp_i != NULL) {
    uint8_t opcode = (uint8_t)get_opcode_enum(temp_i->name);

    uint8_t operand;

    if (opcode == JMP || opcode == JN || opcode == JZ) {
      operand = find_line_addr(instruction_l, temp_i->value);
      printf("Finding line addr of value %d result: %d\n", temp_i->value,
             operand);
    } else {
      operand =
          (temp_i->var_name == ' ') ? 0x00 : find_var_mem(temp_i->var_name);
    }

    fwrite(&opcode, sizeof(uint8_t), 1, bin_file);
    fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
    byte_count += 2;

    if (operand != 0x00) {
      fwrite(&operand, sizeof(uint8_t), 1, bin_file);
      fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
      byte_count += 2;
    } else if (opcode == JMP || opcode == JN || opcode == JZ) {
      fwrite(&operand, sizeof(uint8_t), 1, bin_file);
      fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
      byte_count += 2;
    }

    temp_i = temp_i->next;
  }

  Var *temp_l = var_l;
  while (temp_l != NULL) {
    uint8_t value = (uint8_t)find_var_value(temp_l->name);
    fwrite(&value, sizeof(uint8_t), 1, bin_file);
    fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
    byte_count += 2;

    temp_l = temp_l->next;
  }

  while (byte_count < 516) {
    fwrite(&white_space, sizeof(uint8_t), 1, bin_file);
    byte_count++;
  }

  fclose(bin_file);
}

uint8_t get_opcode_enum(const char *mnemonic) {
  if (strcmp(mnemonic, "NOP") == 0)
    return NOP;
  if (strcmp(mnemonic, "STA") == 0)
    return STA;
  if (strcmp(mnemonic, "LDA") == 0)
    return LDA;
  if (strcmp(mnemonic, "ADD") == 0)
    return ADD;
  if (strcmp(mnemonic, "OR") == 0)
    return OR;
  if (strcmp(mnemonic, "AND") == 0)
    return AND;
  if (strcmp(mnemonic, "NOT") == 0)
    return NOT;
  if (strcmp(mnemonic, "JMP") == 0)
    return JMP;
  if (strcmp(mnemonic, "JN") == 0)
    return JN;
  if (strcmp(mnemonic, "JZ") == 0)
    return JZ;
  if (strcmp(mnemonic, "HLT") == 0)
    return HLT;
  return 0xFF;
}

uint8_t find_var_mem(char var_name) {
  Var *temp = var_l;
  while (temp) {
    if (temp->name == var_name) {
      return temp->mem_addr;
    }
    temp = temp->next;
  }
  return 0x00;
}

uint8_t find_line_addr(Instruction *instruction_l, int line) {

  Instruction *temp = instruction_l;

  int i = 0;
  while (i < line) {
    temp = temp->next;
    i++;
  }

  return temp->mem_addr;
}

uint8_t extract_line_addr(char *line) {
  char *space_ptr = strchr(line, ' ');

  if (space_ptr != NULL && *(space_ptr + 1) != '\0') {
    char *value_s = strdup(space_ptr + 1);
    uint8_t value = atoi(value_s);
    free(value_s);
    return value;
  } else {
    printf("No number found after space.\n");
    return 0x00;
  }
}
