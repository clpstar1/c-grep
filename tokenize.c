#include <stdio.h>
#include <stdlib.h>

/* PATTERN = CIRC -?> (CHAR | SLASH | GROUP | ALT) <?-QUANT <?-PATTERN <?-DOLLAR 
 * CHAR = const 
 * GROUP = [ -> CHAR
 * */

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "assertion failed: %s\n", #cond);                        \
    }                                                                          \
  } while (0)

typedef enum token_type {
  CHAR,
  CCLASS,
  GROUP,
} token_type;

typedef struct token {
  token_type ty;
  union {
    char letter;
  } v;
  struct token *next;
} token;

token *mk_char_token(char letter) {
  token *t = malloc(sizeof(token));
  t->ty = CHAR;
  t->v.letter = letter;
  t->next = NULL;
  return t;
}

void append(token *head, token *t) {
  while(head->next != NULL) {
    head = head->next;
  }
  head->next = t;
}

token *tokenize_pattern(char *s) {
  token *head = NULL;
  while(*s != '\0') {
    if (head == NULL) {
      head = mk_char_token(*s);
    } else {
      append(head, mk_char_token(*s));
    }
    s++;
  }
  return head;
}

int main() {
  char *p = "Hello";
  token *head = tokenize_pattern(p);
  do {
    printf("%c\n", head->v.letter);
  } while ((head = head->next) != NULL);
}
