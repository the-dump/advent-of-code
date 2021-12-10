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

#define MARKED ((uint64_t) -1)

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

void skip_newline(char* str, char** end)
{
    char* a = *end;

    char* b = str + 1;
    char* c = str + 2;

    char* d = str;

    if (str != a && *str == '\n') d = b;    
    if (str != a && b != a && *str == '\r' && *b == '\n') d = c;

    *end = d;
}

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

int read_input(const char* name, uint64_t* array, size_t* length, uint64_t* board_numbers, size_t width, size_t height, size_t* total_boards)
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
        skip_non_digit(str, &end);
        
        str = end;
        
        end = last;
        uint64_t value = strtoull(str, &end, 10);
        
        if (str != end) array[index++] = value;
        if (str == last) break;
        
        str = end;

        end = last;
        skip_newline(str, &end);
        
        char* temp = str;
        str = end;

        if (temp != end) break;
        if (temp == last) break;
    }

    *length = index;
    
    size_t board_numbers_index = 0;
    size_t board_numbers_max_length = width * height * (*total_boards);
    
    while (board_numbers_index < board_numbers_max_length)
    {
        char* end;

        end = last;
        skip_space(str, &end);

        str = end;
        
        end = last;
        uint64_t value = strtoull(str, &end, 10);
        
        if (str != end) board_numbers[board_numbers_index++] = value;
        if (str == last) break;
        
        str = end;
    }
    
    *total_boards = board_numbers_index / (width * height);
    
    cleanup: if (file != -1) close(file);
    if (mem != NULL) free(mem);
    
    return file;
}

size_t find_drawn_on_board(const uint64_t* board, size_t width, size_t height, uint64_t drawn)
{
    for (size_t index = 0; index < width * height; ++index)
    {
        if (board[index] == drawn) return index;
    }

    return -1;
}

int check_board_row(const uint64_t* board, size_t row_index, size_t width)
{
    size_t count = 0;
    size_t offset = row_index * width;

    for (size_t index = offset; index < offset + width; ++index)
    {
        if (board[index] == MARKED) count += 1; 
    }

    return count == width;
}

int check_board_column(const uint64_t* board, size_t column_index, size_t width, size_t height)
{
    size_t count = 0;
    size_t offset = column_index;

    for (size_t index = offset; index < width * height; index += width)
    {
        if (board[index] == MARKED) count += 1;
    }

    return count == height;
}

void bingo_draw_one(uint64_t drawn, uint64_t* board_numbers, size_t width, size_t height, size_t total_boards, size_t* winners, size_t* total_winners)
{
    size_t winner_index = 0;
    size_t board_size = width * height;

    for (size_t board_index = 0; board_index < total_boards; ++board_index)
    {
        uint64_t* board = &board_numbers[board_index * board_size];
        size_t drawn_index = find_drawn_on_board(board, width, height, drawn);

        if (drawn_index >= board_size) continue;
        board[drawn_index] = MARKED;

        size_t row_index = drawn_index / width;
        size_t column_index = drawn_index % width;
        
        if (check_board_row(board, row_index, width) || check_board_column(board, column_index, width, height))
        {
             winners[winner_index++] = board_index;        
        }
    }

    *total_winners = winner_index;
}

size_t bingo_game(const uint64_t* array, size_t length, uint64_t* board_numbers, size_t width, size_t height, size_t total_boards, size_t* winners, size_t* total_winners)
{
    for (size_t index = 0; index < length; ++index)
    {
        uint64_t drawn = array[index];
        bingo_draw_one(drawn, board_numbers, width, height, total_boards, winners, total_winners);
   
        if (*total_winners > 0) return index;
    }
    
    return length;
}

uint64_t calculate_unmarked_sum(const uint64_t* board, size_t width, size_t height)
{
    uint64_t sum = 0;
    size_t board_size = width * height;

    for (size_t index = 0; index < board_size; ++index)
    {
        uint64_t number = board[index];
        if (number != MARKED) sum += number;
    }
    
    return sum;
}

int main(int argc, char** argv)
{
    if (argc < 6) return EXIT_FAILURE;

    uint64_t* array = NULL;
    uint64_t* board_numbers = NULL;
    size_t* winners = NULL;

    size_t length = strtoull(argv[2], NULL, 10); 

    size_t width = strtoull(argv[3], NULL, 10);
    size_t height = strtoull(argv[4], NULL, 10);

    size_t total_boards = strtoull(argv[5], NULL, 10);

    array = malloc(length * sizeof(uint64_t));
    if (array == NULL) goto cleanup;

    size_t board_size = width * height;

    board_numbers = malloc(board_size * total_boards * sizeof(uint64_t));
    if (board_numbers == NULL) goto cleanup;
    
    if (read_input(argv[1], array, &length, board_numbers, width, height, &total_boards) == -1)
    {
        perror("Failed to read input file");
        return EXIT_FAILURE;
    }
    
    winners = malloc(total_boards * sizeof(size_t));
    if (winners == NULL) goto cleanup;

    size_t total_winners;
    size_t last_drawn_index = bingo_game(array, length, board_numbers, width, height, total_boards, winners, &total_winners);
    
    uint64_t last_drawn_1 = 0;
    if (last_drawn_index < length) last_drawn_1 = array[last_drawn_index];
    
    uint64_t unmarked_sum_1 = 0;
    
    if (total_winners >= 1)
    {
        size_t board_numbers_index = winners[0] * board_size;
        unmarked_sum_1 = calculate_unmarked_sum(&board_numbers[board_numbers_index], width, height);
    }
    
    while (last_drawn_index != length && total_boards > 1)
    {
        for (size_t winner_index = 0; winner_index < total_winners; ++winner_index)
        {
            total_boards -= 1;

            size_t index_1 = winners[winner_index] * board_size;
            size_t index_2 = total_boards * board_size;

            for (size_t offset = 0; offset < board_size; ++offset)
            {
                uint64_t temp = board_numbers[index_1 + offset];
                
                board_numbers[index_1 + offset] = board_numbers[index_2 + offset];
                board_numbers[index_2 + offset] = temp;
            }
        }

        size_t a = last_drawn_index + 1;
        size_t b = bingo_game(&array[a], length - a, board_numbers, width, height, total_boards, winners, &total_winners);
        
        last_drawn_index = a + b;
    }

    uint64_t last_drawn_2 = 0;
    if (last_drawn_index < length) last_drawn_2 = array[last_drawn_index];

    uint64_t unmarked_sum_2 = 0;
    if (total_boards >= 1) unmarked_sum_2 = calculate_unmarked_sum(board_numbers, width, height);
    
    printf("ANSWER PART I: %" PRIu64 "\n", last_drawn_1 * unmarked_sum_1);
    printf("ANSWER PART II: %" PRIu64 "\n", last_drawn_2 * unmarked_sum_2);
    
    cleanup: if (array != NULL) free(array);
    if (board_numbers != NULL) free(board_numbers);
    if (winners != NULL) free(winners);
    
    return EXIT_SUCCESS;
}
