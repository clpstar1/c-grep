#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define BUFSZ 1024
#define TRUE 1
#define FALSE 0

// advances s until the first occurence of c in s or the end of s
int match_character_class(char ** s, char c) {
  while (**s != '\0') {
    switch (c) {
      case 'd':
        if (isdigit(**s)) return TRUE;
        break;
      case 'w':
        if (isalpha(**s)) return TRUE;
        break;
      default:
        printf("ERROR: Unrecognized character class");
        exit(1);
    }
    s++;
  }
  return FALSE;
}

// advances s until the first occurence of a single character in group is found or the end of s 
// group should point to the opening [ of the group
int match_group(char * s, char * g) {
  // discard [
  g++;
  if (*g == ']') {
    printf("ERROR: empty character group");
    exit(1);
  }
  char * ss;
  while (*g != ']') {
    if (*g == '\0') {
      printf("ERROR: unclosed character group");
      exit(1);
    }
    ss = s;
    while (*ss != '\0') {
      if (*g == *ss) {
      // if found, discard the rest of the group
        while (*g != ']') {
          g++;
        }
        return TRUE;
      }
      ss++;
    }
    g++;
  }
  return FALSE;
} 

int match(char * s, char * p) {
  // empty p does not match any string
  if (strlen(s) == 0 && strlen(p) == 0) return TRUE;
  if (strlen(s) > 0 && strlen(p) == 0) return FALSE;

  while (*p != '\0' && *s != '\0') {
    if (*p == '[') {
      int match = match_group(s, p);
      printf("%s\n", p);
      printf("%s\n", s);
      if (match) {
        p++; s++;
      }
    }
    if (*p == '\\') {
      // discard escape slash
      p++;
      // match \ literally
      if (*p == '\\') {
        continue;
      }
      int match = match_character_class(s, *p);
      // s points to the digit, advance it and pattern
      if (match) {
        p++; s++;
      }
    }
    // exact match
    if (*p == *s) {
      p++; s++;
    // no match
    } else {
      s++;
    }
  }

  return *p == '\0';
}

int main(int argc, char * argv[]) {
  // assert(match("", "") == TRUE);
  //
  // assert(match("ab", "b") == TRUE);
  // assert(match("ab", "a") == TRUE);
  // assert(match("bab", "a") == TRUE);
  // assert(match("b", "a") == FALSE);
  //
  // assert(match("a1", "\\d") == TRUE);
  // assert(match("1a", "\\d") == TRUE);
  // assert(match("a1a", "\\d") == TRUE);
  // assert(match("a", "\\d") == FALSE);
  assert(match("\\d", "\\\\d") == TRUE);
  //
  assert(match("a", "\\d") == FALSE);
  assert(match("a", "[a]") == FALSE);

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
  

