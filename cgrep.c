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

struct pattern {
  struct token **tokens;
  int length;
  struct pattern *alternative;
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
  CAPTURE_GROUP,
} token_t;

struct token {
  quant_t quantifier;
  token_t type; 
  union {
    char ch;
    char *cclass;
    char *inner;
  } v;
};

typedef struct match_result {
  char *new_s;
  int new_pattern_index;
  bool is_match;
} match_result_s; 


void abort_with_message(char *message) {
  printf("%s\n", message);
  exit(1);
}

char *extract_inner(char *start, char *end){
  if (start >= end) return NULL;
  int ssize = end - start;
  char *inner_s = malloc(sizeof(char) * ssize);
  strncpy(inner_s, start+1, ssize);
  inner_s[ssize-1] = '\0';
  return inner_s;
}

struct pattern *mk_token_arr(char *p) {
  struct pattern *arr = malloc(sizeof(struct pattern));
  arr->length = 0;
  if (*p == '\0') return arr;

  arr->tokens = malloc(sizeof(struct token) * strlen(p));
  while (*p != '\0') {
    struct token *t = malloc(sizeof(struct token));
    t->quantifier = NONE;
    if (*p == '\\') {
      t->type = ESCAPE;
      if (*(p+1) == '\0') abort_with_message("ERROR: empty escape sequence");
      p++;
      t->v.ch = *p;
    }
    else if (*p == '(') {
      char *end = p;
      while ((end = strrchr(end, ')')) != NULL && *(end-1) == '\\');
      if (end == NULL) abort_with_message("ERROR: unclosed capture group");
      char *inner = extract_inner(p, end);
      if (inner == NULL) abort_with_message("ERROR: empty capture group");
      t->type = CAPTURE_GROUP;
      t->v.inner = inner;
      p = end;
    }
    else if (*p == '|') {
      if (*(p+1) == '\0') abort_with_message("ERROR: empty alternative");
      arr->alternative = mk_token_arr(p+1);
      return arr;
    }
    else if (*p == ']') {
      abort_with_message("ERROR: unclosed character class");
    }
    else if (*p == '[') {
      // copy the inner characters of [] to cclass;
      char *end = p;
      while ((end = strchr(end, ']')) != NULL && *(end-1) == '\\');
      if (end == NULL) abort_with_message("ERROR: unclosed character class");

      char *inner = extract_inner(p, end);
      if (inner == NULL) abort_with_message("ERROR: empty character class");
      t->type = CHARACTER_CLASS;
      t->v.cclass = inner;
      p = end;
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
    arr->tokens[arr->length] = t;
    arr->length++;
  }
  return arr;
}


void print_token(struct token *t) {
    printf("type = %d, quantifier = %d, ", t->type, t->quantifier);
    if (t->type == CHAR || t->type == ESCAPE) {
      printf("value = %c\n", t->v.ch);
    }
    else if (t->type == CHARACTER_CLASS) {
      printf("value = %s\n", t->v.cclass);
    }
    else if (t->type == CAPTURE_GROUP) {
      printf("value = %s\n", t->v.inner);
    }
}

void _print_token_arr(struct pattern *arr, char *s) {
  for (int i = 0; i < arr->length; i++) {
    struct token *t = arr->tokens[i];
    print_token(t);
  }
}

void print_token_arr(struct pattern *arr, char *s) {
  while (arr != NULL) {
    _print_token_arr(arr, s);
    arr = arr->alternative;
  }
}

bool match(char *s, char *p);

match_result_s match_alternatives(char *s, struct pattern *p);

match_result_s mk_match_result(char *s, int pi, int pi_expected) {
  return (match_result_s) { 
    .new_s = s, 
    .new_pattern_index = pi,
    .is_match = match_end 
      ? *s == '\0' && pi == pi_expected
      : pi == pi_expected
  };
}

match_result_s mk_fail_result() {
  return mk_match_result(NULL, 0, 1);
}

// returns a pointer to the next char in s not consumed by t
char *_match_token(char *s, struct token *t, int pi) {
  if (*s == '\0') abort_with_message("todo");
  if (t->type == CAPTURE_GROUP) {
    struct pattern *pat = mk_token_arr(t->v.inner);
    match_result_s r = match_alternatives(s, pat);
    // check if the subpattern matched 
    // printf("%d %d\n", pi, r.new_pattern_index);
    // printf("%s", r.new_s);
    if (r.new_pattern_index > pi) {
      return r.new_s;
    }
    return s;
  }
  if (t->type == CHAR) {
    if (t->v.ch == '.' || *s == t->v.ch) return s+1;
    return s;
  }
  else if (t->type == ESCAPE) {
    if (t->v.ch == 'd') return isdigit(*s) ? s+1 : s;
    if (t->v.ch == 'w') return isalpha(*s) ? s+1 : s;
    return *s == t->v.ch ? s+1 : s;
  }
  else if (t->type == CHARACTER_CLASS) {
    bool positive_match = t->v.cclass[0] != '^';
    for (int i = 0; t->v.cclass[i] != '\0'; i++) {
      if (t->v.cclass[i] == *s) {
        return positive_match ? s+1 : s;
      }
    }
    return positive_match ? s : s+1; 
  }
  abort_with_message("ERROR: unknown token type");
  return s;
}

char *match_token(char *s, struct token *t, int pi) {
  char *new_s = _match_token(s, t, pi);
  bool is_match = s != new_s;
  if (!did_match) {
    did_match = is_match;
  }
  return new_s;
}

// advances s and p until p no longer matches and returns the resulting positions
match_result_s consume_pattern(char *s, struct pattern *p) {
  int pi = 0; 
  char *sstart = s;
  while (*s != '\0' && pi < p->length) {
    struct token *t = p->tokens[pi];
    struct token *next = NULL;
    if (pi < p->length - 1) {
      next = p->tokens[pi+1];
    }
    char *new_s; 
    switch (t->quantifier) {
      // match 1 to n
      case PLUS:
        new_s = match_token(s, t, pi);
        if (s == new_s) {
          if (did_match || match_start) return mk_fail_result();
          // try again with the next char in s
          s++;
          continue;
        } 
        s = new_s;
      // match 0 to n
      case STAR:
        while(*s != '\0' && (new_s = match_token(s, t, pi)) != s) {
          if (next != NULL && match_token(s, next, pi) != s) {
            break;
          }
          s = new_s;
        }
        pi++;
        break;
      default:
        new_s = match_token(s, t, pi);
        if (s == new_s) {
          if (did_match || match_start) return mk_fail_result();
          // try again with the next char in s
          s++;
          continue;
        } 
        pi++;
        s = new_s;
    }
  }
  return mk_match_result(s, pi, p->length);
  // printf("pi = %d, len = %d\n", pi, arr->length);
}

/*
  * (cat|dog)bird, catbird
  * */

match_result_s match_alternatives(char *s, struct pattern *pat) {
  while (pat != NULL) {
    match_result_s m = consume_pattern(s, pat);
    if (m.is_match) {
      return m;
    }
    pat = pat->alternative;
  }
  return mk_fail_result();
}

bool match(char *s, char *p) {
  if (s == NULL || p == NULL) {
    abort_with_message("ERROR: string or pattern to match must not be null");
  }
  if (*p == '\0') return *s == '\0';

  match_start = false;
  match_end = false;
  did_match = false;
  
  if (*p == '^') {
    match_start = true;
    p++;
  }
  int plen = strlen(p);
  if (plen > 0 && p[plen-1] == '$') {
    char *wp = malloc(sizeof(char) * strlen(p));
    strcpy(wp, p);
    p = wp;
    p[plen-1] = '\0';
    match_end = true;
  }
  if (*s == '\0' && *p == '\0') return true;
  struct pattern *pat = mk_token_arr(p);
  if (*s == '\0') {
    return pat->tokens[0]->quantifier == STAR;
  }
  return match_alternatives(s, pat).is_match;
}

void test_char_only() {
  ASSERT(match("", "") == true);
  ASSERT(match("a", "") == false);
  ASSERT(match("a", "a") == true);
  ASSERT(match("b", "a") == false);

  ASSERT(match("ab", "b") == true); // suffix
  ASSERT(match("ab", "a") == true); // prefix
  ASSERT(match("bab", "a") == true); // infix
  ASSERT(match("aba", "aa") == false); // split pattern;
}

void test_character_class() {
  ASSERT(match("a1", "\\d") == true);
  ASSERT(match("1a", "\\d") == true);
  ASSERT(match("a1a", "\\d") == true);
  ASSERT(match("a", "\\d") == false);
  ASSERT(match("\\d", "\\\\d") == true);
  ASSERT(match("1 apple", "\\d apple") == true);
  ASSERT(match("1 orange", "\\d apple") == false);
  ASSERT(match("100 apples", "\\d\\d\\d apple") == true);
  ASSERT(match("1 apple", "\\d\\d\\d apple") == false);
  ASSERT(match("3 dogs", "\\d \\w\\w\\ws") == true);
  ASSERT(match("4 cats", "\\d \\w\\w\\ws") == true);
  ASSERT(match("1 dog", "\\d \\w\\w\\ws") == false);

  ASSERT(match(".", "\\.") == true);
  // ASSERT(match("\\", "\\") == true);
}

void test_wildcard() {
  ASSERT(match("dog", "d.+g") == true);
  ASSERT(match("dg", "d.+g") == false);

  ASSERT(match("dog", "d.*g") == true);
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
  ASSERT(match("", "^") == true);
  ASSERT(match("", "$") == true);

  ASSERT(match("a", "^a") == true);
  ASSERT(match("ba", "^a") == false);
  ASSERT(match("^a", "^^a") == true);

  ASSERT(match("a", "a$") == true);
  ASSERT(match("ba", "b$") == false);
  ASSERT(match("a", "a$$") == false);

  ASSERT(match("a", "^a$") == true);
  ASSERT(match("ab", "^a$") == false);
}

void test_quantifiers() {
  ASSERT(match("a", "a*") == true);
  ASSERT(match("aa", "a*") == true);
  ASSERT(match("b", "a*") == true);
  ASSERT(match("aba", "a*") == true);
  ASSERT(match("bca", "a*") == true);
  ASSERT(match("ba", "a*") == true);

  ASSERT(match("a", "a+") == true);
  ASSERT(match("aa", "a+") == true);
  ASSERT(match("b", "a+") == false);
  ASSERT(match("ab", "a+") == true);
  ASSERT(match("aba", "a+") == true);

  // TODO 
  ASSERT(match("", "a*") == true);
}

void test_alternation() {
  ASSERT(match("dog", "cat|dog") == true);
  ASSERT(match("bird", "cat|dog|bird") == true);
  ASSERT(match("cat", "bird|dog") == false);
  ASSERT(match("cat|", "cat\\|") == true);
}

void test_capture_group() {
  ASSERT(match("dog", "(dog)"));
  ASSERT(match("dogdogbirdcat", "(dog|bird)+cat") == true);
  ASSERT(match("dogcogbirdcat", "(dog|bird)+cat") == false);
  ASSERT(match("", "(dog|bird)*") == true);
}

void run_test_cases() {
  // test_char_only();
  // test_character_class();
  // test_groups();
  test_anchors();
  // test_quantifiers();
  // test_wildcard();
  // test_alternation();
  // test_capture_group();
}


int main(int argc, char * argv[]) {

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
    return match(buf, argv[1]);
  } else {
    printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
    return false;
  }
}
  

