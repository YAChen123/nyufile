# 2023 Spring Operating Systems --- Lab 4: File Recovery

## Introduction
FAT32 has been around for over 25 years. Because of its simplicity, it is the most widely compatible file system. 
Although recent computers have adopted newer file systems, FAT32 (and its variant, exFAT) is still dominant in SD cards and USB flash drives due to its compatibility.

Have you ever accidentally deleted a file? Do you know that it could be recovered? In this lab, you will build a FAT32 file recovery tool called **Need You to Undelete my FILE**, or `nyufile` for short.

## Objectives
Through this lab, you will:

* Learn the internals of the FAT32 file system.
* Learn how to access and recover files from a raw disk.
* Get a better understanding of key file system concepts.
* Be a better C programmer. Learn how to write code that manipulates data at the byte level and understand the alignment issue.

## Overview
In this lab, you will work on the data stored in the FAT32 file system **directly**, without the OS file system support. You will implement a tool that recovers a deleted file specified by the user.

## Working with a FAT32 disk image
1. create an empty file of a certain size
2. format the disk with FAT32
3. verify the file system information
4. mount the file system
5. play with the file system
6. unmount the file system
7. examine the file system
   
## Your tasks

### Milestone 1: validate usage
### Milestone 2: print the file system information
### Milestone 3: list the root directory
### Milestone 4: recover a small file
### Milestone 5: recover a large contiguously-allocated file
### Milestone 6: detect ambiguous file recovery requests
### Milestone 7: recover a contiguously-allocated file with SHA-1 hash
### Milestone 8: recover a non-contiguously allocated file

## FAT32 data structures
For your convenience, here are some data structures that you can copy and paste. Please refer to the lecture slides and FAT: General Overview of On-Disk Format for details on the FAT32 file system layout.
### Boot sector

```
#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)
```

### Directory entry
```
#pragma pack(push,1)
typedef struct DirEntry {
  unsigned char  DIR_Name[11];      // File name
  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day
  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day
  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)
```

Lab Website: https://cs.nyu.edu/courses/spring23/CSCI-GA.2250-002/nyufile
