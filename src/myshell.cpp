#include "diskaccess.h"
#include "utils.h"
#include "error.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

int curr_inum = 0;
std::string curr_path = "/";
unsigned char * disk;
char * diskFile;
CMD cmdv;

using namespace std;

int beforeCreateDir(CMD cmdv);
int beforeCreateFile(CMD cmdv);
int beforeDeleteDir(CMD cmdv);
int beforeDeleteFile(CMD cmdv);
int beforeChangeDir(CMD cmdv);
int beforeDir(CMD cmdv);
int beforeSum(CMD cmdv);
int beforeCat(CMD cmdv);
int beforeCp(CMD cmdv);
int exit();
int parseCMD(string cmd, CMD& cmdv);


int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "Usage: \n";
		cout << argv[0] << " [DiskFile]\n";
		return 0;
	}
	diskFile = argv[1];
	disk = (unsigned char*) malloc(DISKSIZE);

	FILE *pfile = fopen(argv[1], "rb");
	if (pfile != NULL) {
		fseek(pfile, 0, SEEK_END);
		if (ftell(pfile) != DISKSIZE) {
			cout << "Could not open this file for reload: File system format error\n";
			free(disk);
			return 0;
		}
		rewind(pfile);
		fread(disk, 1, DISKSIZE, pfile);
		fclose(pfile);
	} else {
		init(disk);
	}

	int fd = open("error.log", O_WRONLY | O_APPEND | O_CREAT, 0x777);
	dup2(fd, 2);

	printf("\e[4;7mWelcome!\e[0m\n");
	printf("Group Information:\n");
	printf("艾琦   201630598503\n");
	printf("邵寒   201636610681\n");
	printf("袁宜晨 201630687238\n\n");

	char cmd[1024];
	while (true) {
		printf("\e[34;1m%s\e[0m$ ", curr_path.c_str());
		cin.getline(cmd, 1024);
		int ret = parseCMD(cmd, cmdv);
		if (ret == -2) {
			free(disk);
			return 0;
		}
	}
}

int parseCMD(string cmd, CMD& cmdv) {
	cmdv.clear();
	if (cmd.size() == 0) return -1;
	int start = 0, found = cmd.find_first_of(' ');
	while (found != string::npos) {
		if (found == start) {
			start = found + 1;
			found = cmd.find_first_of(' ', found+1);
			continue;
		}
		cmdv.push_back(cmd.substr(start, found - start));
		start = found + 1;
		found = cmd.find_first_of(' ', found+1);
	}
	if (start != cmd.size()) {
		cmdv.push_back(cmd.substr(start, cmd.size() - start));
	}
	if (cmdv.size() == 0) return -1;

	if (cmdv[0] == "createDir" || cmdv[0] == "mkdir") {
		return beforeCreateDir(cmdv);
	} else 
	if (cmdv[0] == "createFile" || cmdv[0] == "mkfile") {
		return beforeCreateFile(cmdv);
	} else
	if (cmdv[0] == "exit") {
		return exit();
	} else
	if (cmdv[0] == "changeDir" || cmdv[0] == "cd") {
		return beforeChangeDir(cmdv);
	} else 
	if (cmdv[0] == "dir" || cmdv[0] == "ls") {
		return beforeDir(cmdv);
	} else
	if (cmdv[0] == "sum") {
		return beforeSum(cmdv);
	} else
	if (cmdv[0] == "cat") {
		return beforeCat(cmdv);
	} else 
	if (cmdv[0] == "deleteFile" || cmdv[0] == "rm") {
		return beforeDeleteFile(cmdv);
	} else
	if (cmdv[0] == "deleteDir" || cmdv[0] == "rmdir") {
		return beforeDeleteDir(cmdv);
	} else
	if (cmdv[0] == "cp") {
	 	return beforeCp(cmdv);
	} else {
		cout << cmdv[0] << ": command not found\n";
		return -1;
	}
}

int exit() {
	FILE *pfile = fopen(diskFile, "wb");
	fwrite(disk, 1, DISKSIZE, pfile);
	fclose(pfile);
	return -2;
}

int beforeCreateDir(CMD cmdv) {
	if (cmdv.size() != 2) {
		cout << "Usage: \n";
		cout << "createDir [Path]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "createDir [Path]\n";
		return -1;
	}
	
	string dname = pathv[pathv.size() - 1];
	if (dname.size() > 59) {
		cout << "Directory's name should no more than 59 bytes.\n";
		return -1;
	}
	pathv.resize(pathv.size() - 1);

	return createDir(disk, pathv, dname);
}

int beforeCreateFile(CMD cmdv) {
	if (cmdv.size() != 3) {
		cout << "Usage: \n";
		cout << "createFile [Path] [FileSize(in KByte)]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "createFile [Path] [FileSize(in KByte)]\n";
		return -1;
	}

	string fname = pathv[pathv.size() - 1];
	if (fname.size() > 59) {
		cout << "File's name should no more than 59 bytes.\n";
		return -1;
	}
	pathv.resize(pathv.size() - 1);
	uint32_t size;
	try {
		size = stoi(cmdv[2]);
	} catch (const invalid_argument& ia) {
		cout << "Usage: \n";
		cout << "createFile [Path] [FileSize(in KByte)]\n";
		return -1;
	}
	return createFile(disk, pathv, fname, size);
}

int beforeChangeDir(CMD cmdv) {
	if (cmdv.size() != 2) {
		cout << "Usage: \n";
		cout << "changeDir [Path]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "changeDir [Path]\n";
		return -1;
	}

	return changeDir(disk, pathv);
}

int beforeDir(CMD cmdv) {
	if (cmdv.size() > 2) {
		cout << "Usage: \n";
		cout << "dir [Path of Dir]\n";
		return -1;
	}
	CMD pathv;
	if (cmdv.size() == 1) {
		return dir(disk, pathv);
	} else {
		int ret = parseDir(cmdv[1], pathv);
		if (ret != 0) {
			cout << "Usage: \n";
			cout << "changeDir [Path]\n";
			return -1;
		}
		return dir(disk, pathv);
	}
}

int beforeSum(CMD cmdv) {
	if (cmdv.size() != 1) {
		cout << "Usage: \n";
		cout << "sum\n";
		return -1;
	}
	return sum(disk);
}

int beforeCat(CMD cmdv) {
	if (cmdv.size() != 2) {
		cout << "Usage: \n";
		cout << "cat [Path of File]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "cat [Path of File]\n";
		return -1;
	}

	return cat(disk, pathv);
}

int beforeDeleteFile(CMD cmdv) {
	if (cmdv.size() != 2) {
		cout << "Usage: \n";
		cout << "deleteFile [Path of File]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "deleteFile [Path of File]\n";
		return -1;
	}

	string fname = pathv[pathv.size() - 1];
	pathv.resize(pathv.size() - 1);

	return deleteFile(disk, pathv, fname);
}

int beforeDeleteDir(CMD cmdv) {
	if (cmdv.size() != 2) {
		cout << "Usage: \n";
		cout << "deleteDir [Path of Dir]\n";
		return -1;
	}
	CMD pathv;
	int ret = parseDir(cmdv[1], pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "deleteDir [Path of Dir]\n";
		return -1;
	}

	string dname = pathv[pathv.size() - 1];
	pathv.resize(pathv.size() - 1);

	return deleteDir(disk, pathv, dname);
}

int beforeCp(CMD cmdv) {
	if (cmdv.size() != 3) {
		cout << "Usage: \n";
		cout << "cp [Src File] [Dest File]\n";
		return -1;
	}
	CMD from_pathv;
	int ret = parseDir(cmdv[1], from_pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "cp [Src File] [Dest File]\n";
		return -1;
	}

	CMD to_pathv;
	ret = parseDir(cmdv[2], to_pathv);
	if (ret != 0) {
		cout << "Usage: \n";
		cout << "cp [Src File] [Dest File]\n";
		return -1;
	}
	string fname = to_pathv[to_pathv.size() - 1];
	if (fname.size() > 59) {
		cout << "File's name should no more than 59 bytes.\n";
		return -1;
	}
	to_pathv.resize(to_pathv.size() - 1);

	return cp(disk, from_pathv, to_pathv, fname);
}
