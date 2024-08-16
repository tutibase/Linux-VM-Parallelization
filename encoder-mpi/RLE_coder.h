#pragma once
#include "LZW_coder.h"
#include <map>

const short bitsSize = 16;

void RLE_encoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);
void RLE_decoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);


std::string binaryRepresentationStr(int num, int dict_size);

bool compareFiles(const std::string& file1_name, const std::string& file2_name);
int numberOfCharacters(const std::string& file_name);

std::string RLE_encoder(const std::vector<std::string>& init_dict, const std::string& text);
std::string RLE_decoder(const std::vector<std::string>& init_dict, const std::string& text);

void RLE_parallel(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);
void RLE_parallel_decode(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name);