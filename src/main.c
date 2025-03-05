#include "./headers/main.h"
#include <stdint.h>
#include <stdio.h>

long file_size = 0;
Var *var_l = NULL;
Instruction *instruction_l = NULL;
uint8_t start_mem = 0x00;

// TODO: improve the char to int in cases where the asm has hex values with
// letters
// TODO: remove duplicated code to functions
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
      char line[6];
      do {
        line[0] = bytes[i];
        line[1] = bytes[i + 1];
        line[2] = bytes[i + 2];
        if (bytes[i + 3] != '\n') {
          line[3] = bytes[i + 3];
          line[4] = bytes[i + 4];
          line[5] = '\0';
        } else {
          line[3] = '\0';
        }

        if (needs_two_bytes(line) == -1)
          break;

        uint8_t ntb = needs_two_bytes(line);

        char name[4];
        name[0] = line[0];
        name[1] = line[1];
        name[2] = line[2];
        name[3] = '\0';

        if (ntb == 1) {
          instruction_l = create_instruction_node(name, ' ');
          start_mem += 0x08;
        } else if (ntb == 2) {
          instruction_l = create_instruction_node(name, line[4]);
          start_mem += 0x0F;
        }

        while (bytes[i] != '\n') {
          i++;
        }

        i++;
      } while (needs_two_bytes(line) == 1 || needs_two_bytes(line) == 2);

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

            start_mem = atoi(v);
            printf("Start memory: %x\n", start_mem);
          }

          while (bytes[i] != '\n') {
            i++;
          }

          code_scope = true;
        }
        break;
      default:
        printf("%c", bytes[i]);
        break;
      }
    }
  }
  printf("Variables: \n");
  print_var_list(var_l);
  printf("\n");
  printf("Instructions: \n");
  print_instruction_list(instruction_l);
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
  new->mem_addr = 0x00;
  new->name = name;
  new->value = value;

  if (var_l != NULL) {
    new->next = var_l;
  } else {
    new->next = NULL;
  }

  return new;
}

void print_var_list(Var *l) {

  Var *temp = l;

  while (temp != NULL) {
    printf("{memory_addr: %x | name: %c | value: %d | next: %x} %s ",
           temp->mem_addr, temp->name, temp->value,
           (temp->next != NULL ? temp->next->mem_addr : 0x00),
           (temp->next != NULL ? "->" : ""));

    temp = temp->next;
  }

  printf("\n");
}

Instruction *create_instruction_node(char *name, char var_name) {

  Instruction *new = (Instruction *)malloc(sizeof(Instruction));
  new->mem_addr = start_mem;
  strcpy(new->name, name);
  new->var_name = var_name;

  if (instruction_l != NULL) {
    new->next = instruction_l;
  } else {
    new->next = NULL;
  }

  return new;
}

void print_instruction_list(Instruction *l) {

  Instruction *temp = l;

  while (temp != NULL) {
    printf("{memory_addr: %x | name: %s | value: %d | var_name: %c | next: %x} "
           "%s ",
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

  regcomp(&regex_two_bytes, "^(STA|LDA|ADD|OR|AND|JMP|JN|JZ)\\s+[A-Z]$",
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
