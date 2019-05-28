#include "diskaccess.h"
#include "structs.h"
#include "error.h"
#include <cstring>
#include <iostream>
#include <ctime>


int readBlock(const unsigned char* disk, const uint32_t addr, unsigned char* block) {
	if (addr >= MAXADDR) return ILLEGALADDRESS;
	memcpy(block, disk + addr * BLOCKSIZE, BLOCKSIZE);
	return 0;
}

int writeBlock(unsigned char* disk, const uint32_t addr, const unsigned char* block) {
	if (addr >= MAXADDR) return ILLEGALADDRESS;
	memcpy(disk + addr * BLOCKSIZE, block, BLOCKSIZE);
	return 0;
}

int init(unsigned char* disk) {
	memset(disk, 0xff, DISKSIZE);

	SuperBlock sb;
	sb.inode_occupied = 0;
	sb.block_occupied = DATABLOCKOFFSET;
	sb.files_cnt = 0;
	sb.directory_cnt = 0;
	writeSuperBlock(disk, sb);

	unsigned char bmp[BITMAPSIZE];
	memset(bmp, 0, sizeof(bmp));
	for (int i = 0; i < DATABLOCKOFFSET / 8; i++) {
		bmp[i] = 0xff;
	}
	unsigned char mask = 0;
	for (int i = 0; i < DATABLOCKOFFSET % 8; i++) {
		mask |= 128 >> i;
	}
	bmp[DATABLOCKOFFSET / 8] = mask;
	writeBitMap(disk, bmp);

	uint32_t inum;
	int ret = allocINode(disk, inum);
	if (ret != 0) {
		std::cerr << "init error: " << ret << std::endl;
		return ret;
	}

	Dir dir;
	dir.file_mode = ISDIR;
	dir.create_time = time(NULL);
	dir.modify_time = dir.create_time;
	dir.block_occupied = 1;
	dir.data.push_back(std::make_pair(std::string("."), inum));
	dir.data.push_back(std::make_pair(std::string(".."), inum));
	ret = writeDir(disk, inum, dir);
	if (ret != 0) {
		std::cerr << "init error: " << ret << std::endl;
		freeINode(disk, inum);
		return ret;
	}
}

uint32_t mem2int(const unsigned char* mem) {
	uint32_t tmp = 0;
	for (int i = 0; i < ADDRESSSIZE; i++) {
		tmp = (tmp << 8) | mem[i];
	}
	return tmp;
}

int int2mem(const uint32_t addr, unsigned char* mem) {
	uint32_t tmp = addr;
	for (int i = 0; i < ADDRESSSIZE; i++) {
		mem[i] = (tmp & 0xff0000) >> 16;
		tmp <<= 8;
	}
	return 0;
}

int readSuperBlock(const unsigned char* disk, SuperBlock &sb) {
	unsigned char block[BLOCKSIZE];
	readBlock(disk, SUPERBLOCKOFFSET, block);
	memcpy(&sb, block, sizeof(SuperBlock));
}

int writeSuperBlock(unsigned char* disk, SuperBlock &sb) {
	unsigned char block[BLOCKSIZE];
	memcpy(block, &sb, sizeof(SuperBlock));
	writeBlock(disk, SUPERBLOCKOFFSET, block);
}

int readBitMap(const unsigned char* disk, unsigned char* bmp) {
	readBlock(disk, BITMAPBLOCKOFFSET, bmp);
	readBlock(disk, BITMAPBLOCKOFFSET + 1, bmp + BLOCKSIZE);
}

int writeBitMap(unsigned char* disk, const unsigned char* bmp) {
	writeBlock(disk, BITMAPBLOCKOFFSET, bmp);
	writeBlock(disk, BITMAPBLOCKOFFSET + 1, bmp + BLOCKSIZE);
}

int readINode(const unsigned char* disk, const uint32_t inum, INode &inode) {
	if (inum >= MAXINODE) return INUMERROR;
	int block_num = inum / INODESPERBLOCK + INODEBLOCKOFFSET;
	int offset = inum % INODESPERBLOCK * INODESIZE;
	
	unsigned char block[BLOCKSIZE];
	readBlock(disk, block_num, block);

	memcpy(&inode, block + offset, sizeof(FileState));
	unsigned char buff[INODESIZE - sizeof(FileState)];
	memcpy(buff, block + (offset + sizeof(FileState)), INODESIZE - sizeof(FileState));

	int cnt = 0, ptr = 0;
	if (buff[ptr] == 0xef) {
		inode.address_cnt = 11;
		return 0;
	}
	while (cnt < 10 && buff[ptr] != 0xff) {
		inode.direct_addr[cnt++] = mem2int(buff+ptr);
		ptr += ADDRESSSIZE;
	}
	inode.address_cnt = cnt;
	if (cnt == 10) {
		inode.indirect_addr = mem2int(buff+ptr);
	}
	return 0;
}

int writeINode(unsigned char* disk, const uint32_t inum, const INode &inode) {
	if (inum >= MAXINODE) return INUMERROR;

	int block_num = inum / INODESPERBLOCK + INODEBLOCKOFFSET;
	int offset = inum % INODESPERBLOCK * INODESIZE;

	unsigned char block[BLOCKSIZE];
	readBlock(disk, block_num, block);

	unsigned char tmp[INODESIZE];
	unsigned char *buff = tmp;
	memcpy(buff, &inode, sizeof(FileState));
	buff += sizeof(FileState);
	int cnt = inode.address_cnt;
	if (cnt == 11) { //特殊操作，将inode标记为已被分配
		int2mem(0xefefefef, buff);
		memcpy(block + offset, tmp, INODESIZE);
		writeBlock(disk, block_num, block);
		return 0;
	}
	for (int i = 0; i < cnt; i++) {
		int2mem(inode.direct_addr[i], buff+i*ADDRESSSIZE);
	}
	if (cnt < 10) {
		int2mem(0xffffff, buff+cnt*ADDRESSSIZE);
	} else {
		int2mem(inode.indirect_addr, buff+cnt*ADDRESSSIZE);
	}
	memcpy(block + offset, tmp, INODESIZE);
	writeBlock(disk, block_num, block);
	return 0;
}

int allocINode(unsigned char* disk, uint32_t & inum) {
	SuperBlock sb;
	readSuperBlock(disk, sb);
	if (sb.inode_occupied == MAXINODE) return NOVALIDINODE;
	sb.inode_occupied++;
	writeSuperBlock(disk, sb);

	unsigned char block[BLOCKSIZE];
	for (int i = 0; i < INODEBLOCKS; i++) {
		readBlock(disk, i + INODEBLOCKOFFSET, block);
		for (int j = 0; j < INODESPERBLOCK; j++) {
			if (block[j*INODESIZE+sizeof(FileState)] == 0xff) {
				inum = i * INODESPERBLOCK + j;
				INode inode;
				inode.address_cnt = 11;
				writeINode(disk, inum, inode);
				return 0;
			}
		}
	}
	return NOVALIDINODE;
}

int freeINode(unsigned char* disk, uint32_t inum) {
	if (inum >= MAXINODE) return INUMERROR;
	INode inode;
	readINode(disk, inum, inode);
	if (inode.address_cnt == 0) return 0;
	if (inode.address_cnt == 11) {
		inode.address_cnt = 0;
	}
	
	std::vector<uint32_t> addr;
	for (int i = 0; i < inode.address_cnt; i++) {
		addr.push_back(inode.direct_addr[i]);
	}
	if (inode.address_cnt == 10 && inode.indirect_addr != 0xffffff) {
		unsigned char block[BLOCKSIZE];
		readBlock(disk, inode.indirect_addr, block);
		int ptr = 0;
		while (ptr < ADDRESSPERBLOCK) {
			if (block[ptr * ADDRESSSIZE] == 0xff) {
				break;
			}
			uint32_t addr_tmp = mem2int(block + ptr * ADDRESSSIZE);
			addr.push_back(addr_tmp);
			ptr++;
		}
		addr.push_back(inode.indirect_addr);
	}
	if (addr.size() != 0) {
		freeBlocks(disk, addr.size(), addr.data());
	}
	inode.address_cnt = 0;

	SuperBlock sb;
	readSuperBlock(disk, sb);
	if (sb.inode_occupied == 0) return FREEINODEERROR;
	sb.inode_occupied--;
	writeSuperBlock(disk, sb);
	writeINode(disk, inum, inode);
	return 0;
}

int allocBlocks(unsigned char* disk, int cnt, uint32_t *addr) {
	SuperBlock sb;
	readSuperBlock(disk, sb);
	if (sb.block_occupied > MAXBLOCK - cnt) return NOVALIDBLOCK;
	sb.block_occupied += cnt;
	writeSuperBlock(disk, sb);

	unsigned char bmp[BITMAPSIZE];
	readBitMap(disk, bmp);
	int addr_cnt = 0;
	for (int i = DATABLOCKOFFSET / 8; i < BITMAPSIZE; i++) {
		if (addr_cnt == cnt) break;
		if (bmp[i] == 0xff) continue;
		for (int j = 0; j < 8; j++) {
			if (((128 >> j) & bmp[i]) == 0) {
				addr[addr_cnt++] = i * 8 + j;
				bmp[i] ^= 128 >> j;
			}
			if (addr_cnt == cnt) break;
		}
	}
	writeBitMap(disk, bmp);
	return 0;
}

int freeBlocks(unsigned char* disk, int cnt, const uint32_t *addr) {
	SuperBlock sb;
	readSuperBlock(disk, sb);
	sb.block_occupied -= cnt;
	writeSuperBlock(disk, sb);

	unsigned char bmp[BITMAPSIZE];
	readBitMap(disk, bmp);
	for (int i = 0; i < cnt; i++) {
		bmp[addr[i] / 8] ^= 128 >> (i % 8);
	}
	writeBitMap(disk, bmp);
	return 0;
}

int calcBlocks(uint32_t bytes, uint32_t & blocks) {
	blocks = bytes >> 10;
	if (blocks << 10 != bytes) blocks++;
	return 0;
}

int readFileState(const unsigned char* disk, uint32_t inum, FileState& fs) {
	INode inode;
	int ret = readINode(disk, inum, inode);
	if (ret != 0 || inode.address_cnt == 0) {
		std::cerr << "readFile error: " << ret << std::endl;
		return -1;
	}

	memcpy(&fs, &inode, sizeof(FileState));
	return 0;
}

int readFile(const unsigned char* disk, uint32_t inum, File& file) {
	INode inode;
	int ret = readINode(disk, inum, inode);
	if (ret != 0 || inode.address_cnt == 0) {
		std::cerr << "readFile error: " << ret << std::endl;
		return -1;
	}

	std::vector<uint32_t> addr;
	for (int i = 0; i < inode.address_cnt; i++) {
		addr.push_back(inode.direct_addr[i]);
	}
	if (inode.address_cnt == 10 && inode.indirect_addr != 0xffffff) {
		unsigned char block[BLOCKSIZE];
		readBlock(disk, inode.indirect_addr, block);
		int ptr = 0;
		while (ptr < ADDRESSPERBLOCK) {
			if (block[ptr * ADDRESSSIZE] == 0xff) {
				break;
			}
			uint32_t addr_tmp = mem2int(block + ptr * ADDRESSSIZE);
			addr.push_back(addr_tmp);
			ptr++;
		}
	}

	uint32_t data_size = addr.size() * BLOCKSIZE;
	unsigned char *data = (unsigned char*) malloc(data_size);
	for (int i = 0; i < addr.size(); i++) {
		readBlock(disk, addr[i], data + i * BLOCKSIZE);
	}
	memcpy(&file, &inode, sizeof(FileState));
	file.data.resize(file.data_info);
	memcpy(file.data.data(), data, file.data_info);
	free(data);
	return 0;
}

int writeFile(unsigned char* disk, uint32_t inum, File& file) {
	INode inode;
	int ret = readINode(disk, inum, inode);
	if (ret != 0) {
		std::cerr << "writeFile error: " << ret << std::endl;
		return -1;
	}

	std::vector<uint32_t> addr;
	if (inode.address_cnt != 11) {
		for (int i = 0; i < inode.address_cnt; i++) {
			addr.push_back(inode.direct_addr[i]);
		}
	}
	unsigned char block[BLOCKSIZE];
	if (inode.address_cnt == 10 && inode.indirect_addr != 0xffffff) {
		readBlock(disk, inode.indirect_addr, block);
		int ptr = 0;
		while (ptr < ADDRESSPERBLOCK) {
			if (block[ptr * ADDRESSSIZE] == 0xff) {
				break;
			}
			uint32_t addr_tmp = mem2int(block + ptr * ADDRESSSIZE);
			addr.push_back(addr_tmp);
		}
	}

	uint32_t data_size = file.data.size();
	uint32_t blocks;
	calcBlocks(data_size, blocks);
	file.block_occupied = blocks;
	if (addr.size() > blocks) {
		freeBlocks(disk, addr.size() - blocks, addr.data() + blocks);
		if (addr.size() > 10 && blocks <= 10) {
			freeBlocks(disk, 1, &inode.indirect_addr);
		}

		if (blocks < 10) {
			inode.address_cnt = blocks;
		} else if (blocks == 10) {
			inode.address_cnt = blocks;
			inode.indirect_addr = 0xffffff;
		} else {
			readBlock(disk, inode.indirect_addr, block);
			int2mem(0xffffff, block + ((blocks - 10) * ADDRESSSIZE));
			writeBlock(disk, inode.indirect_addr, block);
		}

		unsigned char *data = (unsigned char *) malloc(blocks * BLOCKSIZE);
		memcpy(&inode, &file, sizeof(FileState));
		writeINode(disk, inum, inode);
		memcpy(data, file.data.data(), file.data.size());
		for (int i = 0; i < blocks; i++) {
			writeBlock(disk, addr[i], data + i * BLOCKSIZE);
		}
		free(data);
	} else if (addr.size() == blocks) {
		unsigned char *data = (unsigned char *) malloc(blocks * BLOCKSIZE);
		memcpy(&inode, &file, sizeof(FileState));
		writeINode(disk, inum, inode);
		memcpy(data, file.data.data(), file.data.size());
		for (int i = 0; i < blocks; i++) {
			writeBlock(disk, addr[i], data + i * BLOCKSIZE);
		}
		free(data);
	} else {
		if (blocks > MAXADDRESSPERINODE) {
			std::cerr << "writeFile error: " << FILESIZEEXCE << std::endl;
			return FILESIZEEXCE;
		}
		int origin_addr_num = addr.size();
		addr.resize(blocks + 1);
		ret = allocBlocks(disk, blocks - origin_addr_num, addr.data() + origin_addr_num);
		if (ret != 0) {
			std::cerr << "allocBlocks error: " << ret << std::endl;
			return ret;
		}

		if (addr.size() > 11 && origin_addr_num <= 10) {
			ret = allocBlocks(disk, 1, addr.data() + blocks);
			if (ret != 0) {
				std::cerr << "allocBlocks error: " << ret << std::endl;
				freeBlocks(disk, blocks - origin_addr_num, addr.data() + origin_addr_num);
				return ret;
			}
		} else {
			addr[blocks] = inode.indirect_addr;
		}

		unsigned char *data = (unsigned char *) malloc(blocks * BLOCKSIZE);
		memcpy(&inode, &file, sizeof(FileState));
		memcpy(data, file.data.data(), file.data.size());
		for (int i = 0; i < blocks; i++) {
			writeBlock(disk, addr[i], data + i * BLOCKSIZE);
		}
		free(data);
		if (blocks > 10) {
			for (int i = 10; i < blocks; i++) {
				int2mem(addr[i], block + (i-10) * ADDRESSSIZE);
			}
			if (blocks - 10 != ADDRESSPERBLOCK) int2mem(0xffffff, block + (blocks - 10) * ADDRESSSIZE);
			writeBlock(disk, addr[blocks], block);
		}

		inode.address_cnt = blocks < 10 ? blocks : 10;
		for (int i = 0; i < inode.address_cnt; i++) {
			inode.direct_addr[i] = addr[i];
		}
		if (blocks > 10) {
			inode.indirect_addr = addr[blocks];
		} else {
			inode.indirect_addr = 0xffffff;
		}
		writeINode(disk, inum, inode);
	}
	return 0;
}

int file2dir(File& file, Dir& dir) {
	if (file.file_mode != ISDIR) {
		return DIRSTRUCTERROR;
	}
	memcpy(&dir, &file, sizeof(FileState));
	std::string str;
	uint32_t inum;
	for (int i = 0; i < file.data.size(); i += 64) {
		str = ((char *)file.data.data()) + i;
		memcpy(&inum, file.data.data() + (i + 60), 4);
		dir.data.push_back(std::make_pair(str, inum));
	}
	return 0;
}

int dir2file(Dir& dir, File& file) {
	file.data.resize(dir.data.size() * 64);
	memcpy(&file, &dir, sizeof(FileState));
	file.data_info = file.data.size();

	for (int i = 0; i < dir.data.size(); i++) {
		strcpy((char*)(file.data.data() + (i * 64)), dir.data[i].first.c_str());
		memcpy(file.data.data() + (i * 64 + 60), &(dir.data[i].second), 4);
	}
	return 0;
}

int readDir(const unsigned char *disk, uint32_t inum, Dir& dir) {
	dir.data.clear();
	File file;
	int ret = readFile(disk, inum, file);
	if (ret != 0) {
		std::cerr << "readDir error: " << ret << std::endl;
		return ret;
	}
	ret = file2dir(file, dir);
	if (ret != 0) {
		std::cerr << "readDir error: " << ret << std::endl;
		return ret;
	}
	return 0;
}

int writeDir(unsigned char *disk, uint32_t inum, Dir& dir) {
	File file;
	dir2file(dir, file);
	int ret = writeFile(disk, inum, file);
	if (ret != 0) {
		std::cerr << "writeDir error: " << ret << std::endl;
		return ret;
	}
	return 0;
}