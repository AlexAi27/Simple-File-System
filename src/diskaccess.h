#pragma once

#include <cstdint>
#include "structs.h"

int readBlock(const unsigned char* disk, const uint32_t addr, unsigned char* block);

int writeBlock(unsigned char* disk, const uint32_t addr, const unsigned char* block);

int init(unsigned char* disk);

uint32_t mem2int(const unsigned char* mem);

int int2mem(const uint32_t addr, unsigned char* mem);

int readSuperBlock(const unsigned char* disk, SuperBlock &sb);

int writeSuperBlock(unsigned char* disk, SuperBlock &sb);

int readBitMap(const unsigned char* disk, unsigned char* bmp);

int writeBitMap(unsigned char* disk, const unsigned char* bmp);

int readINode(const unsigned char* disk, const uint32_t inum, INode &inode);

int writeINode(unsigned char* disk, const uint32_t inum, const INode &inode);

int allocINode(unsigned char* disk, uint32_t & inum);

int freeINode(unsigned char* disk, uint32_t inum);

int allocBlocks(unsigned char* disk, int cnt, uint32_t *addr);

int freeBlocks(unsigned char* disk, int cnt, const uint32_t *addr);

int calcBlocks(uint32_t bytes, uint32_t & blocks);

int readFileState(const unsigned char* disk, uint32_t inum, FileState& fs);

int readFile(const unsigned char* disk, uint32_t inum, File& file);

int writeFile(unsigned char* disk, uint32_t inum, File& file);

int file2dir(File& file, Dir& dir);

int dir2file(Dir& dir, File& file);

int readDir(const unsigned char* disk, uint32_t inum, Dir& dir);

int writeDir(unsigned char* disk, uint32_t inum, Dir& dir);
