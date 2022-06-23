#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>

// Number of bits in a code value
#define code_value_bits 16

// Input a bit
int input_bit(unsigned char* read_bit, unsigned int* bit_len, FILE* input_file, unsigned int* garbage_bit)
{
    if ( (*bit_len) == 0 )
    {
        (*read_bit) = fgetc(input_file);
        if (feof(input_file))
        {
            (*garbage_bit)++;
            if ( (*garbage_bit) > 14 )
            {
                throw std::invalid_argument("Can't decompress");
            }
        }
        (*bit_len) = 8;
    }
    int t = (*read_bit) & 1;
    (*read_bit) >>= 1;
    (*bit_len)--;
    return t;
}

// Decoding function
void decoder(const char* input_name = "encoded.txt", const char* output_name = "output.txt")
{
    unsigned int * alfabet = new unsigned int[256];
    for (int i = 0; i < 256; i++)
    {
        alfabet[i] = 0;
    }
    FILE* input_file = fopen(input_name, "rb");  // Open input file
    if (input_file == nullptr)
    {
       throw std::invalid_argument("File not found.");
    }
    unsigned char col = 0;
    unsigned int col_letters = 0;
    col = fgetc(input_file);
    if (!feof(input_file))
    {
       col_letters = static_cast<unsigned int>(col);
    }

    unsigned char character = 0;

    // Reading the letters used and their number
    for (int i = 0; i < col_letters; i++)
    {
        character = fgetc(input_file);
        if (!feof(input_file))
        {
            fread(reinterpret_cast<char*>(&alfabet[character]), sizeof(uint16_t), 1, input_file);
        }
        else
        {
            throw std::invalid_argument("Can't decompress file.");
        }
    }

    std::vector<std::pair<char, unsigned int>> arr;
    for (int i = 0; i < 256; i++)
    {
        if (alfabet[i] != 0)
        {
            arr.push_back(std::make_pair(static_cast<char>(i), alfabet[i]));
        }
    }

    sort(arr.begin(), arr.end(),[](const std::pair<char, unsigned int> &l, const std::pair<char, unsigned int> &r)
    {
        if (l.second != r.second)
        {
            return l.second >= r.second;
        }
        return l.first < r.first;
    }
    );


    uint16_t* table = new uint16_t[arr.size() + 2];
    table[0] = 0;
    table[1] = 1;
    for (int i = 0; i < arr.size(); i++)
    {
        unsigned int b = arr[i].second;
        for (int j = 0; j < i; j++)
        {
            b += arr[j].second;
        }
        table[i+2] = b;
    }

    if (table[arr.size()] > (1 << ((code_value_bits - 2)) - 1))
    {
        throw std::invalid_argument("Error, too long count.");
    }

    unsigned int low_v = 0;
    unsigned int high_v = ((static_cast<unsigned int> (1) << code_value_bits) - 1);
    unsigned int delitel = table[arr.size()+1];
    unsigned int first_qtr = (high_v + 1) / 4;
    unsigned int half = first_qtr * 2;
    unsigned int third_qtr = first_qtr * 3;
    FILE* output_file = fopen(output_name, "wb +");
    unsigned int bit_len = 0;
    unsigned char read_bit = 0;
    unsigned int garbage_bit = 0;
    uint16_t value = 0;
    int k = 0;

    // Input bits to fill the code value
    for (int i = 1; i <= 16; i++)
    {
        k = input_bit(&read_bit, &bit_len, input_file, &garbage_bit);
        value = 2 * value + k;
    }
    unsigned int diff = high_v - low_v + 1;
    for (;;)
    {   // Read from input file
        unsigned int freq = static_cast<unsigned int> (( (static_cast<unsigned int>(value) - low_v + 1) * delitel - 1) / diff);

        int j;

        // Find symbol
        for (j = 1; table[j] <= freq; j++) {}
        high_v = low_v +  table[j] * diff / delitel - 1;
        low_v = low_v + table[j - 1] * diff  / delitel;

        for (;;) {
            if (high_v < half) {}
            else if (low_v >= half)
            {
                low_v -= half;
                high_v -= half;
                value -= half;
            }
            else if ((low_v >= first_qtr) && (high_v < third_qtr))
            {
                low_v -= first_qtr;
                high_v -= first_qtr;
                value -= first_qtr;
            }
            else
            {
                break;
            }

            low_v += low_v;
            high_v += high_v + 1;
            k = 0;
            k = input_bit(&read_bit, &bit_len, input_file, &garbage_bit);
            value += value + k;
        }

        if (j == 1)
        {
            break;
        }

        // Write this character
        fputc(arr[j-2].first, output_file);
        diff = high_v - low_v + 1;
    }

    fclose(input_file);
    fclose(output_file);
}

// Checking for file matches
unsigned int checker(const char* before_name = "input.txt", const char* after_name = "output.txt")
{
    unsigned int same = 0;
    FILE* before_file = fopen(before_name, "r");
    FILE* after_file = fopen(after_name, "r");

    unsigned char after_l = 0;
    unsigned char before_l = 0;
    while (!feof(after_file) && !feof(before_file))
    {
        after_l = fgetc(after_file);
        before_l = fgetc(before_file);
        if (!feof(after_file) && !feof(before_file))
        {
            if (after_l != before_l)
            {
                same++;
            }
        }
    }

    while (!feof(after_file))
    {
        after_l = fgetc(after_file);
        if (!feof(after_file))
        {
            same++;
        }
    }

    while (!feof(before_file))
    {
        before_l = fgetc(before_file);
        if (!feof(before_file))
        {
            same++;
        }
    }
    fclose(after_file);
    fclose(before_file);
    return same;
}

int main()
{
    decoder();
    std::cout << checker() << std::endl;
}