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

#define IS_MATCH(s, s_next) (s != s_next)

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
  BRACKET_EXPR,
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

bool match(char *s, char *p);

match_result_s match_alternatives(char *s, struct pattern *p);

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

void print_token(struct token *t) {
    printf("type = %d, quantifier = %d, ", t->type, t->quantifier);
    if (t->type == CHAR || t->type == ESCAPE) {
      printf("value = %c\n", t->v.ch);
    }
    else if (t->type == BRACKET_EXPR) {
      printf("value = %s\n", t->v.cclass);
    }
    else if (t->type == CAPTURE_GROUP) {
      printf("value = %s\n", t->v.inner);
    }
}

void print_pattern(struct pattern *p, char *s) {
  while (p != NULL) {
    for (int i = 0; i < p->length; i++) {
      struct token *t = p->tokens[i];
      print_token(t);
    }
    p = p->alternative;
  }
}

struct pattern *mk_pattern(char *p) {
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
      arr->alternative = mk_pattern(p+1);
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
      t->type = BRACKET_EXPR;
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
  return mk_match_result("", 0, 1);
}

bool match_char(char *s, struct token *t) {
  return (t->v.ch == '.' || *s == t->v.ch);
}

bool match_escape(char *s, struct token *t) {
    if (t->v.ch == 'd') return isdigit(*s);
    if (t->v.ch == 'w') return isalpha(*s);
    return *s == t->v.ch;
}

// returns a pointer to the next char in s not consumed by t
char *_match_token(char *s, struct token *t, int pi) {
  if (*s == '\0') abort_with_message("todo");
  if (t->type == CAPTURE_GROUP) {
    struct pattern *pat = mk_pattern(t->v.inner);
    match_result_s r = match_alternatives(s, pat);
    // check if the subpattern matched 
    if (r.new_pattern_index > pi) {
      return r.new_s;
    }
    return s;
  }
  if (t->type == CHAR) {
    return match_char(s, t) ? s + 1 : s;
  }
  else if (t->type == ESCAPE) {
    return match_escape(s, t) ? s + 1 : s;
  }
  else if (t->type == BRACKET_EXPR) {
    bool positive_match = t->v.cclass[0] != '^';
    struct pattern *p = mk_pattern(t->v.cclass);
    for (int i = 0; i < p->length; i++) {
      if (i == 0 && !positive_match) continue;
      if (match_escape(s, p->tokens[i]) || match_char(s, p->tokens[i])) {
        return positive_match ? s + 1 : s;
      }
    }
    return positive_match ? s : s + 1; 
  }
  abort_with_message("ERROR: unknown token type");
  return s;
}

char *match_token(char *s, struct token *t, int pi) {
  char *s_next = _match_token(s, t, pi);
  bool is_match = IS_MATCH(s, s_next);
  if (!did_match) {
    did_match = is_match;
  }
  return s_next;
}

// advances s and p until p no longer matches and returns the resulting positions
match_result_s consume_pattern(char *s, struct pattern *p) {
  int pi = 0; 
  while (*s != '\0' && pi < p->length) {
    struct token *t = p->tokens[pi];
    struct token *next = NULL;
    if (pi < p->length - 1) {
      next = p->tokens[pi+1];
    }
    char *s_next; 
    switch (t->quantifier) {
      // match 1 to n
      case PLUS:
        s_next = match_token(s, t, pi);
        if (!IS_MATCH(s, s_next)) {
          if (did_match || match_start) return mk_fail_result();
          // try again with the next char in s
          s++;
          continue;
        } 
        s = s_next;
      // match 0 to n
      case STAR:
        while(*s != '\0' && (s_next = match_token(s, t, pi)) != s) {
          if (next != NULL && match_token(s, next, pi) != s) {
            break;
          }
          s = s_next;
        }
        pi++;
        break;
      default:
        s_next = match_token(s, t, pi);
        // printf("new_s = %s, s = %s\n", new_s, s);
        // s == new_s means fail 
        if (!IS_MATCH(s, s_next)) {
          if (did_match || match_start) return mk_fail_result();
          // try again with the next char in s
          s++;
          continue;
        } 
        pi++;
        s = s_next;
    }
  }
  return mk_match_result(s, pi, p->length);
}

match_result_s match_alternatives(char *s, struct pattern *p) {
  did_match = false;
  while (p != NULL) {
    if (*s == '\0' && p->tokens[0]->quantifier == STAR) {
      return mk_match_result(s, 0, 0);
    }
    match_result_s m = consume_pattern(s, p);
    if (m.is_match) {
      return m;
    }
    did_match = false;
    p = p->alternative;
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
  return match_alternatives(s, mk_pattern(p)).is_match;
}

void test_char_only() {
  ASSERT(match("", "") == true);
  ASSERT(match("a", "") == false);

  // exact match 
  ASSERT(match("a", "a") == true);
  ASSERT(match("b", "a") == false);

  // substring
  ASSERT(match("ab", "b") == true);
  ASSERT(match("ab", "a") == true); 
  ASSERT(match("bab", "a") == true);
  ASSERT(match("aba", "aa") == false); 
}

void test_character_class() {
  // digit class
  ASSERT(match("1", "\\d") == true);
  ASSERT(match("a", "\\d") == false);

  // word class
  ASSERT(match("w", "\\w") == true);
  ASSERT(match("1", "\\w") == false);

  // grouping constructs
  ASSERT(match("abc", "[ade]") == true);
  ASSERT(match("abc", "[def]") == false);
  ASSERT(match("abc", "[^abc]") == false);
  ASSERT(match("def", "[^abc]") == true);
}

void test_escaping() {
  ASSERT(match("\\", "\\\\") == true);
  ASSERT(match("\\d", "\\\\d") == true);
  ASSERT(match("\\d", "\\d") == false);
  ASSERT(match("b", "a") == false);
}

void test_wildcards() {
  // dot matches a single characer ONCE
  ASSERT(match("dog", "d.g") == true);
  ASSERT(match("og", "d.g") == false);
}

void test_anchors() {
  // ^ matches the start of the string 
  ASSERT(match("", "^") == true);
  ASSERT(match("a", "^a") == true);
  ASSERT(match("ba", "^a") == false);
  ASSERT(match("^a", "^^a") == true);

  // $ matches end of the string 
  ASSERT(match("", "$") == true);
  ASSERT(match("a", "a$") == true);
  ASSERT(match("ba", "b$") == false);
  ASSERT(match("$a", "$a$") == true);

  // beginning and end
  ASSERT(match("", "^$") == true);
  ASSERT(match("a", "^a$") == true);
  ASSERT(match("ab", "^a$") == false);
}

void test_quantifiers() {
  // * matches 0 to n times
  ASSERT(match("b", "a*") == true); // 0
  ASSERT(match("a", "a*") == true); // 1 
  ASSERT(match("aab", "a*") == true); // n

  // + matches 1 to n times
  ASSERT(match("b", "a+") == false); // 0
  ASSERT(match("a", "a+") == true); // 1 
  ASSERT(match("aab", "a+") == true); // n
}

void test_alternation() {
  // | matches if at least one alternative matches
  ASSERT(match("dog", "cat|dog") == true);
  ASSERT(match("bird", "cat|dog|bird") == true);
  ASSERT(match("cat", "bird|dog") == false);
}

void test_capture_group() {
  // () creates a subpattern which can have any construct except start and end of string anchors 
  ASSERT(match("dog", "(dog)") == true);
  ASSERT(match("dog", "(cat)") == false);
  ASSERT(match("a", "((((a))))") == true);
  ASSERT(match("a", "(((\\(a\\))))") == false);
}

void test_combinations() {
  // character class with quantifiers
  ASSERT(match("11", "\\d+") == true);
  ASSERT(match("", "\\d*") == true);
  ASSERT(match("aab", "[a]*b") == true);
  ASSERT(match("aab", "[\\w]*b") == true);
  ASSERT(match("aab", "[^a]*b") == true);

  // wild chard with quantifiers
  ASSERT(match("acb", "[.]*b") == true);
  ASSERT(match("b", "[.]*b") == true);
  ASSERT(match("b", "[.]+b") == false);

  // alternations
  ASSERT(match("acb", "^\\w|\\d$") == false);
  ASSERT(match("acb", "^\\w\\w\\w|\\d+$") == true);
  ASSERT(match("111", "^\\w\\w\\w|\\d+$") == true);
  ASSERT(match("", "^\\w\\w\\w|\\d*$") == true);

  // groups and alternations 
  ASSERT(match("abcabcabc", "(abc)+") == true);
  ASSERT(match("abfabfabf", "(abc)+") == false);
  ASSERT(match("catdogcatbird", "(cat|dog)+bird") == true);
  ASSERT(match("catdogcatbird", "(cat|dog)+$") == false);
  ASSERT(match("catcat", "cat|(dog|bird)+$") == false);
  ASSERT(match("catcat", "c(dog|bird)*atcat$") == true);

  ASSERT(match("ctacat", "c[.]+") == true);
}

void run_test_cases() {
  // ASSERT(match(NULL, NULL) == true);
  // ASSERT(match("", NULL) == false);
  // ASSERT(match(NULL, "") == false);
  test_char_only();
  test_character_class();
  test_anchors();
  test_quantifiers();
  test_wildcards();
  test_alternation();
  test_capture_group();
  test_escaping();
  test_combinations();
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
  

