#include "structs.h"
#include "diskaccess.h"
#include "utils.h"
#include "error.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

extern uint32_t curr_inum;
extern std::string curr_path;
extern std::vector<std::string> cmdv;

int parseDir(std::string &path, std::vector<std::string>& dir) {
	dir.clear();
	if (path.empty()) return PATHFORMERROR;
	int start = 0;
	int found = path.find_first_of('/');
	if (found == 0) {
		dir.push_back(std::string("/"));
		start = found + 1;
		found = path.find_first_of('/', found+1);
	}
	while (found != std::string::npos) {
		if (start != found) {
			dir.push_back(path.substr(start, found - start));
		}
		start = found + 1;
		found = path.find_first_of('/', found+1);
	}
	if (start != path.size()) {
		dir.push_back(path.substr(start, path.size() - start));
	}
	return 0;
}
 
int getINodeForFile(const unsigned char* disk, std::vector<std::string>& pathv, uint32_t & ret_inum) {
	uint32_t inum = curr_inum;
	int ptr = 0;
	if (pathv.size() == 0) {
		ret_inum = inum;
		return 0;
	}
	if (pathv[0] == "/") {
		inum = 0;
		ptr++;
	}
	Dir dir;
	int ret;
	for (; ptr < pathv.size(); ptr++) {
		if (pathv[ptr] == ".") continue;
		ret = readDir(disk, inum, dir);
		if (ret != 0) {
			std::cerr << "getINodeForFile error: " << ret << std::endl;
			return ret;
		}
		uint32_t tmp_inum = 0xffffffff;
		for (auto it : dir.data) {
			if (it.first != pathv[ptr]) continue;
			tmp_inum = it.second;
			break;
		}
		if (tmp_inum == 0xffffffff) {
			std::cerr << "getINodeForFile error: " << NOSUCHFILEORDIR << std::endl;
			return NOSUCHFILEORDIR;
		}
		inum = tmp_inum;
	}
	ret_inum = inum;
	return 0;
}

int createFile(unsigned char* disk, std::vector<std::string>& pathv, std::string &fname, uint32_t size) { //size in KB
	int ret;
	uint32_t dir_inum;
	ret = getINodeForFile(disk, pathv, dir_inum);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	Dir dir;
	readDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	if (dir.data.size() == MAXFILEPERDIR) {
		std::cerr << "createFile error: " << FILENUMEXCE << std::endl;
		parseError(FILENUMEXCE, cmdv);
		return FILENUMEXCE;
	}
	for (auto it : dir.data) {
		if (it.first == fname) {
			std::cerr << "createFile error: " << FILEALREADYEXISTS << std::endl;
			parseError(FILEALREADYEXISTS, cmdv, 1);
			return FILEALREADYEXISTS;
		}
	}

	uint32_t blocks;
	uint32_t size_in_byte = size * 1024;
	ret = calcBlocks(size_in_byte + sizeof(FileState), blocks);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		return ret;
	}
	if (blocks > MAXADDRESSPERINODE) {
		std::cerr << "createFile error: " << FILESIZEEXCE << std::endl;
		parseError(FILESIZEEXCE, cmdv);
		return ret;
	}

	//alloc inode
	uint32_t inum;
	ret = allocINode(disk, inum);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}

	//create file
	File file;
	file.file_mode = ISFILE;
	file.create_time = time(NULL);
	file.modify_time = file.create_time;
	file.block_occupied = blocks;
	file.data_info = size_in_byte;
	file.data.resize(size_in_byte);
	std::stringstream ss;
	time_t * tm = (time_t *) malloc(sizeof(time_t));
	time(tm);
	ss << "This is a file created by group (艾琦, 邵寒, 袁宜晨) at " << ctime(tm);
	strcpy((char*)(file.data.data()), ss.str().c_str());
	free(tm);

	ret = writeFile(disk, inum, file);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}

	dir.modify_time = time(NULL);
	dir.data.push_back(std::make_pair(fname, inum));
	ret = writeDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "createFile error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}

	return 0;
}

int createDir(unsigned char* disk, std::vector<std::string>& pathv, std::string& dname) {
	int ret;
	uint32_t dir_inum;
	ret = getINodeForFile(disk, pathv, dir_inum);
	if (ret != 0) {
		std::cerr << "createDir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}

	Dir pdir;
	ret = readDir(disk, dir_inum, pdir);
	if (ret != 0) {
		std::cerr << "createDir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	if (pdir.data.size() == MAXFILEPERDIR) {
		std::cerr << "createDir error: " << FILENUMEXCE << std::endl;
		parseError(FILENUMEXCE, cmdv);
		return FILENUMEXCE;
	}
	for (auto it : pdir.data) {
		if (it.first == dname) {
			std::cerr << "createDir error: " << FILEALREADYEXISTS << std::endl;
			parseError(FILEALREADYEXISTS, cmdv, 1);
			return FILEALREADYEXISTS;
		}
	}

	uint32_t inum;
	ret = allocINode(disk, inum);
	if (ret != 0) {
		std::cerr << "createDir error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}
	pdir.modify_time = time(NULL);
	pdir.data.push_back(std::make_pair(dname, inum));

	Dir dir;
	dir.file_mode = ISDIR;
	dir.create_time = pdir.modify_time;
	dir.modify_time = pdir.modify_time;
	dir.block_occupied = 1;
	dir.data.push_back(std::make_pair(std::string("."), inum));
	dir.data.push_back(std::make_pair(std::string(".."), dir_inum));
	ret = writeDir(disk, inum, dir);
	if (ret != 0) {
		std::cerr << "createDir error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}

	ret = writeDir(disk, dir_inum, pdir);
	if (ret != 0) {
		std::cerr << "createDir error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}

	return 0;
}

int changeDir(unsigned char* disk, std::vector<std::string>& pathv) {
	int ret;
	uint32_t dir_inum;
	ret = getINodeForFile(disk, pathv, dir_inum);
	if (ret != 0) {
		std::cerr << "changeDir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	FileState fs;
	readFileState(disk, dir_inum, fs);
	if (fs.file_mode != ISDIR) {
		std::cerr << "changeDir error: " << NOTADIR << std::endl;
		parseError(NOTADIR, cmdv, 1);
		return NOTADIR;
	}

	curr_inum = dir_inum;
	std::vector<std::string> tmp;
	if (pathv[0] == "/") {
		tmp.push_back(std::string("/"));
	} else {
		parseDir(curr_path, tmp);
	}
	for (auto it : pathv) {
		if (it == "/") continue;
		if (it == ".") continue;
		if (it == "..") {
			if (tmp.size() == 1) continue;
			tmp.resize(tmp.size() - 1);
			continue;
		}
		tmp.push_back(it);
	}
	std::stringstream ss;
	for (int i = 1; i < tmp.size(); i++) {
		ss << "/" << tmp[i];
	}
	curr_path = ss.str();
	if (curr_path.size() == 0) curr_path = "/";
	return 0;
}

int dir(const unsigned char* disk, std::vector<std::string>& pathv) {
	int ret;
	uint32_t inum;
	ret = getINodeForFile(disk, pathv, inum);
	if (ret != 0) {
		std::cerr << "dir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	Dir dir;
	ret = readDir(disk, inum, dir);
	if (ret != 0) {
		std::cerr << "dir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	FileState fs;

	char date[30];
	time_t t = dir.modify_time;
	struct tm * timeinfo;
	timeinfo = localtime(&t);
	strftime(date, 30, "%Y/%m/%d %R    ", timeinfo);
	std::cout << date << "<DIR>          ." << std::endl;

	for (int i = 1; i < dir.data.size(); i++) {
		readFileState(disk, dir.data[i].second, fs);
		t = fs.modify_time;
		timeinfo = localtime(&t);
		strftime(date, 30, "%Y/%m/%d %R    ", timeinfo);
		if (fs.file_mode == ISDIR) {
			std::cout << date << "<DIR>          " << dir.data[i].first << std::endl;
		} else {
			std::cout << date << std::setw(14) << fs.block_occupied * BLOCKSIZE << " " << dir.data[i].first << std::endl;
		}
	}
	return 0;
}

int sum(const unsigned char* disk) {
	SuperBlock sb;
	readSuperBlock(disk, sb);
	int used_space = sb.block_occupied * BLOCKSIZE;
	int free_space = DISKSIZE - used_space;
	int per_space = (float) used_space / (used_space + free_space) * 100;
	int used_block = sb.block_occupied;
	int free_block = MAXBLOCK - used_block;
	int per_block = (float) used_block / (used_block + free_block) * 100;
	int used_inode = sb.inode_occupied;
	int free_inode = MAXINODE - used_inode;
	int per_inode = (float) used_inode / (used_inode + free_inode) * 100;
	std::cout << "                      IN USE     FREE    TOTAL    PERCENT" << std::endl;
	std::cout << "CAPACITY (In Byte)" << std::setw(10) << used_space << std::setw(9) << free_space << std::setw(9) << used_space + free_space << std::setw(10) << per_space << "%\n";
	std::cout << "BLOCKS  #         " << std::setw(10) << used_block << std::setw(9) << free_block << std::setw(9) << used_block + free_block << std::setw(10) << per_block << "%\n";
	std::cout << "INODES  #         " << std::setw(10) << used_inode << std::setw(9) << free_inode << std::setw(9) << used_inode + free_inode << std::setw(10) << per_inode << "%\n";
	return 0;
}

int cat(const unsigned char* disk, std::vector<std::string>& pathv) {
	int ret;
	uint32_t inum;
	ret = getINodeForFile(disk, pathv, inum);
	if (ret != 0) {
		std::cerr << "cat error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	File file;
	ret = readFile(disk, inum, file);
	if (ret != 0) {
		std::cerr << "cat error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	if (file.file_mode != ISFILE) {
		std::cerr << "cat error: " << NOTAFILE << std::endl;
		parseError(NOTAFILE, cmdv, 1);
		return NOTAFILE;
	}

	std::cout << file.data.data() << std::endl;
	return 0;
}

int deleteFile(unsigned char* disk, std::vector<std::string>& pathv, std::string& fname) {
	if (fname == "/") {
		std::cerr << "deleteFile error: " << NOTAFILE << std::endl;
		parseError(NOTAFILE, cmdv, 1);
		return NOTAFILE;
	}
	int ret;
	uint32_t dir_inum;
	ret = getINodeForFile(disk, pathv, dir_inum);
	if (ret != 0) {
		std::cerr << "deleteFile error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}

	Dir dir;
	ret = readDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "deleteFile error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}

	uint32_t inum = 0xffffffff;

	DirectoryContent::iterator it;
	for (it = dir.data.begin(); it != dir.data.end(); it++) {
		if ((*it).first == fname) {
			inum = (*it).second;
			break;
		}
	}
	if (inum == 0xffffffff) {
		std::cerr << "deleteFile error: " << NOSUCHFILEORDIR << std::endl;
		parseError(NOSUCHFILEORDIR, cmdv, 1);
		return NOSUCHFILEORDIR;
	}

	FileState fs;
	readFileState(disk, inum, fs);
	if (fs.file_mode != ISFILE) {
		std::cerr << "deleteFile error: " << NOTAFILE << std::endl;
		parseError(NOTAFILE, cmdv, 1);
		return NOTAFILE;
	}

	ret = freeINode(disk, inum);
	if (ret != 0) {
		std::cerr << "deleteFile error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}

	dir.data.erase(it);
	dir.modify_time = time(NULL);
	ret = writeDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "deleteFile error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}

	return ret;
}

int deleteDir(unsigned char* disk, std::vector<std::string>& pathv, std::string& dname) {
	if (dname == "/") {
		std::cerr << "deleteDir error: " << DELETEROOTDIR << std::endl;
		parseError(DELETEROOTDIR, cmdv);
		return DELETEROOTDIR;
	}
	int ret;
	uint32_t dir_inum;
	ret = getINodeForFile(disk, pathv, dir_inum);
	if (ret != 0) {
		std::cerr << "deleteDir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}

	Dir dir;
	ret = readDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "deleteDir error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}

	uint32_t inum = 0xffffffff;

	DirectoryContent::iterator it;
	for (it = dir.data.begin(); it != dir.data.end(); it++) {
		if ((*it).first == dname) {
			inum = (*it).second;
			break;
		}
	}
	if (inum == 0xffffffff) {
		std::cerr << "deleteDir error: " << NOSUCHFILEORDIR << std::endl;
		parseError(NOSUCHFILEORDIR, cmdv, 1);
		return NOSUCHFILEORDIR;
	}
	if (inum == curr_inum) {
		std::cerr << "deleteDir error: " << DELETECURRDIR << std::endl;
		parseError(DELETECURRDIR, cmdv);
		return DELETECURRDIR;
	}

	Dir sdir;
	ret = readDir(disk, inum, sdir);
	if (ret != 0) {
		std::cerr << "deleteDir error: " << NOTADIR << std::endl;
		parseError(NOTADIR, cmdv, 1);
		return NOTADIR;
	}
	if (sdir.data.size() != 2) {
		std::cerr << "deleteDir error: " << NOTANEMPTYDIR << std::endl;
		parseError(NOTANEMPTYDIR, cmdv, 1);
		return NOTANEMPTYDIR;
	}

	ret = freeINode(disk, inum);
	if (ret != 0) {
		std::cerr << "deleteDir error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}

	dir.data.erase(it);
	dir.modify_time = time(NULL);
	ret = writeDir(disk, dir_inum, dir);
	if (ret != 0) {
		std::cerr << "deleteDir error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}

	return ret;
}

int cp(unsigned char* disk, std::vector<std::string>& from_pathv, std::vector<std::string>& to_pathv, std::string& to_fname) {
	int ret;
	uint32_t from_inum;
	ret = getINodeForFile(disk, from_pathv, from_inum);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	File file;
	ret = readFile(disk, from_inum, file);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}
	if (file.file_mode != ISFILE) {
		std::cerr << "cp error: " << NOTAFILE << std::endl;
		parseError(NOTAFILE, cmdv, 1);
		return NOTAFILE;
	}
	file.create_time = time(NULL);
	file.modify_time = file.create_time;

	uint32_t to_dir_inum;
	ret = getINodeForFile(disk, to_pathv, to_dir_inum);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	Dir dir;
	ret = readDir(disk, to_dir_inum, dir);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		parseError(ret, cmdv, 1);
		return ret;
	}
	for (auto it : dir.data) {
		if (it.first == to_fname) {
			std::cerr << "cp error: " << FILEALREADYEXISTS << std::endl;
			parseError(FILEALREADYEXISTS, cmdv);
			return FILEALREADYEXISTS;
		}
	}

	uint32_t inum;
	ret = allocINode(disk, inum);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		parseError(ret, cmdv);
		return ret;
	}
	ret = writeFile(disk, inum, file);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}

	dir.data.push_back(std::make_pair(to_fname, inum));
	dir.modify_time = file.create_time;
	ret = writeDir(disk, to_dir_inum, dir);
	if (ret != 0) {
		std::cerr << "cp error: " << ret << std::endl;
		freeINode(disk, inum);
		parseError(ret, cmdv);
		return ret;
	}
	return ret;
}

int parseError(int error_code, const std::vector<std::string>& cmdv, int index) {
	printf("\e[1m%s\e[0m \e[31;1merror\e[0m: ", cmdv[0].c_str());
	if (index != 0) {
		printf("'\e[1m%s\e[0m' ", cmdv[index].c_str());
	}
	switch (error_code) {
		case 1:
			printf("Illegal address\n");
			break;
		case 2:
			printf("INode number exceeded\n");
			break;
		case 3:
			printf("No valid INode for allocating\n");
			break;
		case 4:
			printf("INode number exceeded\n");
			break;
		case 5:
			printf("This INode cannot be free\n");
			break;
		case 6:
			printf("File size exceedes limit (%d Bytes)\n", MAXADDRESSPERINODE * BLOCKSIZE);
			break;
		case 7:
			printf("Not a directory\n");
			break;
		case 8:
			printf("Too many files in this directory\n");
			break;
		case 9:
			break;
		case 10:
			printf("Empty path\n");
			break;
		case 11:
			printf("No such file or directory\n");
			break;
		case 12:
			printf("File already exists\n");
			break;
		case 13:
			printf("Not a directory\n");
			break;
		case 14:
			printf("Not a file\n");
			break;
		case 15:
			printf("Directory not empty\n");
			break;
		case 16:
			printf("Cannot delete this directory (root/)\n");
			break;
		case 17:
			printf("Cannot delete this directory (current_dir./)\n");
			break;
		default:
			break;
	}
}