#include "RLE_coder.h"
#include <cmath>

void RLE_encoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	std::ifstream fin;
	std::ofstream fout;

	fin.open(fin_name);
	fout.open(fout_name);

	std::string different = "";
	char prev;
	char next;
	int count = 1;
	fin >> std::noskipws >> prev;
	// посимвольное считывание файла
	while (fin >> std::noskipws >> next) {
		if (next == prev) {
			count++;

			if (different != "") {
				fout << "0" << binaryRepresentationStr(different.size(), pow(2, bitsSize - 1));
				for (int i = 0; i < different.size(); i++) {
					fout << binaryRepresentationStr(getIndex(init_dict, std::string(1, different[i])), init_dict.size());
				}
				different = "";
			}
		}
		else {

			if (count == 1) {
				different += prev;
			}
			else {
				fout << "1" << binaryRepresentationStr(count, pow(2, bitsSize - 1))
					<< binaryRepresentationStr(getIndex(init_dict, std::string(1, prev)), init_dict.size());
			}

			count = 1;
			prev = next;
		}
	}

	if (different == "") {
		fout << "1" << binaryRepresentationStr(count, pow(2, bitsSize - 1))
			<< binaryRepresentationStr(getIndex(init_dict, std::string(1, prev)), init_dict.size());
	}
	else {
		different += prev;
		fout << "0" << binaryRepresentationStr(different.size(), pow(2, bitsSize - 1));
		for (int i = 0; i < different.size(); i++) {
			fout << binaryRepresentationStr(getIndex(init_dict, std::string(1, different[i])), init_dict.size());
		}
	}

	fin.close();
	fout.close();
}

std::string RLE_encoder(const std::vector<std::string>& init_dict, const std::string& text) {
	std::string result;

	std::string different = "";
	char prev;
	char next;
	int count = 1;
	prev = text[0];
	// посимвольное считывание файла
	for (int i = 1; i < text.size(); i++) {
		next = text[i];
		if (next == prev) {
			count++;

			if (different != "") {
				result.append("0" + binaryRepresentationStr(different.size(), pow(2, bitsSize - 1)));
				for (int i = 0; i < different.size(); i++) {
					result.append(binaryRepresentationStr(getIndex(init_dict, std::string(1, different[i])), init_dict.size()));
				}
				different = "";
			}
		}
		else {

			if (count == 1) {
				different += prev;
			}
			else {
				result.append("1" + binaryRepresentationStr(count, pow(2, bitsSize - 1)) +
					 binaryRepresentationStr(getIndex(init_dict, std::string(1, prev)), init_dict.size()));
			}

			count = 1;
			prev = next;
		}
	}

	if (different == "") {
		result.append("1" + binaryRepresentationStr(count, pow(2, bitsSize - 1))
			+ binaryRepresentationStr(getIndex(init_dict, std::string(1, prev)), init_dict.size()));
	}
	else {
		different += prev;
		result.append("0" + binaryRepresentationStr(different.size(), pow(2, bitsSize - 1)));
		for (int i = 0; i < different.size(); i++) {
			result.append(binaryRepresentationStr(getIndex(init_dict, std::string(1, different[i])), init_dict.size()));
		}
	}

	return result;
}


void RLE_decoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	std::ifstream fin;
	std::ofstream fout;


	fin.open(fin_name);
	fout.open(fout_name);

	char buff[50];
	while (!fin.eof()) {
		fin.get(buff, bitsSize + 1);
		std::string binary_string(buff);
		if (binary_string == "") break;

		if (binary_string[0] == '1') {
			int num = 0;
			num = getNumFromBinStr(binary_string.substr(1, binary_string.size() - 1));

			fin.get(buff, nearestPower2(init_dict.size()) + 1);
			binary_string = buff;
			for (int i = 0; i < num; i++) {
				fout << init_dict[getNumFromBinStr(binary_string)];
			}
		}
		else {
			int num = 0;
			num = getNumFromBinStr(binary_string);

			for (int i = 0; i < num; i++) {
				fin.get(buff, nearestPower2(init_dict.size()) + 1);
				binary_string = buff;
				fout << init_dict[getNumFromBinStr(binary_string)];
			}
		}

	}

	fin.close();
	fout.close();
}


std::string RLE_decoder(const std::vector<std::string>& init_dict, const std::string& text) {
	std::istringstream fin(text);
	std::ostringstream fout;

	char buff[50];
	while (fin) {
		fin.get(buff, bitsSize + 1);
		std::string binary_string(buff);
		if (binary_string == "") break;

		if (binary_string[0] == '1') {
			int num = 0;
			num = getNumFromBinStr(binary_string.substr(1, binary_string.size() - 1));

			fin.get(buff, nearestPower2(init_dict.size()) + 1);
			binary_string = buff;
			for (int i = 0; i < num; i++) {
				fout << init_dict[getNumFromBinStr(binary_string)];
			}
		}
		else {
			int num = 0;
			num = getNumFromBinStr(binary_string);

			for (int i = 0; i < num; i++) {
				fin.get(buff, nearestPower2(init_dict.size()) + 1);
				binary_string = buff;
				fout << init_dict[getNumFromBinStr(binary_string)];
			}
		}

	}

	return fout.str();
}


void RLE_parallel(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	std::vector<std::string> parts;
	std::string part;
	std::string coded_part;

	if (rank == 0) {
		// Разделить файл только в процессе с рангом 0
		parts = split_file_into_parts(fin_name, size);

		// Рассылка каждой части файла соответствующему процессу
		// MPI_Scatter ожидает, что все элементы будут одинакового размера, поэтому используем Send/Recv
		for (int i = 1; i < size; i++) {
			MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
		}
		part = parts[0];  // Часть для процесса 0
	}
	else {
		MPI_Status status;
		MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
		int count;
		MPI_Get_count(&status, MPI_CHAR, &count);
		part.resize(count);
		MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Непосредственно кодировка
	coded_part = RLE_encoder(init_dict, part);

	// Сбор закодированных частей обратно в процесс 0
	if (rank == 0) {
		std::vector<std::string> coded_parts(size);
		coded_parts[0] = std::move(coded_part);
		for (int i = 1; i < size; i++) {
			MPI_Status status;
			MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
			int count;
			MPI_Get_count(&status, MPI_CHAR, &count);
			coded_parts[i].resize(count);
			MPI_Recv(&coded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		// Сохранение результатов
		std::ofstream fout(fout_name);
		for (const auto& cp : coded_parts) {
			fout << cp.size() << ' ';
		}
		fout << '\n';
		for (const auto& cp : coded_parts) {
			fout << cp;
		}
		fout.close();
	}
	else {
		// Отправка закодированной части обратно процессу 0
		MPI_Send(coded_part.data(), coded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	}
}

void RLE_parallel_decode(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	std::vector<std::string> parts;
	std::string part;
	std::string decoded_part;

	if (rank == 0) {
		// Разделить файл только в процессе с рангом 0
		std::vector<std::string> parts = split_file_decoder(fin_name);

		// Рассылка каждой части файла соответствующему процессу
		for (int i = 1; i < size; i++) {
			MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
		}
		part = parts[0];  // Часть для процесса 0
	}
	else {
		MPI_Status status;
		MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
		int count;
		MPI_Get_count(&status, MPI_CHAR, &count);
		part.resize(count);
		MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	decoded_part = RLE_decoder(init_dict, part);

	// Сбор декодированных частей обратно в процесс 0
	if (rank == 0) {
		std::vector<std::string> decoded_parts(size);
		decoded_parts[0] = std::move(decoded_part);
		for (int i = 1; i < size; i++) {
			MPI_Status status;
			MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
			int count;
			MPI_Get_count(&status, MPI_CHAR, &count);
			decoded_parts[i].resize(count);
			MPI_Recv(&decoded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		// Сохранение результатов
		std::ofstream fout(fout_name, std::ios::binary);
		for (const auto& dp : decoded_parts) {
			fout << dp;
		}
		fout.close();
	}
	else {
		// Отправка закодированной части обратно процессу 0
		MPI_Send(decoded_part.data(), decoded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	}
}

///////////////////////////////////

std::string binaryRepresentationStr(int num, int dict_size) {
	std::vector<bool> code(nearestPower2(dict_size));
	int i = 0;
	while (num > 0) {
		code[code.size() - 1 - i] = num % 2;
		num /= 2;
		i++;
	}

	std::string result;
	for (int i = 0; i < code.size(); i++) {
		result += code[i] ? "1" : "0";
	}

	return result;
}

bool compareFiles(const std::string& file1_name, const std::string& file2_name) {
	std::ifstream file1(file1_name);
	std::ifstream file2(file2_name);

	const int buff_size = 500;
	char buff[buff_size];
	while (!file1.eof() and !file2.eof()) {
		file1.get(buff, buff_size);
		std::string line1(buff);

		file2.get(buff, buff_size);
		std::string line2(buff);

		if (line1 != line2) {
			return false;
		}
	}

	return !(file1.eof() != file2.eof());
}


int numberOfCharacters(const std::string& file_name) {
	std::ifstream fin;
	fin.open(file_name);

	char symbol;
	int result = 0;
	// посимвольное считывание файла
	while (fin >> std::noskipws >> symbol) {
		result++;
	}

	fin.close();
	return result;
}