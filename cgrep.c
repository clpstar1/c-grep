#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

#define BUFSZ 1024
#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "assertion failed: %s\n", #cond);                        \
    }                                                                          \
  } while (0)

bool match_start;
bool match_end;
bool did_match;

bool _match(char *s, char *p);

bool match_class(char *s, char *p) {
  bool is_match = false;
  char * pp = p;
  bool negate = *pp == '^';

  if (negate) {
    pp++;
  }
  while (*pp != ']') {
    if (*pp == '\0') {
      printf("ERROR: unclosed character group");
      exit(1);
    }
    if (*s == *pp) {
      is_match = true;
    }
    pp++;
  }
  if (negate) {
    is_match = !is_match;
  }
  if (!is_match) {
    if (did_match) return false;
    if (match_start) return false;
    return _match(s+1, p-1);
  }
  else {
    did_match = true;
    p = pp; 
    return _match(s+1, p+1);
  }
}

bool match_escape(char *s, char *p) {
  char *next = p+1;
  switch(*next) {
    case '*':
      while(*s != '\0' && *s == *p) {
        if (_match(s, next+1)) return true;
        s++;
      }
      return _match(s, next+1);
    case '+':
      if (*s != *p) {
        return _match(s+1, p);
      }
      s++;
      while (*s != '\0' && *s == *p) {
        if (_match(s, next+1)) return true;
        s++; 
      }
      return _match(s, next+1);
  }
  bool is_match = false; 

  if (*p == '\0') {
    printf("ERROR: unterminated escape slash");
      exit(1);
  }
  switch (*p) {
    case 'd': 
      is_match = isdigit(*s);
      break;
    case 'w':
      is_match = isalpha(*s);
      break;
    default:
      is_match = *s == *p;
  }
  if (!is_match) {
    if (did_match) return false;
    if (match_start) return false;
    return _match(s+1, p-1);
  }
  did_match = true;
  return _match(s+1, p+1);
}

bool match_char(char *s, char *p) {
  char *next = p+1;
  switch(*next) {
    case '*':
      while(*s != '\0' && *s == *p) {
        if (_match(s, next+1)) return true;
        s++;
      }
      return _match(s, next+1);
    case '+':
      if (*s != *p) {
        return _match(s+1, p);
      }
      s++;
      while (*s != '\0' && *s == *p) {
        if (_match(s, next+1)) return true;
        s++; 
      }
      return _match(s, next+1);
  }
  if (*s == *p) {
    did_match = true;
    return _match(s+1, p+1);
  }
  if (did_match) return false;
  if (match_start) return false;
  return _match(s+1, p);
}

bool _match(char *s, char *p) {
  if (*p == '$' && *(p+1) == '\0') return true;
  if (*p == '\0') return true;  
  if (*s == '\0') return *p == '\0' || *(p+1) == '*';
  if (*p == '\\') return match_escape(s, p+1);
  if (*p == '[') return  match_class(s, p+1);
  return match_char(s, p);
}

bool match(char *s, char *p) {
  did_match = false;
  if (*p == '^') {
    match_start = true;
    p++;
  }
  if (*(p+strlen(p) - 1) == '$') {
    match_end = true;
  }
  return _match(s, p);
}

void test_char_only() {
  ASSERT(match("", "") == true);
  ASSERT(match("a", "") == true);

  ASSERT(match("ab", "b") == true);
  ASSERT(match("ab", "a") == true);
  ASSERT(match("b", "a") == false);
  ASSERT(match("aba", "aa") == false);
  ASSERT(match("bab", "a") == true);
  ASSERT(match("a\n", "a") == true);
}

void test_character_class() {
  ASSERT(match("a1", "\\d") == true);
  ASSERT(match("1a", "\\d") == true);
  ASSERT(match("a1a", "\\d") == true);
  ASSERT(match("a", "\\d") == false);
  ASSERT(match("\\d", "\\\\d") == true);
  ASSERT(match("a", "\\d") == false);

  ASSERT(match("1 apple", "\\d apple") == true);
  ASSERT(match("1 orange", "\\d apple") == false);
  ASSERT(match("100 apples", "\\d\\d\\d apple") == true);
  ASSERT(match("1 apple", "\\d\\d\\d apple") == false);
  ASSERT(match("3 dogs", "\\d \\w\\w\\ws") == true);
  ASSERT(match("4 cats", "\\d \\w\\w\\ws") == true);
  ASSERT(match("1 dog", "\\d \\w\\w\\ws") == false);

  ASSERT(match(".", "\\.") == true);
}

void test_wildcard() {
  ASSERT(match("dog", "d.+g") == true);
  ASSERT(match("dg", "d.+g") == false);

  ASSERT(match("doooooooooooog", "d.*g") == true);
  ASSERT(match("dg", "d.*g") == true);

  ASSERT(match("d..g", "^d\\.+g$") == true);
  ASSERT(match("ad..g", "^d\\.+g$") == false);
}

void test_groups() {
  ASSERT(match("abc", "[a]") == true);
  ASSERT(match("a", "[e]") == false);

  ASSERT(match("abc", "[^abc]") == false);
  ASSERT(match("def", "[^abc]") == true);
}

void test_anchors() {
  ASSERT(match("slogs", "^slog") == true);
  ASSERT(match("slogsa", "^slog") == true);
  ASSERT(match("slogs", "^log") == false);
  ASSERT(match("a", "a$") == true);
  ASSERT(match("slogs", "log$") == false);
  ASSERT(match("slogs", "^slogs$") == true);
  ASSERT(match("log", "^slogs$") == false);
}

void test_quantifiers() {
  ASSERT(match("aa", "a+") == true);
  ASSERT(match("b", "a+") == false);
  ASSERT(match("b", "a*") == true);
  ASSERT(match("a", "a*") == true);
  ASSERT(match("aa", "a*") == true);
  ASSERT(match("bca", "a*") == true);
  ASSERT(match("ba", "a*") == true);

  // TODO 
  ASSERT(match("", "a*") == true);
}

void test_alternation() {
  ASSERT(match("catdog", "(cat|dog)+") == true);
  ASSERT(match("cat", "(bird|dog)") == false);
}

void run_test_cases() {
  test_char_only();
  // test_character_class();
  // test_groups();
  // test_anchors();
  test_quantifiers();
  // test_wildcard();
  // test_alternation();
}

struct token_arr {
  struct token **tokens;
  int length;
};

typedef enum quant_t {
  NONE,
  STAR,
  PLUS
} quant_t;

typedef enum token_t {
  CHAR,
  ESCAPE,
  CHARACTER_CLASS,
  ALTERNATION,
} token_t;

struct token {
  quant_t quantifier;
  token_t type; 
  union {;
    char ch;
    char *cclass;
    struct token_arr *alternatives;
  } v;
};

void abort_with_message(char *message) {
  printf("%s", message);
  exit(1);
}

struct token_arr *mk_token_arr(char *p) {
  struct token_arr *arr = malloc(sizeof(struct token_arr));
  arr->tokens = malloc(sizeof(token_t) * strlen(p));
  arr->length = 0;

  while (*p != '\0') {
    struct token *t = malloc(sizeof(token_t));
    t->quantifier = NONE;
    if (*p == '[') {
      char *end = strchr(p, ']');
      if (end == NULL) {
        abort_with_message("ERROR: unclosed character class");
      }
      int ssize = (end - p) + 2; // space for [ and \0 inclusive
      char *cclass_str = malloc(sizeof(ssize));
      // copy from p until end inclusive
      strncpy(cclass_str, p, ssize-1);
      cclass_str[ssize-1] = '\0';
      t->type = CHARACTER_CLASS;
      t->v.cclass = cclass_str;
      p = end;
    }
    else if (*p == '\\') {
      t->type = ESCAPE;
      t->v.ch = *(p++);
    }
    else {
      t->type = CHAR;
      t->v.ch = *p;
    }
    p++;
    if (*p == '*') {
      t->quantifier = STAR;
      p++;
    }
    if (*p == '+') {
      t->quantifier = PLUS;
      p++;
    }
    arr->tokens[arr->length++] = t;
  }
  return arr;
}

void print_token_arr(struct token_arr *arr) {
  printf("%d\n", arr->length);
  for (int i = 0; i < arr->length; i++) {
    struct token *t = arr->tokens[i];
    printf("type = %d, quantifier = %d, ", t->type, t->quantifier);
    if (t->type == CHAR) {
      printf("value = %c\n", t->v.ch);
    }
    else if (t->type == CHARACTER_CLASS) {
      printf("value = %s\n", t->v.cclass);
    }
  }
}

int main(int argc, char * argv[]) {
  struct token_arr *arr = mk_token_arr(argv[1]);
  print_token_arr(arr);
  return 0;

  if (argc != 2) {
    printf("USAGE: string | cgrep PATTERN\n");
    return 1;
  }

  char * pattern = argv[1];
  if (strcmp(pattern, "test") == 0) {
    run_test_cases();
    exit(0);
  }

  char buf[BUFSZ];
  fgets(buf, sizeof buf, stdin);
  int len_line = strlen(buf);

  if (buf[len_line-1] == '\n') {
    if (match(buf, argv[1])) {
      return true;
    }
  } else {
    printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
    return false;
  }
}
  

