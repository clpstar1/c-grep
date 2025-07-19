#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define BUFSZ 1024
#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "assertion failed: %s\n", #cond);                        \
    }                                                                          \
  } while (0)

bool match(char *s, char *p);

typedef enum quantifier { Single, Plus, Star } quantifier;

typedef struct token {
  char * val; 
  quantifier quant;
} token;

typedef struct token_array {
  token * t;
  int length;
} token_array;

char * mk_character_class(char class) {
  char * c = malloc(sizeof(char) * 3);
  c[0] = '\\';
  c[1] = class;
  c[2] = '\0';
  return c;
}

char * mk_character(char val) {
  char * c = malloc(sizeof(char) * 2);
  c[0] = val;
  c[1] = '\0';
  return c;
}

char * mk_character_group(char * pattern) {
  int end = 0;
  // [a]

  char pcur;
  do {
    pcur = pattern[end];
    if (pcur == ']' && end < 2) {
      printf("ERROR: empty character group");
      exit(1);
    }
    end++;
  } while (pcur != ']');

  char * ptr = malloc(sizeof(char) * (end + 1));
  
  for (int i = 0; i <= end; i++) {
    ptr[i] = pattern[i];
    if (i == end) {
      ptr[i] = '\0';
    }
  }

  return ptr; 
}

char * mk_alternation(char * pattern) {
  int token_idx = 1; // skip opening paren 
  int option_sz = 0;
  char pcur; 
  while ((pcur = pattern[token_idx]) != ')') {
    if (pcur == '\0') {
      printf("ERROR: unclosed alternation");
      exit(1);
    }
    if (pcur == '|') {
      if (option_sz == 0) {
        printf("ERROR: empty alternation option");
        exit(1);
      }
      option_sz = 0;
    }
    option_sz++;
    token_idx++;
  }
  if (token_idx == 1) {
    printf("ERROR: empty alternation");
    exit(1);
  }
  char * val = malloc(sizeof(char) * (token_idx+1));
  for (int i = 0; i <= token_idx; i++) {
    if (i == token_idx) {
      val[i] = '\0';
    }
    val[i] = pattern[i];
  }
  return val;
}

token_array * tokenize(char * pattern) {
  token * t = malloc(sizeof(token) * (strlen(pattern)));
  int t_index = 0; 
  int p_index = 0;

  while (pattern[p_index] != '\0') {
    if (p_index == 0 && pattern[p_index] == '^') {
      p_index++;
      continue;
    }
    
    if (p_index == strlen(pattern) - 1 && pattern[p_index] == '$') {
      p_index++;
      continue;
    }

    if (pattern[p_index] == '*') {
      if (t_index > 0) {
        t[t_index-1].quant = Star;
      }
      p_index++;
    }
    else if (pattern[p_index] == '+') {
      if (t_index > 0) {
        t[t_index-1].quant = Plus;
      }
      p_index++;
    }

    else {
      token * tcur = &t[t_index];
      tcur->quant = Single;

      if (pattern[p_index] == '(') {
        tcur->val = mk_alternation(&pattern[p_index]);
        while(pattern[p_index] != ')') p_index++;
      }
      else if (pattern[p_index] == '[') {
        tcur->val = mk_character_group(pattern);
        while (pattern[p_index] != ']') p_index++;
      }
      else if (pattern[p_index] == '\\') {
        p_index++;
        switch (pattern[p_index]) {
          case 'd':
            tcur->val = mk_character_class('d');
            break;
          case 'w':
            tcur->val = mk_character_class('w');
            break;
          default:
            tcur->val = mk_character_class(pattern[p_index]);
        }
      }
      else {
        tcur->val = mk_character(pattern[p_index]);
      }
      t_index++;
      p_index++;
    }
  }
  token_array * arr = malloc(sizeof(token_array));
  arr->t = t;
  arr->length = t_index;
  return arr;
}

bool match(char *s, char *p);

bool match_token(char * s, char * tval) {
  if (*tval == '.') return true;
  if (*tval == '\\') {
    tval++;
    switch (*tval) {
      case 'd': return isdigit(*s);
      case 'w': return isalpha(*s);
      default: return *s == *tval;
    }
  }
  // (cat|dog)
  else if (*tval == '(') {
    // discard opening paren
    tval++;
    char * t_iter = tval;
    int alternation_token_sz = 0; 
    char * alternation;
    while (true) {
      if (*t_iter == '|') {
        alternation = malloc(sizeof(char) * alternation_token_sz + 1);
        memcpy(alternation, tval, alternation_token_sz);
        alternation[alternation_token_sz] = '\0';
        if (match(s, alternation)) return true; 
        alternation_token_sz = 0;
        t_iter++;
        tval = t_iter;
      }
      if (*t_iter == ')') {
        alternation = malloc(sizeof(char) * alternation_token_sz + 1);
        memcpy(alternation, tval, alternation_token_sz);
        alternation[alternation_token_sz] = '\0';
        return match(s, alternation);
      }
      else {
        alternation_token_sz++;
        t_iter++;
      }
    }
  }
  else if (*tval == '[') {
    tval++;
    bool is_negative_group = false;
    if (*tval == '^') {
      is_negative_group = true;
      tval++;
    }
    char * s_iter;
    while (*tval != ']') {
      s_iter = s;
      while (*s_iter != '\0') {
        if (match_token(s_iter, tval)) {
          return !is_negative_group;
        }
        s_iter++;
      }
      tval++;
    }
    return is_negative_group;
  }
  else {
    return *s == *tval;
  }
}

bool match(char *s, char *p) {
  if (strlen(s) == 0 && strlen(p) == 0) return true;
  if (strlen(s) > 0 && strlen(p) == 0) return false;

  bool has_start_anchor = *p == '^';
  bool has_end_anchor = *(p + strlen(p) - 1) == '$';
  
  token_array arr = *tokenize(p);
  token * tokens = arr.t; 
  int ti = 0;

  bool first_match = false;

  // for (int i = 0; i < arr.length; i++) {
  //   printf("val = %s, quant = %d\n", tokens[i].val, tokens[i].quant);
  // }

  do {
    token t = tokens[ti];
    quantifier q = t.quant;
    char * val = t.val;
    bool match = false;

    char * next = NULL; 
    if (ti+1 < arr.length) {
      next = tokens[ti+1].val;
    }

    switch (q) {
      // match between 1 and n times 
      case Plus:
        match = match_token(s, val);
        // no match means advance the string but NOT advancing the pattern
        // - in s = ba and p = a+, 'a+' only matches the second a and b is ignored
        if (!match) {
          s++;
          break;
        }
        // consume until token no longer matches or s is empty
        s++;
        while (*s != '\0' && match_token(s, val)) {
          // if the next token in the pattern also matches our greedy consumption must end
          // otherwise in the pattern "d.+g" and the string dog, '.+' consumes everything since '.+' also matches 'g'
          // this would result in the pattern not being fully consumed when the string is fully consumed
          if (next != NULL && match_token(s, next)) break; 
          s++;
        }
        ti++;
        break;
      // match between 0 and n times
      case Star:
        // always true since 0 matches are allowed
        match = true;
        // consume until token no longer matches or s is empty
        while (*s != '\0' && match_token(s, val)) {
          // see explanation for Plus quantifier
          if (next != NULL && match_token(s, next)) break; 
          s++;
        }
        // here we can always advance the pattern as every char of s is a positive match
        ti++;
        break;
      default:
        match = match_token(s, val);
        if (match) {
          ti++;
        }
        s++;
        break;
    }

    if (match && !first_match) {
      first_match = true;
    }

    if (has_end_anchor && first_match && !match) return false;
    if (has_start_anchor && !match) return false;
  } while (ti < arr.length && *s != '\0');

  
  return has_end_anchor 
    ? ti == arr.length && *s == '\0'
    : ti == arr.length;
}

void test_wildcard() {
  ASSERT(match("dog", "d.+g") == true);
  ASSERT(match("dg", "d.+g") == false);

  ASSERT(match("doooooooooooog", "d.*g") == true);
  ASSERT(match("dg", "d.*g") == true);

  ASSERT(match("d..g", "^d\\.+g$") == true);
  ASSERT(match("ad..g", "^d\\.+g$") == false);
}

void test_char_only() {
  ASSERT(match("", "") == true);
  ASSERT(match("a", "") == false);
  //
  ASSERT(match("ab", "b") == true);
  ASSERT(match("ab", "a") == true);
  ASSERT(match("bab", "a") == true);
  ASSERT(match("b", "a") == false);

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

void test_groups() {
  ASSERT(match("abc", "[a]") == true);
  ASSERT(match("abc", "[e]") == false);

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
  ASSERT(match("", "a*") == true);
}

void test_alternation() {
  ASSERT(match("catdog", "(cat|dog)+") == true);
  ASSERT(match("cat", "(bird|dog)") == false);
  // ASSERT(match("cat", "(|dog)") == false);
  // ASSERT(match("cat", "(") == false);
  // ASSERT(match("cat", "(|dog|)") == false);
  // ASSERT(match("cat", "()") == false);
}

void run_test_cases() {
  test_char_only();
  test_character_class();
  test_groups();
  test_anchors();
  test_quantifiers();
  test_wildcard();
  test_alternation();
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
    if (match(buf, argv[1])) {
      return true;
    }
  } else {
    printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
    return false;
  }
}
  

