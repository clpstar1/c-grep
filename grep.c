#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define BUFSZ 1024
#define TRUE 1
#define FALSE 0

int match_line(char * line, char * pattern) {
  int l_idx = 0;
  int l_len = strlen(line);
  int p_idx = 0;
  int p_len = strlen(pattern);

  // pattern is not consumed yet
  while (p_idx < p_len) {
    
    if (l_idx >= l_len) {
      return FALSE;
    }

    char p_cur = pattern[p_idx];
    char l_cur = line[l_idx];
    // printf("%c\n", p_cur);

    if (p_cur == '[') {
      int match = FALSE; 
      // consume characters until a character in the group is met or string is empty
      while (l_idx++ < l_len) {
        l_cur = line[l_idx];
        int group_start = p_idx + 1;
        while ((p_cur = pattern[group_start++]) != ']') {
          if (group_start > p_len) {
            printf("ERROR: unmatched bracket");
            exit(1);
          }
          if (p_cur == l_cur) { 
            match = TRUE;
            break;
          } 
        }
      }
      return match;
    }
    else if (p_cur == '\\') {
      // consume characters until class is met or string is empty
      int match = FALSE;
      p_idx++;
      char class = pattern[p_idx];
      while (l_idx < l_len) {
        l_cur = line[l_idx];
        switch (class) {
          case 'd':
              match = isdigit(l_cur);
              break;
          case 'w':
              match = isalpha(l_cur);
              break;
          default:
            printf("ERROR: unrecognized character class");
            exit(EXIT_FAILURE);
        }
        if (match) {
            return TRUE;
        }
        l_idx++;
      }
      return match;
    }
    // consume characters until p_cur is met or line is empty
    else {
      // matched, increase both
      if (p_cur == l_cur) {
        l_idx++;
        p_idx++;
      }
      // mismatch 
      else {
        l_idx++;
      }
    }
    
  }

  // pattern consumed, line still has characters left 
  if (l_idx < l_len - 1) {
    return FALSE;
  }

  return TRUE; 

  for (int i = 0; line[i] != '\0'; i++) {

    if (pattern[0] == '[') {
      // try each letter in the character group for each letter in line
    }

    if (pattern[0] == '\\') {
      switch (pattern[1]) {
        case 'd':
          if (isdigit(line[i])) {
            return 0;
          }
          break;
        case 'w':
          if (isalpha(line[i])) {
            return 0;
          }
          break;
      }
    }

    if (line[i] == pattern[0]) {
      return 0;
    }
    
  }
  return 1;
}

int main(int argc, char * argv[]) {
  assert(match_line("abc", "bc") == TRUE);
  assert(match_line("abc", "bbc") == FALSE);
  assert(match_line("Hello", "o") == TRUE);
  assert(match_line("Hello", "x") == FALSE);
  assert(match_line("Hell1", "\\d") == TRUE);
  assert(match_line("Hello", "\\d") == FALSE);
  assert(match_line("Hello", "\\w") == TRUE);
  assert(match_line("!?$", "\\w") == FALSE);

  assert(match_line("Hello", "[abe]") == TRUE);
  assert(match_line("Hello", "[abc]") == FALSE);

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
  
}
