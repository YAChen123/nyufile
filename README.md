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

Lab Website: https://cs.nyu.edu/courses/spring23/CSCI-GA.2250-002/nyufile
