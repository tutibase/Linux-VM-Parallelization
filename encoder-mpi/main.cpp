/* 
а) Случайно сгенерировать файл в 10 тысяч символов, используя указанный алфавит
(заглавные и строчные буквы).Закодировать текстовую информацию, используя указанный в
задании алгоритм.Определить цену кодирования.Декодировать информацию, определить
коэффициент сжатия.Программно проверить, что декодирование произошло верно.

б) Закодировать информацию алгоритмом RLE.Декодировать информацию, определить
коэффициент сжатия.Программно проверить, что декодирование произошло верно.

в) Аналогично пункту а) закодировать информацию, применив двухступенчатое кодирование
(RLE + алгоритм варианта и алгоритм варианта + RLE).Показать, какой из способов более
эффективный. 

*/
#include <iostream>
#include "LZW_coder.h"
#include "RLE_coder.h"


int main(int argc, char** argv)
{
    //omp_set_num_threads(3);
    MPI_Init(&argc, &argv);
    setlocale(LC_ALL, "Russian");
    srand(time(0));

    std::vector<std::string> bin_dict = { "0", "1" };
    std::vector<std::string> num_dict = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\n", " "};

    std::vector<std::string> dict = { "p", "o", "l", "y", " " };

    std::string file_name = "file.txt";
    generateFile(dict, file_name);

    LZW_parallel(dict, file_name, "LZW_coded.txt");
    LZW_parallel_decode(dict, "LZW_coded.txt", "LZW_decoded.txt");

    RLE_parallel(dict, file_name, "RLE_coded.txt");
    RLE_parallel_decode(dict, "RLE_coded.txt", "RLE_decoded.txt");
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        // сравнение исходного файла и декодированных
        std::cout << "LZW decoding was " << (compareFiles(file_name, "LZW_decoded.txt") ? "correct" : "incorrect");
        std::cout << "\nRLE decoding was " << (compareFiles(file_name, "RLE_decoded.txt") ? "correct" : "incorrect");

        // коэффициенты сжатия
        std::cout << "\n\nLZW compression ratio: " << float(file_size) / (numberOfCharacters("LZW_coded.txt") / 8) << std::endl;
        std::cout << "RLE compression ratio: " << float(file_size) / (numberOfCharacters("RLE_coded.txt") / 8) << std::endl;
    }
    
    // двухступенчатое кодирование и сравнение с исходным файлом
    LZW_parallel(num_dict, "RLE_coded.txt", "RLE_LZW_coded.txt");
    LZW_parallel_decode(num_dict, "RLE_LZW_coded.txt", "RLE_coded_LZW_decoded.txt");
    RLE_parallel_decode(dict, "RLE_coded_LZW_decoded.txt", "RLE_LZW_decoded.txt");
    
    RLE_parallel(num_dict, "LZW_coded.txt", "LZW_RLE_coded.txt");
    RLE_parallel_decode(num_dict, "LZW_RLE_coded.txt", "LZW_coded_RLE_decoded.txt");
    LZW_parallel_decode(dict, "LZW_coded_RLE_decoded.txt", "LZW_RLE_decoded.txt");
    
    if (rank == 0) {
        // сравнение исходного файла и декодированных
        std::cout << "\nRLE_LZW decoding was " << (compareFiles(file_name, "RLE_LZW_decoded.txt") ? "correct" : "incorrect");
        std::cout << "\nLZW_RLE decoding was " << (compareFiles(file_name, "LZW_RLE_decoded.txt") ? "correct" : "incorrect");

        // коэффициенты сжатия
        std::cout << "\n\nRLE_LZW compression ratio: " << float(file_size) / (numberOfCharacters("RLE_LZW_coded.txt") / 8) << std::endl;
        std::cout << "LZW_RLE compression ratio: " << float(file_size) / (numberOfCharacters("LZW_RLE_coded.txt") / 8) << std::endl;
    }

    MPI_Finalize();
    std::cout << "\n\nFinish ";
}

