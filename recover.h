#ifndef _RECOVER_H_
#define _RECOVER_H_

int recover(int argc, char **argv);
int validate_usage(int argc, char **argv);
int printFSInfo(char **argv);
int listRootDir(char **argv);
int recoverFile(const char *disk_path, char *filename);
int recoverFileWithSha1(const char *disk_path, char *filename, char *sha1);
void hex_string_to_byte_array(const char *hex_string, unsigned char *byte_array, size_t length);

#endif
