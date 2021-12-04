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

enum direction
{
    direction_none = 0,
    direction_forward = 1,
    direction_up = 2,
    direction_down = 3,
};

struct movement
{
    enum direction direction;
    uint64_t value;
};

struct position
{
    int64_t horizontal;
    int64_t depth;
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

void skip_alpha(char* str, char** end)
{
    char* a = *end;

    while (str != a)
    {
        if (!isalpha(*str)) break;
        str += 1;
    }

    *end = str;
}

int read_input(const char* name, struct movement* array, size_t* length)
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
        skip_alpha(str, &end);

        size_t diff = end - str;
        enum direction direction = direction_none;

        if (!strncmp(str, "forward", diff)) direction = direction_forward;
        if (!strncmp(str, "up", diff)) direction = direction_up;
        if (!strncmp(str, "down", diff)) direction = direction_down;
        
        str = end;

        end = last;
        uint64_t value = strtoull(str, &end, 10);
        
        struct movement movement;
        
        movement.direction = direction;
        movement.value = value;

        if (str != end) array[index++] = movement;
        if (str == last) break;
        
        str = end;
    }
    
    *length = index;
    
    cleanup: if (file != -1) close(file);
    if (mem != NULL) free(mem);
    
    return file;
}

struct position calculate_final_position(const struct movement* array, size_t length)
{
    struct position current_position = {0, 0};

    for (size_t index = 0; index < length; ++index)
    {
        struct movement movement = array[index];

        enum direction direction = movement.direction;
        uint64_t value = movement.value;

        struct position offset = {0, 0};

        switch (direction)
        {
            case direction_forward: offset.horizontal += value; break;
            case direction_up: offset.depth -= value; break;
            case direction_down: offset.depth += value; break;
        }

        current_position.horizontal += offset.horizontal;
        current_position.depth += offset.depth;
    }
    
    return current_position;
}

struct position calculate_final_position_with_aim(const struct movement* array, size_t length)
{
    struct position current_position = {0, 0};
    uint64_t aim = 0;

    for (size_t index = 0; index < length; ++index)
    {
        struct movement movement = array[index];

        enum direction direction = movement.direction;
        uint64_t value = movement.value;

        switch (direction)
        {
            case direction_up: aim -= value; break;
            case direction_down: aim += value; break;
        }
        
        if (direction == direction_forward)
        {
            current_position.horizontal += value;
            current_position.depth += aim * value;
        }
    }
    
    return current_position;
}

int main(int argc, char** argv)
{
    if (argc < 3) return EXIT_FAILURE;
    
    size_t length = strtoull(argv[2], NULL, 10);
    struct movement* array = malloc(length * sizeof(struct movement));

    if (read_input(argv[1], array, &length) == -1)
    {
        perror("Failed to read input file");
        return EXIT_FAILURE;
    }
    
    struct position final_position_1 = calculate_final_position(array, length);
    struct position final_position_2 = calculate_final_position_with_aim(array, length);

    printf("ANSWER PART I: %" PRId64 "\n", final_position_1.horizontal * final_position_1.depth);
    printf("ANSWER PART II: %" PRId64 "\n", final_position_2.horizontal * final_position_2.depth);
    
    free(array);

    return EXIT_SUCCESS;
}
