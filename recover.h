#ifndef _RECOVER_H_
#define _RECOVER_H_

int recover(int argc, char **argv);
int validate_usage(int argc, char **argv);
int printFSInfo(char **argv);
int listRootDir(char **argv);
int recoverFile(const char *disk_path, char *filename);

#endif
