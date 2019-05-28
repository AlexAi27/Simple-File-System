#include "diskaccess.h"
#include "comm.h"
#include "utils.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

int curr_inum = 0;

void testForAllocAllBlocks() {
	unsigned char *disk = (unsigned char*) malloc(DISKSIZE);
	init(disk);
	uint32_t inum;
	for (int j = 0; j <= MAXINODE; j++) {
		int ret = allocINode(disk, inum);
		if (ret != 0) {
			printf("error: %d\n", ret);
			break;
		}
		INode inode;
		inode.address_cnt = 4;
		uint32_t addr[5];
		ret = allocBlocks(disk, 4, addr);
		if (ret != 0) {
			SuperBlock sb;
			readSuperBlock(disk, sb);
			printf("%d:\n%d\n%d\n\n", inum, sb.inode_occupied, sb.block_occupied);
			printf("error: %d\n", ret);
			break;
		}
		for (int i = 0; i < 4; i++) {
			inode.direct_addr[i] = addr[i];
		}
		writeINode(disk, inum, inode);
	}
	printf("\n");
	free(disk);
}

void testForDirParse(std::string s) {
	std::vector<std::string> pathv;
	int ret = parseDir(s, pathv);
	if (ret != 0) return;
	std::cout << pathv.size() << std::endl;
	for (auto it : pathv) {
		std::cout << it << std::endl;
	}
}

void testForCreate() {
	unsigned char *disk = (unsigned char*) malloc(DISKSIZE);
	init(disk);
	std::vector<std::string> pathv;
	std::string name;
	name = "/dir1";
	parseDir(name, pathv);
	name = pathv[pathv.size()-1];
	pathv.resize(pathv.size()-1);
	int ret = createDir(disk, pathv, name);
	if (ret == 0) {
		printf("createDir success\n");
	}

	name = "/dir1/file1";
	parseDir(name, pathv);
	name = pathv[pathv.size()-1];
	pathv.resize(pathv.size() - 1);
	ret = createFile(disk, pathv, name, 3);
	if (ret == 0) {
		printf("createFile success\n");
	}

	uint32_t inum;
	ret = getINodeForFile(disk, pathv, inum);
	if (ret != 0) {
		printf("getINodeForFile of dir error!\n");
		return;
	}
	Dir dir;
	ret = readDir(disk, inum, dir);
	if (ret == 0) {
		for (auto it : dir.data) {
			std::cout << it.first << " " << it.second << std::endl;
		}
	}

	pathv.push_back(std::string("file1"));
	ret = getINodeForFile(disk, pathv, inum);
	if (ret != 0) {
		printf("getINodeForFile of file error!\n");
		return;
	}
	File file;
	ret = readFile(disk, inum, file);
	if (ret == 0) {
		printf("%s", file.data.data());
	}

	free(disk);
}

int main() {
	File file;
	Dir dir;
	memcpy(&file, &dir, sizeof(FileState));
	testForCreate();
}