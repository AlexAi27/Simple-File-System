#pragma once

#include <cstdint>
#include <vector>
#include <string>

#define DISKSIZE 0x1000000 //in Byte, 16MB
#define BLOCKSIZE 0x400 //in Byte, 1KB
#define SUPERBLOCKOFFSET 0x0 //in block
#define BITMAPBLOCKOFFSET 0x1 //in block
#define DATABLOCKOFFSET 0x337 //in block
#define MAXADDR 0x4000 //Address should be smaller than this limit
#define ADDRESSSIZE 3 //in Byte

struct SuperBlock {
	uint32_t inode_occupied;
	uint32_t block_occupied;
	uint32_t files_cnt;
	uint32_t directory_cnt;
};

#define BITMAPSIZE 0x800
#define MAXBLOCK 0x4000

#define INODESIZE 53
#define INODESPERBLOCK 19
#define INODEBLOCKS 0x334
#define INODEBLOCKOFFSET 0x3 // in block
#define MAXINODE 0x3CDC
#define ISFILE 0x1
#define ISDIR 0x0
struct FileState {
	int32_t file_mode; //directory or file
	uint32_t create_time;
	uint32_t modify_time;
	uint32_t block_occupied;
	uint32_t data_info; //data size
};
struct INode : public FileState {
	uint32_t address_cnt;
	uint32_t direct_addr[10];
	uint32_t indirect_addr;
}; //3*10 + 3


#define DATABLOCKS 0x3CC9
#define ADDRESSPERBLOCK 0x155
#define MAXADDRESSPERINODE 0x15F

struct DataBlock {
	char data[BLOCKSIZE];
};



typedef std::vector<unsigned char> FileContent;
struct File : public FileState{
	FileContent data;
};


#define MAXFILEPERDIR 0x15F0
typedef std::pair<std::string, uint32_t> FileNode;
typedef std::vector<FileNode> DirectoryContent;
struct Dir : public FileState{
	DirectoryContent data;
};

typedef std::vector<std::string> CMD;