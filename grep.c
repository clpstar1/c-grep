#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFSZ 1024

int match_line(char * line, char * pattern) {
  for (int i = 0; line[i] != '\n'; i++) {

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
  if (argc != 2) {
    printf("USAGE: crepe PATTERN\n");
    return 1;
  }

  char buf[BUFSZ];
  fgets(buf, sizeof buf, stdin);
  int len_line = strlen(buf);

  if (buf[len_line-1] == '\n') {
    return match_line(buf, argv[1]);
  } else {
    printf("ERROR: line too long got %d max = %d", len_line, BUFSZ);
    return 1;
  }

  return 0;
  
}
