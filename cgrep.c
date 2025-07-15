#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#define BUFSZ 1024
#define TRUE 1
#define FALSE 0

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

int match(char *s, char *p) {
  if (strlen(s) == 0 && strlen(p) == 0) return TRUE;
  if (strlen(s) > 0 && strlen(p) == 0) return FALSE;

  bool has_start_anchor = *p == '^';
  if (has_start_anchor) {
    p++;
  }

  token_array arr = *tokenize(p);
  token * tokens = arr.t; 

  bool has_end_anchor = *(p + strlen(p)-1) == '$';

  int ti = 0;
  while (ti < arr.length && *s != '\0') {
    token t = tokens[ti];
    quantifier q = t.quant;
    char * val = t.val;
    bool match = false;

    if (*val == '$') {
      ti++;
    }

    switch (q) {
      case Plus:
        while (*s != '\0') {
          if (match_token(s, val)) {
            match = true;
          } else {
            break;
          }
          s++;
        }
        if (match) {
          ti++;
        }
        s++;
        break;
      case Star:
        match = true;
        while (*s != '\0') {
          if (!match_token(s, val)) break;
          s++;
        }
        s++;
        ti++;
        break;
      default:
        if (match_token(s, tokens[ti].val)) {
          match = true;
          ti++;
        }
        s++;
    }
    if (has_start_anchor && !match) {
      return false;
    }
  }
    if (*val == '$') {
      ti++;
    }
  return has_end_anchor
  ? *s == '\0' && ti == arr.length
  : ti == arr.length;
  
}

int main(int argc, char * argv[]) {
  ASSERT(match("", "") == TRUE);
  ASSERT(match("a", "") == FALSE);
  //
  ASSERT(match("ab", "b") == TRUE);
  ASSERT(match("ab", "a") == TRUE);
  ASSERT(match("bab", "a") == TRUE);
  ASSERT(match("b", "a") == FALSE);
  //
  ASSERT(match("aa", "a+") == TRUE);
  ASSERT(match("b", "a+") == FALSE);
  ASSERT(match("b", "a*") == TRUE);
  ASSERT(match("a", "a*") == TRUE);
  ASSERT(match("aa", "a*") == TRUE);
  ASSERT(match("bca", "a*") == TRUE);

  // TODO 
  // ASSERT(match("", "a*") == TRUE);

  ASSERT(match(".", "\\.") == TRUE);

  // character class 
  ASSERT(match("a1", "\\d") == TRUE);
  ASSERT(match("1a", "\\d") == TRUE);
  ASSERT(match("a1a", "\\d") == TRUE);
  ASSERT(match("a", "\\d") == FALSE);
  ASSERT(match("\\d", "\\\\d") == TRUE);
  match("\\d", "\\\\d");
  ASSERT(match("a", "\\d") == FALSE);

  ASSERT(match("abc", "[a]") == TRUE);
  ASSERT(match("abc", "[e]") == FALSE);

  ASSERT(match("abc", "[^abc]") == FALSE);
  ASSERT(match("def", "[^abc]") == TRUE);
  //
  // \d apple should match "1 apple", but not "1 orange".
  ASSERT(match("1 apple", "\\d apple") == TRUE);
  ASSERT(match("1 orange", "\\d apple") == FALSE);
  // \d\d\d apple should match_escape_seqh "100 apples", but not "1 apple".
  ASSERT(match("100 apples", "\\d\\d\\d apple") == TRUE);
  ASSERT(match("1 apple", "\\d\\d\\d apple") == FALSE);
  // \d \w\w\ws should match "3 dogs" and "4 cats" but not "1 dog" (because the "s" is not present at the end).
  ASSERT(match("3 dogs", "\\d \\w\\w\\ws") == TRUE);
  ASSERT(match("4 cats", "\\d \\w\\w\\ws") == TRUE);
  ASSERT(match("1 dog", "\\d \\w\\w\\ws") == FALSE);
  //
  ASSERT(match("slogs", "^slog") == TRUE);
  ASSERT(match("slogs", "^log") == FALSE);

  ASSERT(match("slogs", "logs$") == TRUE);
  ASSERT(match("slogs", "log$") == FALSE);

  ASSERT(match("slogs", "^slogs$") == TRUE);
  ASSERT(match("log", "^slogs$") == FALSE);

  // if (argc != 2) {
  //   printf("USAGE: crepe PATTERN\n");
  //   return 1;
  // }
  //
  // char buf[BUFSZ];
  // fgets(buf, sizeof buf, stdin);
  // int len_line = strlen(buf);
  //
  // if (buf[len_line-1] == '\n') {
  //   if (match(buf, argv[1])) {
  //     return TRUE;
  //   }
  // } else {
  //   printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
  //   return FALSE;
  // }
}
  

