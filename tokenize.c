#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ASSERT(cond) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, "assertion failed: %s\nFile: %snLine: %d\n", \
              #cond, __FILE__, __LINE__); \
      abort(); \
    } \
  } while(0) \

typedef enum quantifier {
  Single,
  Plus,
  Star
} quanitifer; 

typedef struct token {
  char * val; 
  quanitifer quant;
} token;

typedef struct token_array {
  token * t;
  int length;
} token_array;

token_array * tokenize(char * pattern) {
  // "abc\0";
  token * t = malloc(sizeof(token) * (strlen(pattern)));
  int t_index = 0; 
  int p_index = 0;
  while (pattern[p_index] != '\0') {
    char * c = malloc(sizeof(char) + 1);
    c[0] = pattern[p_index];
    c[1] = '\0';
    t[t_index].val = c;
    t[t_index].quant = Single;
    t_index++;
    p_index++;
  }
  token_array * arr = malloc(sizeof(token_array));
  arr->t = t;
  arr->length = p_index;
  return arr;
}

int main(int argc, char * argv[]) {
  token_array * arr = tokenize("a");
  ASSERT(strcmp(arr->t[0].val, "a") == 0);

  arr = tokenize("abc");
  for (int i = 0; i < arr->length; i++) {
    token t = arr->t[i];
    printf("val = %s, quant = %d\n", t.val, t.quant);
  }

}
