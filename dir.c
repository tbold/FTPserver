#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dir.h"
#include <sys/stat.h>
#include <string.h>
#include <strings.h>

int loggedIn = NOT_LOGGED_IN;

int isLoggedIn() {
  return loggedIn;
}

void logoutClient() {
  loggedIn = 0;
}

/*
  If client is not already logged in, check username
  Returns
      LOGGED_IN success
      MISSING_PARAMETER missing username from client
      error codes otherwise
*/
int loginClient(int sockfd, char* args) {
  // if user already logged in
  if (isLoggedIn() == LOGGED_IN) 
    return ALREADY_LOGGED_IN;
  
  // if cannot read user name
  if (args == NULL) 
    return MISSING_PARAMETER;
  
  // user name must be ANON
  if(strcasecmp(args, "ANON") == 0) {
    loggedIn = LOGGED_IN;
    return loggedIn;
  } 
  return INVALID_USERNAME;
}

/*
  Returns
    DIRECTORY_CHANGED on success
    GENERIC_FAILURE on any type of error
*/
int changeToParentDir(char * mainDir) {
  char currDir[512];
  char parentDir[512];
  bzero(currDir, sizeof(currDir));
  bzero(parentDir, sizeof(parentDir));

  if (getcwd(currDir, sizeof(currDir)) == NULL) {
    // there was an error reading current directory
    return GENERIC_FAILURE;
  } 
  if (strcasecmp(currDir, mainDir) == 0) {
      // cannot cdup out of root directory
      // return success
      return DIRECTORY_CHANGED;
  } 
  int length = strlen(currDir);
  int i;
  // start loop from the back
  for (i = length - 1; i >= 0; i--) {
    // found parent directory
    if (currDir[i] == '/') {
      break;
    }
  }
  if (i == 0) {
    if (strcmp("..", mainDir) == 0) 
      return GENERIC_FAILURE;
  } 

  if (strncpy(parentDir, currDir, i) != NULL) {
    parentDir[i+1] = '\0';
    // parent checks out, change directory
    if (chdir(parentDir) == 0) {
      return DIRECTORY_CHANGED;
    }
    
  } 
  // couldn't copy string or change dir return error
  return GENERIC_FAILURE;
}

int changeWorkingDir(char * args) {
  if (args == NULL) 
    return MISSING_PARAMETER;
  
  if (check_path(args) < 0) 
    // couldn't change dir, return error
    return GENERIC_FAILURE;
  
  return DIRECTORY_CHANGED;
}

/*
  checks if given path contains ./ or ../
*/
int check_path(char *dir) {
  // If the CWD argument contains ./ or ../ then you should return 550.  
  if (strstr(dir, "../") != NULL || strstr(dir, "./") != NULL) {
      return -1;
  }
  return 1;
}

/* 
   This function takes the name of a directory and lists all the regular
   files and directoies in the directory. 

 */

int printDir(int fd, char * directory) {

  // Get resources to see if the directory can be opened for reading
  
  DIR * dir = NULL;
  
  dir = opendir(directory);
  if (!dir) return -1;
  
  struct dirent *dirEntry;
  int entriesPrinted = 0;
  
  for (dirEntry = readdir(dir);
       dirEntry;
       dirEntry = readdir(dir)) {
    if (dirEntry->d_type == DT_REG) {  // Regular file
      struct stat buf;

      if (stat(dirEntry->d_name, &buf) < 0) {
        return -1;
      }

	    dprintf(fd, "F    %-20s     %d\r\n", dirEntry->d_name, buf.st_size);
    } else if (dirEntry->d_type == DT_DIR) { // Directory
      dprintf(fd, "D        %s\r\n", dirEntry->d_name);
    } else {
      dprintf(fd, "U        %s\r\n", dirEntry->d_name);
    }
    entriesPrinted++;
  }
  
  // Release resources
  closedir(dir);
  return entriesPrinted;
}

   
