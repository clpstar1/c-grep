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

// advances s until the first occurence of a single character in group is found or the end of s 
// group should point to the opening [ of the group
int match_group(char ** s, char ** g) {
  char * gg = *g;

  // check for unclosed character group
  while (*gg != ']') {
    if (*gg == '\0') {
      printf("ERROR: unclosed character group");
      exit(1);
    }
    gg++;
  }
  // check for empty character group [
  gg++;
  if (*gg == ']') {
    printf("ERROR: empty character group");
    exit(1);
  }

  // check for a match
  gg = *g;
  // discard [
  gg++;
  char * ss;
  int match = FALSE;
  while (match == FALSE && *gg != ']') {
    ss = *s;
    while (*ss != '\0') {
      if (*gg == *ss) {
        // if found, discard the rest of the group
        match = TRUE;
        while (*gg != ']') {
          gg++;
        }
        break;
      }
      ss++;
    }
    if (!match) {
      gg++;
    }
  }
  // set s to the found letter
  *s = ss;
  *g = gg;
} 

int match(char * s, char * p) {
  // empty p does not match any string
  if (strlen(s) == 0 && strlen(p) == 0) return TRUE;
  if (strlen(s) > 0 && strlen(p) == 0) return FALSE;

  while (*p != '\0' && *s != '\0') {

    if (*p == '[') {
      match_group(&s, &p);
      if (*s != '\0') {
        p++;
      }
      s++;
    }

    else if (*p == '\\') {
      // discard escape slash
      p++;
      match_escape_seq(&s, *p);
      // match: advance pattern 
      if (*s != '\0') {
        p++;
      }
      s++;
    }
    // exact match
    else if (*p == *s) {
      p++; s++;
    } else {
      s++;
    }
  }

  return *p == '\0';
}

int main(int argc, char * argv[]) {
  ASSERT(match("", "") == TRUE);
  
  ASSERT(match("ab", "b") == TRUE);
  ASSERT(match("ab", "a") == TRUE);
  ASSERT(match("bab", "a") == TRUE);
  ASSERT(match("b", "a") == FALSE);

  // match characters not in a character class literally
  ASSERT(match(".", "\\.") == TRUE);

  // character class 
  ASSERT(match("a1", "\\d") == TRUE);
  ASSERT(match("1a", "\\d") == TRUE);
  ASSERT(match("a1a", "\\d") == TRUE);
  ASSERT(match("a", "\\d") == FALSE);
  ASSERT(match("\\d", "\\\\d") == TRUE);
  ASSERT(match("a", "\\d") == FALSE);


  ASSERT(match("a", "[a]") == TRUE);

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
  

