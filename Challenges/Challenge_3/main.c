#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

enum mode
{
    mode_none = 0,
    mode_most_common = 1,
    mode_least_common = 2,
};

void skip_space(char* str, char** end)
{
    char* a = *end;

    while (str != a)
    {
        if (!isspace(*str)) break;
        str += 1;
    }

    *end = str;
}

int read_input(const char* name, uint64_t* array, size_t* length)
{
    int file = -1;
    char* mem = NULL;
    
    file = open(name, O_RDONLY);
    if (file == -1) goto cleanup;

    struct stat64 stat;
    fstat64(file, &stat);

    mem = malloc(stat.st_size + 1);
    if (mem == NULL) goto cleanup;

    size_t mem_index = 0;
    size_t mem_remaining = stat.st_size;

    while (mem_remaining != 0)
    {
        ssize_t chars = read(file, mem + mem_index, mem_remaining);
        if (chars == -1) goto cleanup;

        mem_index += chars;
        mem_remaining -= chars;
    }
    
    char* last = mem + mem_index;
    *last = 0;

    char* str = mem;

    size_t index = 0;
    size_t max_length = *length;

    while (index < max_length)
    {
        char* end;

        end = last;
        skip_space(str, &end);

        str = end;
        
        end = last;
        uint64_t value = strtoull(str, &end, 2);
        
        if (str != end) array[index++] = value;
        if (str == last) break;
        
        str = end;
    }
    
    *length = index;
    
    cleanup: if (file != -1) close(file);
    if (mem != NULL) free(mem);
    
    return file;
}

void count_ones_in_columns(const uint64_t* array, size_t length, size_t column_offset, uint64_t* column_counts, size_t total_columns)
{
    for (size_t row_index = 0; row_index < length; ++row_index)
    {
        uint64_t row = array[row_index];   
        row >>= column_offset; 
    
        for (size_t column_index = 0; column_index < total_columns; ++column_index)
        {
            column_counts[column_index] += row & 1;
            row >>= 1;
        }
    }
}

uint64_t most_or_least_common_bit_in_columns(const uint64_t* array, size_t length, size_t column_offset, uint64_t* column_counts, size_t total_columns, enum mode mode)
{
    uint64_t result = 0;
    
    memset(column_counts, 0, total_columns * sizeof(uint64_t));
    count_ones_in_columns(array, length, column_offset, column_counts, total_columns);

    for (size_t column_index = 0; column_index < total_columns; ++column_index)
    {
        uint64_t ones = column_counts[column_index];
        uint64_t zeros = length - ones;
        
        uint64_t bit = 0;
        
        switch (mode)
        {
            case mode_most_common: bit = ones >= zeros; break;
            case mode_least_common: bit = ones < zeros; break;
        }
        
        result |= bit << column_index;
    }
    
    return result;
}

size_t calculate_rating(uint64_t* array, size_t length, size_t total_columns, enum mode mode)
{
    size_t column_index = total_columns;

    while (!__builtin_sub_overflow(column_index, 1, &column_index))
    {
        size_t row_index = 0;
        
        uint64_t column_count = 0;
        uint64_t rate = most_or_least_common_bit_in_columns(array, length, column_index, &column_count, 1, mode);
        
        uint64_t bit = 0;
        if (rate) bit = 1;
        
        while (row_index < length)
        {
            if (length == 1) return length;
            
            uint64_t row = array[row_index];
            uint64_t column_bit = (row >> column_index) & 1;

            if (column_bit != bit)
            {
                length -= 1;
                
                array[row_index] = array[length];
                array[length] = row;

                continue;
            }

            row_index += 1;
        }
    }
    
    return length;
}

int main(int argc, char** argv)
{
    if (argc < 4) return EXIT_FAILURE;

    uint64_t* array = NULL;
    uint64_t* column_counts = NULL;

    size_t length = strtoull(argv[2], NULL, 10);
    size_t total_columns = strtoull(argv[3], NULL, 10);

    array = malloc(length * sizeof(uint64_t));
    if (array == NULL) goto cleanup;

    if (read_input(argv[1], array, &length) == -1)
    {
        perror("Failed to read input file");
        return EXIT_FAILURE;
    }
    
    column_counts = malloc(total_columns * sizeof(uint64_t));
    if (column_counts == NULL) goto cleanup;
    
    uint64_t gamma_rate = most_or_least_common_bit_in_columns(array, length, 0, column_counts, total_columns, mode_most_common);
    uint64_t eplison_rate = most_or_least_common_bit_in_columns(array, length, 0, column_counts, total_columns, mode_least_common);
    
    uint64_t oxygen = 0;
    uint64_t carbon = 0;
    
    if (calculate_rating(array, length, total_columns, mode_most_common)) oxygen = array[0];
    if (calculate_rating(array, length, total_columns, mode_least_common)) carbon = array[0];
    
    printf("ANSWER PART I: %" PRIu64 "\n", gamma_rate * eplison_rate);
    printf("ANSWER PART II: %" PRIu64 "\n", oxygen * carbon);
    
    cleanup: if (array != NULL) free(array);
    if (column_counts != NULL) free(column_counts);
    
    return EXIT_SUCCESS;
}
