#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <mpi.h>
#include <sstream>

const unsigned short file_size = 10000;

void generateFile(const std::vector<std::string>& init_dict, const std::string& file_name);

void LZW_encoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);
void LZW_decoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);

int getIndex(const std::vector<std::string>& dictionary, const std::string& str);
int getNumFromBinStr(std::string binary_string);

int nearestPower2(int x);
std::vector<bool> binaryRepresentation(int num, int dict_size);

std::vector<std::string> split_file_into_parts(const std::string& filename, size_t num_parts);
std::vector<std::string> split_file_decoder(const std::string& filename);

std::string LZW_encoder(const std::vector<std::string>& init_dict, const std::string& text);
std::string LZW_decoder(const std::vector<std::string>& init_dict, const std::string& text);
void LZW_parallel(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);
void LZW_parallel_decode(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);