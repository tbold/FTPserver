#ifndef _PASSIVEHELPERH__

#define _PASSIVEHELPERH__
#define NOT_PASSIVE -1
#define PASSIVE 1

int startPassiveMode();
int isInPassiveMode();
int getPassiveSocket();
void setPassiveMode(int);
int getPassivePort();
char* constructPasvMessage(char *);
void closePassiveSocket(int);

#endif