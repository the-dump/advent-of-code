#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

struct vector_2
{
    double x;
    double y;
};

struct line
{
    struct vector_2 start;
    struct vector_2 end;
};

union line_array
{
    struct line line;
    double array[4];
};

void skip_non_digit(char* str, char** end)
{
    char* a = *end;

    while (str != a)
    {
        if (isdigit(*str)) break;
        str += 1;
    }

    *end = str;
}

int read_input(const char* name, struct line* array, size_t* length)
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

        union line_array line_array;
        memset(&line_array, 0, sizeof(union line_array));
        
        size_t value_index = 0;
        
        for (size_t value_index = 0; value_index < sizeof(union line_array) / sizeof(double); ++value_index)
        {
            end = last;
            skip_non_digit(str, &end);
            
            str = end;
            
            end = last;
            uint64_t value = strtoull(str, &end, 10);
            
            if (str != end) line_array.array[value_index] = (double) value;
            if (str == last) break;
            
            str = end;
        }
        
        if (str != last) array[index++] = line_array.line;
        if (str == last) break;
    }
    
    *length = index;
    
    cleanup: if (file != -1) close(file);
    if (mem != NULL) free(mem);
    
    return file;
}

size_t ceil_up(size_t value, size_t multiple)
{
    size_t remainder = value % multiple;

    if (remainder != 0)
    {
        value += multiple;
        value -= remainder;
    }
    
    return value;
}

size_t draw_one_line(uint64_t* canvas, uint64_t* other_canvas, size_t canvas_width, size_t canvas_height, const struct line* line)
{
    size_t count = 0;
    
    struct vector_2 start = line->start;
    struct vector_2 end = line->end;

    double direction_x = end.x - start.x;
    double direction_y = end.y - start.y;

    double mag = hypot(direction_x, direction_y);

    double unit_x = round(direction_x / mag);
    double unit_y = round(direction_y / mag);
    
    double x = start.x;
    double y = start.y;

    size_t word_bits = sizeof(uint64_t) * CHAR_BIT;

    while (1)
    {
        size_t bit_index = (size_t) round(y) * canvas_width + (size_t) round(x);
        
        size_t index = bit_index / word_bits;
        size_t shift = bit_index % word_bits;
        
        uint64_t word_1 = canvas[index];
        int bit_1 = (word_1 >> shift) & 1;

        uint64_t word_2 = other_canvas[index];
        int bit_2 = (word_2 >> shift) & 1;
        
        uint64_t set = (uint64_t) 1 << shift;
        if (!bit_1) canvas[index] = word_1 | set; 
        
        if (bit_1 && !bit_2)
        {
            count += 1;
            other_canvas[index] = word_2 | set;
        }
        
        double diff_x = end.x - x;
        double diff_y = end.y - y;
        
        if (round(diff_x) == 0 && round(diff_y) == 0) break;

        x += unit_x;
        y += unit_y;
    }
    
    return count;
}

void print_canvas(const uint64_t* canvas, size_t canvas_width, size_t canvas_height)
{
    size_t word_bits = sizeof(uint64_t) * CHAR_BIT;

    for (size_t y = 0; y < canvas_height; ++y)
    {
        for (size_t x = 0; x < canvas_width; ++x)
        {
            size_t bit_index = y * canvas_width + x;

            size_t index = bit_index / word_bits;
            size_t shift = bit_index % word_bits;

            uint64_t word = canvas[index];
            int bit = (word >> shift) & 1;

            switch (bit)
            {
                case 0: printf("⬜"); break;
                case 1: printf("⬛"); break;
            }
        }
        
        printf("\n");
    }
}

int main(int argc, char** argv)
{
    if (argc < 5) return EXIT_FAILURE;

    struct line* array = NULL;

    uint64_t* canvas = NULL;
    uint64_t* other_canvas = NULL;

    size_t length = strtoull(argv[2], NULL, 10);

    size_t canvas_width = strtoull(argv[3], NULL, 10);
    size_t canvas_height = strtoull(argv[4], NULL, 10);

    size_t canvas_area = canvas_width * canvas_height;
    
    array = malloc(length * sizeof(struct line));
    if (array == NULL) goto cleanup;

    if (read_input(argv[1], array, &length) == -1)
    {
        perror("Failed to read input file");
        return EXIT_FAILURE;
    }
    
    size_t word_size = sizeof(uint64_t) * CHAR_BIT;
    size_t canvas_mem_bytes = ceil_up(canvas_area, word_size) / CHAR_BIT;

    canvas = malloc(canvas_mem_bytes);
    if (canvas == NULL) goto cleanup;

    other_canvas = malloc(canvas_mem_bytes);
    if (other_canvas == NULL) goto cleanup; 

    memset(canvas, 0, canvas_mem_bytes);
    memset(other_canvas, 0, canvas_mem_bytes);
    
    size_t perpendicular_overlapping_line_points = 0;
    size_t every_overlapping_line_points = 0;

    for (size_t index = 0; index < length; ++index)
    {
        const struct line* line = &array[index];

        struct vector_2 start = line->start;
        struct vector_2 end = line->end;

        every_overlapping_line_points += draw_one_line(canvas, other_canvas, canvas_width, canvas_height, line);
    }
    
    memset(canvas, 0, canvas_mem_bytes);
    memset(other_canvas, 0, canvas_mem_bytes);

    for (size_t index = 0; index < length; ++index)
    {
        const struct line* line = &array[index];

        struct vector_2 start = line->start;
        struct vector_2 end = line->end;
        
        if (start.x != end.x && start.y != end.y) continue;
        perpendicular_overlapping_line_points += draw_one_line(canvas, other_canvas, canvas_width, canvas_height, line);
    }

    printf("ANSWER PART I: %zu\n", perpendicular_overlapping_line_points);
    printf("ANSWER PART II: %zu\n", every_overlapping_line_points);
    
    cleanup: if (array != NULL) free(array);
    if (canvas != NULL) free(canvas);
    
    return EXIT_SUCCESS;
}
