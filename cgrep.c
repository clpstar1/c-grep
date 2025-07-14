#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

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

// advances s until the first occurence of c in s is found
// if c is not in s then s will point to '\0' after the call fo the function
void match_escape_seq(char ** s, char c) {
  while (**s != '\0') {
    switch (c) {
      case 'd':
        if (isdigit(**s)) return;
        break;
      case 'w':
        if (isalpha(**s)) return;
        break;
      // if its not a special character class match it literally
      default:
        if (**s == c) return; 
        break;
    }
    (*s)++;
  }
}

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
            t[t_index].val = mk_simple(pattern[p_index]);
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


int match_group(char * s, char * g) {
  char * g_iter = g;

  // check for unclosed character group
  while (*g_iter != ']') {
    if (*g_iter == '\0') {
      printf("ERROR: unclosed character group");
      exit(1);
    }
    g_iter++;
  }
  // check for empty character group [
  g_iter++;
  if (*g_iter == ']') {
    printf("ERROR: empty character group");
    exit(1);
  }

  g_iter = g;
  // discard [
  g_iter++;

  char * s_iter;
  while (*g_iter != ']') {
    s_iter = s;
    while (*s_iter != '\0') {
      if (*g_iter == *s_iter) {
        return TRUE;
      }
      s_iter++;
    }
    g_iter++;
  }
  return FALSE;
} 

int match2(char *s, char *p) {
  token_array * arr = tokenize(p);
}

int match(char * s, char * p) {
  // empty p does not match any string
  if (strlen(s) == 0 && strlen(p) == 0) return TRUE;
  if (strlen(s) > 0 && strlen(p) == 0) return FALSE;

  int has_start_anchor = *p == '^';
  if (has_start_anchor) {
    p++;
  }

  int has_end_anchor = *(p + strlen(p)-1) == '$';
  char end = has_end_anchor 
    ? '$'
    : '\0';

  while (*p != end && *s != '\0') {
    int match = FALSE;

    if (*p == '[') {
      int is_negative_group = FALSE;

      if (*(p+1) == '^') {
        is_negative_group = TRUE;
      }

      match = match_group(s, p);
      if (is_negative_group) {
        match = !match;
      }

      if (match) {
        // discard the rest of the group
        for(;*p != ']'; p++);
      }
    }

    else if (*p == '\\') {
      // discard escape slash
      p++;
      match_escape_seq(&s, *p);
      // match: advance pattern 
      match = *s != '\0';
    }
    
    // exact match
    else {
      // switch (NEXT(p)) {
      //   case '+':
      //     match = FALSE;
      //     while (*s == *p) {
      //       match = TRUE;
      //       s++; 
      //     } 
      //   case '*':
      //     match = TRUE;
      //     while (*s == *p) {
      //       s++; 
      //     } 
      //     break;
      //   default: 
      //     match = *p == *s;
      //     break;
      // }
      match = *p == *s;
    }

    if (match) {
      p++;
    } else {
      if (has_start_anchor) return FALSE;
    }
    s++;
  }

  if (has_end_anchor) {
    return *p == end && *s == '\0';
  }

  return *p == end;
}

int main(int argc, char * argv[]) {
  ASSERT(match("", "") == TRUE);
  ASSERT(match("a", "") == FALSE);
  
  ASSERT(match("ab", "b") == TRUE);
  ASSERT(match("ab", "a") == TRUE);
  ASSERT(match("bab", "a") == TRUE);
  ASSERT(match("b", "a") == FALSE);

  ASSERT(match(".", "\\.") == TRUE);

  // character class 
  ASSERT(match("a1", "\\d") == TRUE);
  ASSERT(match("1a", "\\d") == TRUE);
  ASSERT(match("a1a", "\\d") == TRUE);
  ASSERT(match("a", "\\d") == FALSE);
  ASSERT(match("\\d", "\\\\d") == TRUE);
  ASSERT(match("a", "\\d") == FALSE);

  ASSERT(match("abc", "[a]") == TRUE);
  ASSERT(match("abc", "[e]") == FALSE);
  ASSERT(match("abc", "[^abc]") == FALSE);
  ASSERT(match("def", "[^abc]") == TRUE);

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

  ASSERT(match("slogs", "^slog") == TRUE);
  ASSERT(match("slogs", "^log") == FALSE);

  ASSERT(match("slogs", "logs$") == TRUE);
  ASSERT(match("slogs", "log$") == FALSE);

  ASSERT(match("slogs", "^slogs$") == TRUE);
  ASSERT(match("log", "^slogs$") == FALSE);

  ASSERT(match("aa", "a+") == TRUE);
  ASSERT(match("b", "a+") == FALSE);

  ASSERT(match("", "a*") == TRUE);
  ASSERT(match("a", "a*") == TRUE);
  ASSERT(match("aa", "a*") == TRUE);
  ASSERT(match("bca", "a*") == TRUE);

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
  

