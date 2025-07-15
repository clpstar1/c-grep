#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#define BUFSZ 1024
#define ASSERT(cond) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, "assertion failed: %s\n", #cond); \
    } \
  } while(0) \

typedef enum quantifier {
  Single,
  Plus,
  Star
} quantifier;

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

char * mk_simple(char val) {
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

token_array * tokenize(char * pattern) {
  // "abc\0";
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
      if (pattern[p_index] == '[') {
        t[t_index].val = mk_character_group(pattern);
        t[t_index].quant = Single;
        for (;*pattern != ']'; pattern++);
      }
      else if (pattern[p_index] == '\\') {
        p_index++;
        switch (pattern[p_index]) {
          case 'd':
            t[t_index].val = mk_character_class('d');
            break;
          case 'w':
            t[t_index].val = mk_character_class('w');
            break;
          default:
            t[t_index].val = mk_character_class(pattern[p_index]);
            t[t_index].quant = Single;
        }
      }
      else {
        t[t_index].val = mk_simple(pattern[p_index]);
        t[t_index].quant = Single;
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


bool match_token(char * s, char * tval) {
  if (*tval == '\\') {
    tval++;
    // printf("s = %s, tval = %s\n", s, tval);
    switch (*tval) {
      case 'd': return isdigit(*s);
      case 'w': return isalpha(*s);
      default: return *s == *tval;
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

// a, a$
bool match(char *s, char *p) {
  if (strlen(s) == 0 && strlen(p) == 0) return true;
  if (strlen(s) > 0 && strlen(p) == 0) return false;

  bool has_start_anchor = *p == '^';
  bool has_end_anchor = *(p + strlen(p) - 1) == '$';
  
  token_array arr = *tokenize(p);
  token * tokens = arr.t; 
  int ti = 0;

  bool first_match = false;

  while (ti < arr.length && *s != '\0') {
    token t = tokens[ti];
    quantifier q = t.quant;
    char * val = t.val;
    bool match = false;

    switch (q) {
      case Plus:
        while (*s != '\0' && (match = match_token(s++, val) == true));
        break;
      case Star:
        match = true;
        while (*s != '\0' && match_token(s++, val));
        break;
      default:
        match = match_token(s, val);
        break;
    }
    if (match && !first_match) {
      first_match = true;
    }

    if (has_end_anchor && first_match && !match) return false;
    if (has_start_anchor && !match) return false;
    if (match) {
      ti++;
    }
    s++;
  }
  return has_end_anchor 
  ? ti == arr.length && *s == '\0'
  : ti == arr.length;
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

  // TODO 
  // ASSERT(match("", "a*") == true);
}

void run_test_cases() {
  test_char_only();
  test_character_class();
  test_groups();
  test_anchors();
  test_quantifiers();
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
  

