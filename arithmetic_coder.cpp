#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>

// Number of bits in a code value
#define code_value_bits 16

// Get the index of a symbol
int indexForSymbol(char c, std::vector<std::pair<char, unsigned int>> arr) {
    for (int i = 0; i < arr.size(); i++) {
        if (c == arr[i].first) {
            return i+2;
        }
    }

    return -1;
}
// Writing a bit to a file
void output_bit(unsigned int bit, unsigned int* bit_len,
                unsigned char* write_bit, FILE* output_file) {
    (*write_bit) >>= 1;
    if (bit) (*write_bit) |= 0x80;
    (*bit_len)--;
    if ( (*bit_len) == 0 ) {
        fputc((*write_bit), output_file);
        (*bit_len) = 8;
    }
}

// Write a sequence of bits to a file
void bitsPlusFollow(unsigned int bit, unsigned int* bits_to_follow,
                    unsigned int* bit_len, unsigned char* write_bit,
                    FILE* output_file) {
    output_bit(bit, bit_len, write_bit, output_file);

    for (; *bits_to_follow > 0; (*bits_to_follow)--) {
        output_bit(!bit, bit_len, write_bit, output_file);
    }
}

// Coding function
double coder(const char* input_name = "input.txt",
                const char* output_name = "encoded.txt") {
    uint16_t * alfabet = new uint16_t[256];
    for (int i = 0; i < 256; i++) {
        alfabet[i] = 0;
    }

    FILE* input_file = fopen(input_name, "rb");
    if (input_file == nullptr) {
       throw std::invalid_argument("File not found.");
    }

    unsigned char character = 0;

    while (!feof(input_file)) {  // Read from input file
       character = fgetc(input_file);
       if (!feof(input_file)) {
           alfabet[character]++;
       }
    }

    fclose(input_file);

    std::vector<std::pair<char, unsigned int>> arr;
    for (int i=0; i < 256; i++) {
        if (alfabet[i] != 0) {
            arr.push_back(std::make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(arr.begin(), arr.end(),
            [](const std::pair<char, unsigned int> &l,
                const std::pair<char, unsigned int> &r) {
                if (l.second != r.second) {
                    return l.second >= r.second;
                }

                return l.first < r.first;
            });


    uint16_t* table = new uint16_t[arr.size() + 2];
    table[0] = 0;
    table[1] = 1;
    for (int i = 0; i < arr.size(); i++) {
        unsigned int b = arr[i].second;
        for (int j = 0; j < i; j++) {
            b += arr[j].second;
        }
        table[i+2] = b;
    }

    if (table[arr.size()] > (1 << ((code_value_bits - 2)) - 1)) {
        throw std::invalid_argument("Error, too long count.");
    }

    unsigned int low_v = 0;
    unsigned int high_v = ((static_cast<unsigned int>(1)
                                    << code_value_bits) - 1);

    unsigned int delitel = table[arr.size()+1];
    unsigned int diff = high_v - low_v + 1;
    unsigned int first_qtr = (high_v + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;

    unsigned int bits_to_follow = 0;
    unsigned int bit_len = 8;
    unsigned char write_bit = 0;

    int j = 0;

    input_file = fopen(input_name, "rb");
    FILE* output_file = fopen(output_name, "wb +");

    char col_letters = arr.size();
    fputc(col_letters, output_file);

    // Writing the letters used and their number
    for (int i = 0; i < 256; i++) {
        if (alfabet[i] != 0) {
            fputc(static_cast<char>(i), output_file);
            fwrite(reinterpret_cast<const char*>(&alfabet[i]),
                    sizeof(uint16_t), 1, output_file);
        }
    }


    while (!feof(input_file)) {  // Read from input file
        character = fgetc(input_file);

        if (!feof(input_file)) {
            j = indexForSymbol(character, arr);

            // Narrow the code region to that allotted  to this symbol
            high_v = low_v +  table[j] * diff / delitel - 1;
            low_v = low_v + table[j - 1] * diff  / delitel;

            for (;;) {  // Loop to output bits
                if (high_v < half) {
                    bitsPlusFollow(0, &bits_to_follow,
                                    &bit_len, &write_bit, output_file);
                } else if (low_v >= half) {
                    bitsPlusFollow(1, &bits_to_follow,
                                    &bit_len, &write_bit, output_file);
                    low_v -= half;
                    high_v -= half;
                } else if ((low_v >= first_qtr) && (high_v < third_qtr)) {
                    bits_to_follow++;
                    low_v -= first_qtr;
                    high_v -= first_qtr;
                } else {
                    break;
                }

                low_v += low_v;
                high_v += high_v + 1;
            }
        } else {  // encode the EOF symbol
            high_v = low_v +  table[1] * diff / delitel - 1;
            low_v = low_v + table[0] * diff  / delitel;

            for (;;) {
                if (high_v < half) {
                    bitsPlusFollow(0, &bits_to_follow,
                                    &bit_len, &write_bit, output_file);
                } else if (low_v >= half) {
                    bitsPlusFollow(1, &bits_to_follow,
                                    &bit_len, &write_bit, output_file);
                    low_v -= half;
                    high_v -= half;
                } else if ((low_v >= first_qtr) && (high_v < third_qtr)) {
                    bits_to_follow++;
                    low_v -= first_qtr;
                    high_v -= first_qtr;
                } else {
                    break;
                }

                low_v += low_v;
                high_v += high_v + 1;
            }

            bits_to_follow+=1;

            if (low_v < first_qtr) {
                bitsPlusFollow(0, &bits_to_follow,
                                &bit_len, &write_bit, output_file);
            } else {
                bitsPlusFollow(1, &bits_to_follow,
                                &bit_len, &write_bit, output_file);
            }

            write_bit >>= bit_len;
            fputc(write_bit, output_file);
        }
        diff = high_v - low_v + 1;
    }

    fclose(input_file);
    fclose(output_file);

    unsigned int file_full_size = 0;
    unsigned int commpres_size = 0;
    struct stat sb{};
    struct stat se{};

    // Finding compression ratio
    if (!stat(input_name, &sb)) {
        file_full_size = sb.st_size;
    } else {
        perror("stat");
    }
    if (!stat(output_name, &se)) {
        commpres_size = se.st_size;
    } else {
        perror("stat");
    }

    return (commpres_size + 0.0 ) / file_full_size;
}

int main() {
    std::cout << coder() << std::endl;  // Print compression ratio
}