#ifndef _DIRH__

#define _DIRH__
#define LOGGED_IN 1
#define DIRECTORY_CHANGED 2

#define ALREADY_LOGGED_IN -1
#define MISSING_PARAMETER -2
#define NOT_LOGGED_IN -3
#define INVALID_USERNAME -4
#define CLIENT_QUIT -5
#define GENERIC_FAILURE -6

int printDir(int, char*);
int loginClient(int, char*);
int isLoggedIn();
void logoutClient();
int changeToParentDir(char *);
int changeWorkingDir(char *);
int check_path(char *);
#endif
