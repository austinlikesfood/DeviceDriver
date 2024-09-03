/**************************************************************
* Class:  CSC-415-02 Summer 2024
* Name: Austin Kuykendall
* Student ID: 920222066
* GitHub ID: austinlikesfood
* Project: Assignment 6 – Device Driver
*
* File: Kuykendall_Austin_HW6_main.c
*
* Description: main interface that prompts user input for data
* after specifying the value of a key
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> // for 0_RDWR

#define SETKEY _IO('e', 2)
#define DECRYPT _IO('e', 1)
#define ENCRYPT _IO('e', 0)
#define MAX_INPUT 512 // set the max amount of user input to 512. 
                      // can handle exception to be <= 

int main(int argc, char *argv[]) {
  int userKey, fileDescriptor;
  // malloc max char gets rid of the seg fault
  char *val = malloc(MAX_INPUT);
  char *charInput = malloc(MAX_INPUT);
  
  char again[1];
  while (1) 
  {
    printf("starting driver\n");

    fileDescriptor = open("/dev/encryptor", O_RDWR);
    if (fileDescriptor < 0) 
    {
      perror("device failed");
      return fileDescriptor;
    }

    printf("choose an integer for a key from 1-50:");
    scanf("%d", &userKey);
    getc(stdin);

    // ioctl to set the key for the device driver
    ioctl(fileDescriptor, SETKEY, userKey);

    printf("enter desired phrase to be encrypted:\n");
    scanf("%[^\n]s", charInput);
    getc(stdin);

    printf("Length: %ld\n", strlen(charInput));
    printf("checking data parsed:\n%s\n", charInput);

    write(fileDescriptor, charInput, strlen(charInput));
    read(fileDescriptor, val, strlen(charInput));

    printf("\nencryption of text:\n%s\n\n", val);
    printf("reverse engineering to check decryption\n\n");

    // the ioctl method lets me interact with the decypher and kernel
    ioctl(fileDescriptor, DECRYPT);
    //standard read of cahr length
    read(fileDescriptor, val, strlen(charInput));
    printf("decrypted data: \n%s\n\n", val);
    // rbefore I close the program I want to enrypt the input again
    ioctl(fileDescriptor, ENCRYPT);
    close(fileDescriptor);

    printf("to reiterate, press y or n ");
    scanf("%1s", again);
    // when y is pressed we loop again to take in anotheri nput
    if (again[0] != 'y') 
    {
      break;
    }
    for (int i = 0; i < MAX_INPUT; i++) 
    {
      charInput[i] = '\0';
      val[i] = '\0';
    }
  }
  printf("\n end of assignment 6\n");
}