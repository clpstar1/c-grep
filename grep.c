#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define BUFSZ 1024
#define TRUE 1
#define FALSE 0

int match(char * s, char * p) {
  // empty p does not match any string
  if (strlen(s) == 0 && strlen(p) == 0) return TRUE;
  if (strlen(s) > 0 && strlen(p) == 0) return FALSE;

  while (*p != '\0' && *s != '\0') {
    // character group
    if (*p == '[') {
      // TODO Copy the inner pattern and match it
    } 
    // character class
    else if (*p == '\\') {
      if (*(p++) == '\0') {
        printf("ERROR: unclosed character class");
        exit(1);
      }
      switch (*p) {
        case 'd':
          while (*s != '\0' && !isdigit(*s)) {
            s++;
          }
          break;
        case 'w':
          while (*s != '\0' && !isalpha(*s)) {
            s++;
          }
          break;
        default:
          printf("ERROR: unrecognized character class");
          exit(1);
      }
      // no char class in string 
      if (*s == '\0') {
        return FALSE;
      }
      p++; s++;
    } 
    // exact match
    else if (*p == *s) {
      p++; s++;
    // no match
    } else {
      s++;
    }
  }

  return *p == '\0';
}

int main(int argc, char * argv[]) {
  assert(match("", "") == TRUE);

  // simple matches
  //
  // |a|0   -> a|0|
  // |a|0   -> a|0|
  assert(match("a", "a") == TRUE);

  // |a|b0  -> a|b|0
  // |b|0   -> |b|0
  assert(match("ab", "b") == TRUE);

  // |a|b0  -> |b|0
  // |a|0   -> |0|
  assert(match("ab", "a") == TRUE);
  //
  // |a|0   -> a|0|
  // |b|0   -> a|0|
  assert(match("b", "a") == FALSE);

}
  // assert(match("Hell1", "\\d") == TRUE);
  // assert(match("a", "\\d") == FALSE);
  // assert(match("wa", "\\w\\w") == TRUE);
  // assert(match("!?$", "\\w") == FALSE);
  //
  // assert(match_line("Hello", "[abe]") == TRUE);
  // assert(match_line("Hello", "[abc]") == FALSE);

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
  //   return match_line(buf, argv[1]);
  // } else {
  //   printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
  //   return 1;
  // }
  //
  // return 0;
  

