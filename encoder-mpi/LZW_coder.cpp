#include "LZW_coder.h"
#include <cmath>

// генерация файла на основе алфавита
void generateFile(const std::vector<std::string>& init_dict, const std::string& file_name) {
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int alphabet_size = init_dict.size();
	int local_size = file_size / size;  // размер задачи на процесс
	std::string local_buffer;

	if (rank == 0) {
		local_size += file_size % size;
	}

	// Генерация данных в каждом процессе
	srand(time(NULL) + rank);  
	for (int i = 0; i < local_size; ++i) {
		local_buffer += init_dict[rand() % alphabet_size];
	}

	// Сбор закодированных частей в процесс 0
	if (rank == 0) {
		std::vector<std::string> generated_parts(size);
		generated_parts[0] = std::move(local_buffer);
		for (int i = 1; i < size; i++) {
			MPI_Status status;
			MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
			int count;
			MPI_Get_count(&status, MPI_CHAR, &count);
			generated_parts[i].resize(count);
			MPI_Recv(&generated_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		// Процесс 0 записывает собранные данные в файл
		std::ofstream file(file_name);
		for (const auto& gp : generated_parts) {
			file << gp;
		}
		file.close();
	}
	else {
		// Отправка сгенерированной части процессу 0
		MPI_Send(local_buffer.data(), local_buffer.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	}

}


// кодирование LZW
void LZW_encoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	std::ifstream fin;
	std::ofstream fout;

	fin.open(fin_name);
	fout.open(fout_name);

	std::vector<std::string> dictionary(init_dict);

	char symbol;
	std::string str = "";
	// посимвольное считывание файла
	while (fin >> std::noskipws >> symbol) {
		str += symbol;

		// поиск строки в словаре
		if (std::find(dictionary.begin(), dictionary.end(), str) == dictionary.end()) {
			// not found
			char eChar = str[str.size() - 1];
			str.pop_back();

			// отправка в поток
			std::vector<bool> tmp_code = binaryRepresentation(getIndex(dictionary, str), dictionary.size());
			for (int i = 0; i < tmp_code.size(); i++) {
				fout << tmp_code[i];
			}

			// добавление строки в словарь
			str.push_back(eChar);
			dictionary.push_back(str);

			// следующая строка начинается с последнего символа новой для словаря строки
			str = eChar;
		}
	}

	// отправка в поток последнего символа
	std::vector<bool> tmp_code = binaryRepresentation(getIndex(dictionary, str), dictionary.size());
	for (int i = 0; i < tmp_code.size(); i++) {
		fout << tmp_code[i];
	}

	fin.close();
	fout.close();
}

// кодирование LZW для строк
std::string LZW_encoder(const std::vector<std::string>& init_dict, const std::string& text) {
	std::vector<std::string> dictionary(init_dict);
	std::string result = "";
	char symbol;
	std::string str = "";
	// посимвольное считывание файла
	for (int i = 0; i < text.size(); i++) {
		char symbol = text[i];
		str += symbol;

		// поиск строки в словаре
		if (std::find(dictionary.begin(), dictionary.end(), str) == dictionary.end()) {
			// not found
			char eChar = str[str.size() - 1];
			str.pop_back();

			// отправка в поток
			std::vector<bool> tmp_code = binaryRepresentation(getIndex(dictionary, str), dictionary.size());
			for (int i = 0; i < tmp_code.size(); i++) {
				result.append(std::to_string(tmp_code[i] ? 1 : 0));
			}

			// добавление строки в словарь
			str.push_back(eChar);
			dictionary.push_back(str);

			// следующая строка начинается с последнего символа новой для словаря строки
			str = eChar;
		}
	}

	// отправка в поток последнего символа
	std::vector<bool> tmp_code = binaryRepresentation(getIndex(dictionary, str), dictionary.size());
	for (int i = 0; i < tmp_code.size(); i++) {
		result.append(std::to_string(tmp_code[i] ? 1 : 0));
	}

	return result;
}

void LZW_parallel(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
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
	coded_part = LZW_encoder(init_dict, part);

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

void LZW_parallel_decode(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
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
	
	decoded_part = LZW_decoder(init_dict, part);

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

// декодирование LZW
void LZW_decoder(const std::vector<std::string>& init_dict, const std::string& fin_name, const std::string& fout_name) {
	std::ifstream fin;
	std::ofstream fout;

	fin.open(fin_name);
	fout.open(fout_name);

	bool first_sym = 1;
	bool flag = 1;
	std::vector<std::string> dictionary(init_dict);
	std::string str = "";
	char buff[50];
	while (!fin.eof()) {
		flag = 1;
		// считываем x бит из файла
		if (dictionary.size() == 2 and first_sym) {
			fin.get(buff, 2);
			first_sym = 0;
		}
		else {
			fin.get(buff, nearestPower2(dictionary.size() + 1) + 1);
		}

		std::string binary_string(buff);
		if (binary_string == "") break;
		std::string tmp;

		// если строки ещё нет в словаре, то дополняем её самостоятельно (ссылка на объяснение)
		// http://vmath.ru/vf5/codes/lzw#dekodirovanie:~:text=%D0%9F-,%D0%9F%D1%80%D0%B8%D0%BC%D0%B5%D1%80,-.%20%D0%97%D0%B0%D0%BA%D0%BE%D0%B4%D0%B8%D1%80%D0%BE%D0%B2%D0%B0%D1%82%D1%8C%20%D0%B8
		if (getNumFromBinStr(binary_string) == dictionary.size())
			tmp = str + str[0];
		else
			tmp = dictionary[getNumFromBinStr(binary_string)];

		// проход по всем символам считанной строки
		for (int i = 0; i < tmp.size(); i++) {
			str += tmp[i];
			// поиск строки в словаре
			if (std::find(dictionary.begin(), dictionary.end(), str) == dictionary.end() and flag) {
				// not found

				char eChar = str[str.size() - 1];
				str.pop_back();

				// отправка в поток
				fout << str;

				// добавление строки в словарь
				str.push_back(eChar);
				dictionary.push_back(str);

				// следующая строка начинается с невошедших символов tmp
				str = tmp.substr(i, tmp.size() - i); //tmp.size() - i символов начиная с i-го 
				flag = false;
			}
			if (!flag) break;
		}
	}

	// отправка в поток последнего символа
	fout << str;

	fin.close();
	fout.close();
}

// декодирование LZW строки
std::string LZW_decoder(const std::vector<std::string>& init_dict, const std::string& text) {
	bool first_sym = true;
	std::vector<std::string> dictionary(init_dict);
	std::string str = "";
	std::string binary_string = "";
	int index = 0;
	std::string result;

	while (index < text.length()) {
		if (dictionary.size() == 2 && first_sym) {
			binary_string = text.substr(index, 1);
			first_sym = false;
			index += 1;
		}
		else {
			int bits = nearestPower2(dictionary.size() + 1);
			if (index + bits <= text.length()) {
				binary_string = text.substr(index, bits);
				index += bits;
			}
			else {
				break;
			}
		}

		if (binary_string.empty()) break;

		std::string tmp;
		int bin_index = getNumFromBinStr(binary_string);

		if (bin_index == dictionary.size()) {
			tmp = str + str[0];
		}
		else {
			tmp = dictionary[bin_index];
		}

		result += tmp;

		if (!str.empty()) {
			std::string new_entry = str + tmp[0];
			if (std::find(dictionary.begin(), dictionary.end(), new_entry) == dictionary.end()) {
				dictionary.push_back(new_entry); // Add new entry to the dictionary
			}
		}

		str = tmp; // Update str to be the last match
	}
	return result;
}

/////////////////////////////////////////////

// индекс элемента в векторе
int getIndex(const std::vector<std::string>& dictionary, const std::string& str) {
	auto it = std::find(dictionary.begin(), dictionary.end(), str);
	if (it != dictionary.end()) {
		int ind = std::distance(dictionary.begin(), it);
		return ind;
	}
	std::cout << "Элемента " << str << " нет в векторе." << std::endl;
	return -1;
}

// число в 10-чной СС, полученное из его двоичного представления 
int getNumFromBinStr(std::string binary_string) {
	std::reverse(binary_string.begin(), binary_string.end());
	int decimal_number = 0;
	int base = 2;

	for (int i = 0; i < binary_string.size(); i++) {
		if (binary_string[i] == '1') {
			decimal_number += pow(base, i);
		}
	}
	return decimal_number;
}

// Ищет и возвращает ближайшую к x сверху степень двойки
int nearestPower2(int x) {
	int result = 0;
	int i = 1;
	while (i < x) {
		result++;
		i *= 2;
	}
	return result;
}

// бинарное представление числа с разрядностью x, где 2^x >= dict_size
std::vector<bool> binaryRepresentation(int num, int dict_size) {
	std::vector<bool> code(nearestPower2(dict_size));
	int i = 0;
	while (num > 0) {
		code[code.size() - 1 - i] = num % 2;
		num /= 2;
		i++;
	}

	return code;
}


std::vector<std::string> split_file_into_parts(const std::string& filename, size_t num_parts) {
	// Открыть файл
	std::ifstream file(filename);

	// Считать содержимое файла в строку
	std::string content((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	// Закрыть файл
	file.close();

	// Вычислить размер одной части
	size_t part_size = content.size() / num_parts;
	std::vector<std::string> parts;
	parts.reserve(num_parts);

	// Разделить содержимое на части
	for (size_t i = 0; i < num_parts; ++i) {
		size_t start = i * part_size;
		size_t end = (i == num_parts - 1) ? content.size() : (i + 1) * part_size;
		parts.push_back(content.substr(start, end - start));
	}

	return parts;
}


std::vector<std::string> split_file_decoder(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;
	std::vector<int> numbers;

	// Считывание первой строки и преобразование в вектор целых чисел
	if (getline(file, line)) {
		std::istringstream iss(line);
		int number;
		while (iss >> number) {
			numbers.push_back(number);
		}
	}

	// Считывание второй строки как строку
	getline(file, line);
	file.close();

	std::vector<std::string> result(numbers.size());
	int current_pos = 0;  // Начальная позиция для извлечения подстроки
	for (int i = 0; i < result.size(); i++) {
		result[i] = line.substr(current_pos, numbers[i]);
		current_pos += numbers[i];  // Перемещаем начало следующей подстроки
	}

	return result;
}