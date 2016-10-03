#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main()
{
  int bytes_read;
  size_t nbytes = 100;
  char *my_string = calloc(100, sizeof(char));
  sprintf(my_string, "%s", "hellohwsds");
  char *token;

  token = strsep(&my_string, "h");
  printf("%s\n", my_string);;
  printf("%s\n",token);

  return 0;
}