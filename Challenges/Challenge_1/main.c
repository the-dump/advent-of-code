#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

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
    
    mem[mem_index] = 0;
    char* str = mem;

    size_t index = 0;
    size_t max_length = *length;

    while (index < max_length)
    {
        char* end = mem + mem_index;
        uint64_t value = strtoull(str, &end, 10);
        
        if (str != end)
        {
            array[index] = value;
            index += 1;
        }
        
        if (str == mem + mem_index) break;
        str = end + 1;
    }
    
    *length = index;
    
    cleanup: if (file != -1) close(file);
    if (mem != NULL) free(mem);
    
    return file;
}

uint64_t count_increasing_pairs(const uint64_t* array, size_t length)
{
    if (length == 0) return 0;

    uint64_t count = 0;
    uint64_t previous = array[0];

    for (uint64_t index = 1; index < length; ++index)
    {
        if (array[index] > previous) count += 1;
        previous = array[index];
    }
    
    return count;
}

uint64_t count_increasing_3_segment_windows(const uint64_t* array, size_t length)
{
    if (length < 3) return 0;

    uint64_t count = 0;

    uint64_t previous_1 = array[0];
    uint64_t previous_2 = array[1];
    uint64_t previous_3 = array[2];

    uint64_t previous_sum = previous_1 + previous_2 + previous_3;

    for (uint64_t index = 3; index < length; ++index)
    {
        uint64_t sum = previous_2 + previous_3 + array[index];
        if (sum > previous_sum) count += 1;

        previous_1 = previous_2;
        previous_2 = previous_3;
        previous_3 = array[index];
        
        previous_sum = sum;
    }
    
    return count;
}

int main(int argc, char** argv)
{
    if (argc < 2) return EXIT_FAILURE;

    size_t length = 5000;
    uint64_t* array = malloc(length * sizeof(uint64_t));
    
    if (read_input(argv[1], array, &length) == -1)
    {
        perror("Failed to read input file");
        return EXIT_FAILURE;
    }
    
    uint64_t answer_1 = count_increasing_pairs(array, length);
    uint64_t answer_2 = count_increasing_3_segment_windows(array, length);
    
    printf("ANSWER PART I: %" PRIu64 "\n", answer_1);
    printf("ANSWER PART II: %" PRIu64 "\n", answer_2);
    
    return EXIT_SUCCESS;
}
