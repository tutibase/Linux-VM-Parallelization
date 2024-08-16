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
#include <omp.h>


int main()
{
    //omp_set_num_threads(99);

    setlocale(LC_ALL, "Russian");
    srand(time(0));

    std::cout << "Threads num: " << omp_get_max_threads();
    if (omp_get_max_threads() > 1000) {
        std::cout << "\nThreads num > 1000, err!";
        return 0;
    }

    std::vector<std::string> bin_dict = { "0", "1" };
    std::vector<std::string> num_dict = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "\n", " "};

    std::vector<std::string> dict = { "p", "o", "l", "y", " " };

    std::string file_name = "file.txt";
    generateFile(dict, file_name);
    
    LZW_parallel(dict, file_name, "LZW_coded.txt");
    LZW_parallel_decode(dict, "LZW_coded.txt", "LZW_decoded.txt");

    RLE_parallel(dict, file_name, "RLE_coded.txt");
    RLE_parallel_decode(dict, "RLE_coded.txt", "RLE_decoded.txt");


    #pragma omp parallel
    {
        #pragma omp sections
        {
            // сравнение исходного файла и декодированных
            #pragma omp section 
            {
                std::string tmp(std::string("\nLZW decoding was ") + ((compareFiles(file_name, "LZW_decoded.txt")) ? "correct" : "incorrect") 
                    + std::string(", thread ") + std::to_string(omp_get_thread_num()));
                std::cout << tmp;
            }
            #pragma omp section
            {
                std::string tmp(std::string("\nRLE decoding was ") + ((compareFiles(file_name, "RLE_decoded.txt")) ? "correct" : "incorrect")
                    + std::string(", thread ") + std::to_string(omp_get_thread_num()));
                std::cout << tmp; 
            }
            // коэффициенты сжатия
            #pragma omp section
            {
                std::string tmp(std::string("\n\nLZW compression ratio: ") 
                    + std::to_string(float(file_size) / (numberOfCharacters("LZW_coded.txt") / 8)) 
                    + std::string(", thread ") + std::to_string(omp_get_thread_num()) 
                    + std::string("\nRLE compression ratio: ") 
                    + std::to_string(float(file_size) / (numberOfCharacters("RLE_coded.txt") / 8))
                    + std::string(", thread ") + std::to_string(omp_get_thread_num()));
                std::cout << tmp;
            }
        }
    }

    // двухступенчатое кодирование и сравнение с исходным файлом
    LZW_parallel(num_dict, "RLE_coded.txt", "RLE_LZW_coded.txt");
    LZW_parallel_decode(num_dict, "RLE_LZW_coded.txt", "RLE_coded_LZW_decoded.txt");
    RLE_parallel_decode(dict, "RLE_coded_LZW_decoded.txt", "RLE_LZW_decoded.txt");
    std::cout << "\n\nRLE_LZW decoding was " << (compareFiles(file_name, "RLE_LZW_decoded.txt") ? "correct" : "incorrect");
    
    RLE_parallel(num_dict, "LZW_coded.txt", "LZW_RLE_coded.txt");
    RLE_parallel_decode(num_dict, "LZW_RLE_coded.txt", "LZW_coded_RLE_decoded.txt");
    LZW_parallel_decode(dict, "LZW_coded_RLE_decoded.txt", "LZW_RLE_decoded.txt");
    std::cout << "\nLZW_RLE decoding was " << (compareFiles(file_name, "LZW_RLE_decoded.txt") ? "correct" : "incorrect");

    std::cout << "\n\nRLE_LZW compression ratio: " << float(file_size) / (numberOfCharacters("RLE_LZW_coded.txt") / 8) << std::endl;
    std::cout << "LZW_RLE compression ratio: " << float(file_size) / (numberOfCharacters("LZW_RLE_coded.txt") / 8) << std::endl;

    std::cout  << "\n\nFinish";
}

