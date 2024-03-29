#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <openssl/sha.h>

#include "recover.h"

#define SHA_DIGEST_LENGTH 20
#define EMPTY_SHA1 "da39a3ee5e6b4b0d3255bfef95601890afd80709"

#pragma pack(push,1)
typedef struct BootEntry {
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short BPB_RootEntCnt;
    unsigned short BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short BPB_FATSz16;
    unsigned short BPB_SecPerTrk;
    unsigned short BPB_NumHeads;
    unsigned int BPB_HiddSec;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned short BPB_ExtFlags;
    unsigned short BPB_FSVer;
    unsigned int BPB_RootClus;
    unsigned short BPB_FSInfo;
    unsigned short BPB_BkBootSec;
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int BS_VolID;
    unsigned char BS_VolLab[11];
    unsigned char BS_FilSysType[8];
} BootEntry;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct DirEntry {
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth; 
    unsigned short DIR_CrtTime;     
    unsigned short DIR_CrtDate;    
    unsigned short DIR_LstAccDate;  
    unsigned short DIR_FstClusHI;
    unsigned short DIR_WrtTime;
    unsigned short DIR_WrtDate;
    unsigned short DIR_FstClusLO;
    unsigned int DIR_FileSize;
} DirEntry;
#pragma pack(pop)

typedef struct {
    int fd;
    size_t size;
    unsigned char *data;
} DiskData;
 
int recover(int argc, char **argv){

    // Milestone 1: validate usage
    if(validate_usage(argc,argv) != 0){
        return 1;
    }
    return 0;
}

int validate_usage(int argc, char **argv){
    char *error_message = 
    "Usage: ./nyufile disk <options>\n"
    "  -i                     Print the file system information.\n"
    "  -l                     List the root directory.\n"
    "  -r filename [-s sha1]  Recover a contiguous file.\n"
    "  -R filename -s sha1    Recover a possibly non-contiguous file.\n";

    if(argc < 2){
        printf("%s", error_message);
        return 1;
    }

    if (access(argv[1], F_OK) == -1) {
        printf("%s", error_message);
        return 1;
    }

    int option;
    int i_flag = 0, l_flag = 0, r_flag = 0, R_flag = 0, s_flag = 0;
    char *filename = NULL;
    char *sha1 = NULL;

    optind = 2;

    // Set opterr to 0 to disable default error messages generated by getopt()
    opterr = 0;

    while ((option = getopt(argc, argv, "ilr:R:s:")) != -1) {
        switch(option){
            case 'i':
                i_flag = 1;
                break;          
            case 'l':
                l_flag = 1;
                break;
            case 'r':
                if(optarg == NULL){
                    printf("%s", error_message);
                    return 1;
                }
                r_flag = 1;
                filename = optarg;
                break;
            case 's':
                if(optarg == NULL){
                    printf("%s", error_message);
                    return 1;
                }
                s_flag = 1;
                sha1 = optarg;
                break;
            case 'R':
                if(optarg == NULL){
                    printf("%s", error_message);
                    return 1;
                }
                R_flag = 1;
                filename = optarg;
                break;
            default:
                printf("%s", error_message);
                return 1;
        }
    }

    if(i_flag){
        // Milestone 2: Print the file system information.
        return printFSInfo(argv);
    }else if(l_flag){
        // Milestone 3: list the root directory.
        return listRootDir(argv);
    }else if(r_flag && !s_flag){
        // Recover a contiguous file without shal
        return recoverFile(argv[1], filename);
    }else if(r_flag && s_flag){
        // Recover a contiguous file shal
        recoverFileWithSha1(argv[1], filename, sha1);
    }else if(R_flag && s_flag){
        // Recover a possibly non-contiguous file.
        printf("Recover a possibly non-contiguous file.\n");
    }else if(R_flag && !s_flag){
        printf("%s", error_message);
        return 1;
    }

    return 0;
}

DiskData *getDiskData(const char  *disk_path){
    DiskData *disk_data = malloc(sizeof(DiskData));
    if (!disk_data) {
        perror("Error allocating memory");
        return NULL;
    }
    // Open file
    disk_data->fd = open(disk_path, O_RDWR);
    if (disk_data->fd < 0) {
        perror("Error opening file");
        free(disk_data);
        return NULL;
    }
    // Get file size
    struct stat st;
    if (fstat(disk_data->fd, &st) == -1) {
        perror("Error getting file stats");
        close(disk_data->fd);
        free(disk_data);
        return NULL;
    }
    disk_data->size = st.st_size;
    // Map file into memory
    disk_data->data = mmap(NULL, disk_data->size, PROT_READ | PROT_WRITE, MAP_SHARED, disk_data->fd, 0);
    if (disk_data->data == MAP_FAILED) {
        perror("Error mapping file");
        close(disk_data->fd);
        free(disk_data);
        return NULL;
    }

    return disk_data;
}

void freeDiskData(DiskData *disk_data) {
    if(disk_data) {
        if (disk_data->data != MAP_FAILED) {
            munmap(disk_data->data, disk_data->size);
        }
        if (disk_data->fd >= 0) {
            close(disk_data->fd);
        }
        free(disk_data);
    }
}

BootEntry *getBootEntry(const char *disk_path){
    BootEntry *boot_entry = (BootEntry *)malloc(sizeof(BootEntry));
    if (boot_entry == NULL) {
        perror("Failed to allocate memory for BootEntry.\n");
        return NULL;
    }
    DiskData *diskData = getDiskData(disk_path);
    if(diskData == NULL){
        return NULL;
    }
    memcpy(boot_entry, diskData->data, sizeof(BootEntry)); // Copy data into boot_entry
    freeDiskData(diskData);
    return boot_entry;
}

int printFSInfo(char **argv){
    BootEntry *bs = getBootEntry(argv[1]);
    if(bs != NULL){
        printf("Number of FATs = %d\n", bs->BPB_NumFATs);
        printf("Number of bytes per sector = %d\n", bs->BPB_BytsPerSec);
        printf("Number of sectors per cluster = %d\n", bs->BPB_SecPerClus);
        printf("Number of reserved sectors = %d\n", bs->BPB_RsvdSecCnt);
        free(bs);  
    }
    return 0;
}

int listRootDir(char **argv){
    BootEntry *bs = getBootEntry(argv[1]);
    if(bs == NULL){
        return 1;
    }
    unsigned int first_data_sector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_FATSz32);
    unsigned int first_root_dir_cluster = bs->BPB_RootClus;
    unsigned int root_dir_size = bs->BPB_BytsPerSec * bs->BPB_SecPerClus;

    DiskData *diskData = getDiskData(argv[1]);
    if(diskData == NULL){
        free(bs);
        return 1;
    }

    int entry_count = 0;
    // Follow the cluster chain in the FAT
    unsigned int current_cluster = first_root_dir_cluster;
    while (current_cluster < 0x0FFFFFF7) {
        unsigned int first_sector = first_data_sector + ((current_cluster - 2) * bs->BPB_SecPerClus);
        unsigned char *dir_data = diskData->data + (first_sector * bs->BPB_BytsPerSec);
        
        for(unsigned int i = 0; i< root_dir_size; i+= sizeof(DirEntry)){
            DirEntry *dirEntry = (DirEntry *)(dir_data + i);

            // skip deleted directory
            if(dirEntry->DIR_Name[0] == 0x00 || dirEntry->DIR_Name[0] == 0xE5){
                continue;
            }
            // skep LFN
            if(dirEntry->DIR_Attr == 0x0F){
                continue;
            }
            // print the 8.3 filename
            for(int j = 0; j< 8; j++){
                if(dirEntry->DIR_Name[j] != ' '){
                    printf("%c",dirEntry->DIR_Name[j]);
                }
            }

            //if the entry is a directory, you should append a / indicator.
            if(dirEntry->DIR_Attr == 0x10){
                // Directory entry
                printf("/ (starting cluster = %u)\n", dirEntry->DIR_FstClusLO);
            }else{
                // Non Empty Entry
                    for(int k = 8; k< 11; k++){
                        if(dirEntry->DIR_Name[k] != ' '){
                            if(k == 8){
                                printf(".");
                            }
                            printf("%c",dirEntry->DIR_Name[k]);
                        }
                    }
                // File entry
                printf(" (size = %u", dirEntry->DIR_FileSize);
                if (dirEntry->DIR_FileSize != 0) {
                    printf(", starting cluster = %u", dirEntry->DIR_FstClusLO);
                }
                printf(")\n");
            }
            entry_count++;
        }
        // Find the next cluster in the FAT
        unsigned int fat_offset = bs->BPB_RsvdSecCnt * bs->BPB_BytsPerSec + current_cluster * 4;
        current_cluster = *((unsigned int *)(diskData->data + fat_offset)) & 0x0FFFFFFF;
    }
    printf("Total number of entries = %u\n", entry_count);
    // Clean up
    freeDiskData(diskData);
    free(bs);
    return 0;
}

int recoverFile(const char *disk_path, char *filename){
    BootEntry *bs = getBootEntry(disk_path);
    if(bs == NULL){
        return 1;
    }
    unsigned int first_data_sector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_FATSz32);
    unsigned int first_root_dir_cluster = bs->BPB_RootClus;
    unsigned int root_dir_size = bs->BPB_BytsPerSec * bs->BPB_SecPerClus;

    DiskData *diskData = getDiskData(disk_path);
    if(diskData == NULL){
        free(bs);
        return 1;
    }

    int fileDeleted = 0;
    DirEntry *newDirEntry = NULL;

    // Follow the cluster chain in the FAT
    unsigned int current_cluster = first_root_dir_cluster;
    while (current_cluster < 0x0FFFFFF7) {
        unsigned int first_sector = first_data_sector + ((current_cluster - 2) * bs->BPB_SecPerClus);
        unsigned char *dir_data = diskData->data + (first_sector * bs->BPB_BytsPerSec);
        
        for(unsigned int i = 0; i< root_dir_size; i+= sizeof(DirEntry)){
            DirEntry *dirEntry = (DirEntry *)(dir_data + i);

            // skip current directory
            if(dirEntry->DIR_Name[0] == 0x00){
                continue;
            }
            // skip LFN
            if(dirEntry->DIR_Attr == 0x0F){
                continue;
            }

            // skip direcotry files
            if(dirEntry->DIR_Attr == 0x10){
                continue;
            }

            char matchFilename[12];
            int nameEnd = 0;
            // Copy the filename part
            for(int j = 0; j< 8; j++){
                if(dirEntry->DIR_Name[j] != ' '){
                    matchFilename[nameEnd++] = dirEntry->DIR_Name[j];
                }
            }

            // Copy the extension part
            for(int k = 8; k < 11; k++) {
                if(dirEntry->DIR_Name[k] != ' '){
                    if(k == 8){
                        // Add the '.' character
                        matchFilename[nameEnd++] = '.';
                    }
                    matchFilename[nameEnd++] = dirEntry->DIR_Name[k];
                }
            }
            matchFilename[nameEnd] = '\0';

            if(dirEntry->DIR_Name[0] == 0xE5 && strcmp(matchFilename + 1, filename +1) == 0){
                fileDeleted++;
                newDirEntry = dirEntry;
            }
        }
        // Find the next cluster in the FAT
        unsigned int fat_offset = bs->BPB_RsvdSecCnt * bs->BPB_BytsPerSec + current_cluster * 4;
        current_cluster = *((unsigned int *)(diskData->data + fat_offset)) & 0x0FFFFFFF;
    }

    if(fileDeleted == 0){
        printf("%s: file not found\n", filename);
    }else if(fileDeleted > 1){
        printf("%s: multiple candidates found\n", filename);
    }else{
        newDirEntry->DIR_Name[0] = filename[0];
        if(newDirEntry->DIR_FileSize != 0){
            unsigned int starting_cluster = newDirEntry->DIR_FstClusLO;
            unsigned int current_cluster = starting_cluster;
            unsigned int next_cluster = 0;
            unsigned int end_of_file = 0x0FFFFFFF;

            // bs->BPB_BytsPerSec - 1 to ensure we got the near whole integer
            unsigned int num_clusters = (newDirEntry->DIR_FileSize + bs->BPB_BytsPerSec * bs->BPB_SecPerClus - 1) / (bs->BPB_BytsPerSec * bs->BPB_SecPerClus);
            for (unsigned int fat_num = 0; fat_num < bs->BPB_NumFATs; fat_num++) {
                // Traverse the chain of clusters in the FAT
                for (unsigned int i = 0; i < num_clusters; i++) {
                    // Get the offset of the current cluster in the FAT
                    unsigned int fat_offset = (bs->BPB_RsvdSecCnt + fat_num * bs->BPB_FATSz32) * bs->BPB_BytsPerSec + current_cluster * 4;

                    // Update the FAT entry to point to the next cluster in the chain
                    if (i < num_clusters - 1) {
                        next_cluster = current_cluster + 1;
                        memcpy(diskData->data + fat_offset, &next_cluster, sizeof(next_cluster));
                    }
                    // Mark the final cluster in the chain as end-of-file
                    else {
                        memcpy(diskData->data + fat_offset, &end_of_file, sizeof(end_of_file));
                    }

                    // Update the current cluster to the next cluster
                    current_cluster = next_cluster;
                }
                // Reset the current cluster to the starting cluster for the next FAT
                current_cluster = starting_cluster;
            }
        }
        printf("%s: successfully recovered\n", filename);
    }

    // Clean up
    freeDiskData(diskData);
    free(bs);
    return 0;
}

int recoverFileWithSha1(const char *disk_path, char *filename, char *sha1){
    BootEntry *bs = getBootEntry(disk_path);
    if(bs == NULL){
        return 1;
    }
    unsigned int first_data_sector = bs->BPB_RsvdSecCnt + (bs->BPB_NumFATs * bs->BPB_FATSz32);
    unsigned int first_root_dir_cluster = bs->BPB_RootClus;
    unsigned int root_dir_size = bs->BPB_BytsPerSec * bs->BPB_SecPerClus;

    DiskData *diskData = getDiskData(disk_path);
    if(diskData == NULL){
        free(bs);
        return 1;
    }

    int fileDeleted = 0;
    DirEntry *newDirEntry = NULL;
    unsigned char sha1_byte_array[SHA_DIGEST_LENGTH];
    hex_string_to_byte_array(sha1, sha1_byte_array, SHA_DIGEST_LENGTH);


    // Follow the cluster chain in the FAT
    unsigned int current_cluster = first_root_dir_cluster;
    while (current_cluster < 0x0FFFFFF7) {
        unsigned int first_sector = first_data_sector + ((current_cluster - 2) * bs->BPB_SecPerClus);
        unsigned char *dir_data = diskData->data + (first_sector * bs->BPB_BytsPerSec);
        
        for(unsigned int i = 0; i< root_dir_size; i+= sizeof(DirEntry)){
            DirEntry *dirEntry = (DirEntry *)(dir_data + i);

            // skip current directory
            if(dirEntry->DIR_Name[0] == 0x00){
                continue;
            }
            // skip LFN
            if(dirEntry->DIR_Attr == 0x0F){
                continue;
            }

            // skip direcotry files
            if(dirEntry->DIR_Attr == 0x10){
                continue;
            }

            char matchFilename[12];
            int nameEnd = 0;
            // Copy the filename part
            for(int j = 0; j< 8; j++){
                if(dirEntry->DIR_Name[j] != ' '){
                    matchFilename[nameEnd++] = dirEntry->DIR_Name[j];
                }
            }

            // Copy the extension part
            for(int k = 8; k < 11; k++) {
                if(dirEntry->DIR_Name[k] != ' '){
                    if(k == 8){
                        // Add the '.' character
                        matchFilename[nameEnd++] = '.';
                    }
                    matchFilename[nameEnd++] = dirEntry->DIR_Name[k];
                }
            }
            matchFilename[nameEnd] = '\0';

            if(dirEntry->DIR_Name[0] == 0xE5){
                if(dirEntry->DIR_FileSize != 0){
                    unsigned int starting_cluster = dirEntry->DIR_FstClusLO;
                    unsigned int dir_offset = (first_data_sector + (starting_cluster - 2) * bs->BPB_SecPerClus) * bs->BPB_BytsPerSec;
                    unsigned char* file_content = (unsigned char*) malloc(dirEntry->DIR_FileSize * sizeof(unsigned char));

                    // Read the entire file content with a single memcpy call
                    memcpy(file_content, diskData->data + dir_offset, dirEntry->DIR_FileSize);
                    unsigned char computed_hash[SHA_DIGEST_LENGTH];
                    SHA1(file_content, dirEntry->DIR_FileSize, computed_hash);
                    if(strcmp(matchFilename + 1, filename +1) == 0 && memcmp(computed_hash, sha1_byte_array, SHA_DIGEST_LENGTH) == 0){
                        fileDeleted++;
                        newDirEntry = dirEntry;
                    }
                    free(file_content);
                }else if(strcmp(matchFilename + 1, filename +1) == 0 && strcmp(sha1, EMPTY_SHA1) == 0){
                    fileDeleted++;
                    newDirEntry = dirEntry;
                }
            }

        }
        // Find the next cluster in the FAT
        unsigned int fat_offset = bs->BPB_RsvdSecCnt * bs->BPB_BytsPerSec + current_cluster * 4;
        current_cluster = *((unsigned int *)(diskData->data + fat_offset)) & 0x0FFFFFFF;
    }

    if(fileDeleted == 0){
        printf("%s: file not found\n", filename);
    }else{
        newDirEntry->DIR_Name[0] = filename[0];
        if(newDirEntry->DIR_FileSize != 0){
            unsigned int starting_cluster = newDirEntry->DIR_FstClusLO;
            unsigned int current_cluster = starting_cluster;
            unsigned int next_cluster = 0;
            unsigned int end_of_file = 0x0FFFFFFF;

            // bs->BPB_BytsPerSec - 1 to ensure we got the near whole integer
            unsigned int num_clusters = (newDirEntry->DIR_FileSize + bs->BPB_BytsPerSec * bs->BPB_SecPerClus - 1) / (bs->BPB_BytsPerSec * bs->BPB_SecPerClus);
            for (unsigned int fat_num = 0; fat_num < bs->BPB_NumFATs; fat_num++) {
                // Traverse the chain of clusters in the FAT
                for (unsigned int i = 0; i < num_clusters; i++) {
                    // Get the offset of the current cluster in the FAT
                    unsigned int fat_offset = (bs->BPB_RsvdSecCnt + fat_num * bs->BPB_FATSz32) * bs->BPB_BytsPerSec + current_cluster * 4;

                    // Update the FAT entry to point to the next cluster in the chain
                    if (i < num_clusters - 1) {
                        next_cluster = current_cluster + 1;
                        memcpy(diskData->data + fat_offset, &next_cluster, sizeof(next_cluster));
                    }
                    // Mark the final cluster in the chain as end-of-file
                    else {
                        memcpy(diskData->data + fat_offset, &end_of_file, sizeof(end_of_file));
                    }

                    // Update the current cluster to the next cluster
                    current_cluster = next_cluster;
                }
                // Reset the current cluster to the starting cluster for the next FAT
                current_cluster = starting_cluster;
            }
        }
        printf("%s: successfully recovered with SHA-1\n", filename);
    }

    // Clean up
    freeDiskData(diskData);
    free(bs);
    return 0;
}

void hex_string_to_byte_array(const char *hex_string, unsigned char *byte_array, size_t length) {
    for (size_t i = 0; i < length; i++) {
        sscanf(hex_string + 2 * i, "%02x", (unsigned int *)&byte_array[i]);
    }
}

