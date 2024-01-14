#include<stdio.h>
int initialize() {
    printf("Initializing plugin\n");
    return 0;
}

int run(char** argv) {
  printf("Running plugin\n");
  for (int i = 0; argv[i] != NULL; i++) {
    printf("argv[%d] = %s\n", i,argv[i]); 
  }
}

int cleanup() {
    printf("Cleaning plugin\n");
}
