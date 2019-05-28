#pragma once

int parseDir(std::string &path, std::vector<std::string>& dir);

int getINodeForFile(const unsigned char* disk, std::vector<std::string>& pathv, uint32_t & ret_inum);

int createFile(unsigned char* disk, std::vector<std::string>& pathv, std::string& fname, uint32_t size);

int createDir(unsigned char* disk, std::vector<std::string>& pathv, std::string& dname);

int changeDir(unsigned char* disk, std::vector<std::string>& pathv);

int dir(const unsigned char* disk, std::vector<std::string>& pathv);

int sum(const unsigned char* disk);

int cat(const unsigned char* disk, std::vector<std::string>& pathv);

int deleteFile(unsigned char* disk, std::vector<std::string>& pathv, std::string& fname);

int deleteDir(unsigned char* disk, std::vector<std::string>& pathv, std::string& dname);

int cp(unsigned char* disk, std::vector<std::string>& from_pathv, std::vector<std::string>& to_pathv, std::string& to_fname);

int parseError(int error_code, const std::vector<std::string>& cmdv, int index = 0);