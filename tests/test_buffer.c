#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../buffer.h"

static FILE *file_with_text(const char *text)
{
    FILE *file = tmpfile();
    assert(file != NULL);
    assert(fputs(text, file) >= 0);
    rewind(file);
    return file;
}

static size_t read_file(FILE *file, char *destination, size_t capacity)
{
    rewind(file);
    size_t length = fread(destination, 1, capacity, file);
    assert(!ferror(file));
    return length;
}

static void test_empty_file(void)
{
    FILE *input = tmpfile();
    assert(input != NULL);

    Buffer buffer;
    assert(buffer_init(&buffer) == 0);
    assert(buffer_load(&buffer, input) == 0);
    assert(buffer_line_count(&buffer) == 1);
    assert(strcmp(buffer_line_at(&buffer, 0), "") == 0);

    FILE *output = tmpfile();
    assert(output != NULL);
    assert(buffer_save(&buffer, output) == 0);
    char contents[2] = {'x', '\0'};
    assert(read_file(output, contents, sizeof(contents)) == 0);

    fclose(output);
    fclose(input);
    buffer_free(&buffer);
}

static void test_long_lines_and_crlf(void)
{
    FILE *input = tmpfile();
    assert(input != NULL);
    for (size_t i = 0; i < 5000; ++i) {
        assert(fputc('x', input) != EOF);
    }
    assert(fputs("\r\nsecond", input) >= 0);
    rewind(input);

    Buffer buffer;
    assert(buffer_init(&buffer) == 0);
    assert(buffer_load(&buffer, input) == 0);
    assert(buffer_line_count(&buffer) == 2);
    assert(buffer_line_length(&buffer, 0) == 5000);
    assert(buffer_line_length(&buffer, 1) == 6);
    assert(strcmp(buffer_line_at(&buffer, 1), "second") == 0);

    fclose(input);
    buffer_free(&buffer);
}

static void test_editing(void)
{
    Buffer buffer;
    assert(buffer_init(&buffer) == 0);

    assert(buffer_insert_char(&buffer, 0, 0, 'h') == 0);
    assert(buffer_insert_char(&buffer, 0, 1, 'i') == 0);
    assert(strcmp(buffer_line_at(&buffer, 0), "hi") == 0);

    assert(buffer_split_line(&buffer, 0, 1) == 0);
    assert(buffer_line_count(&buffer) == 2);
    assert(strcmp(buffer_line_at(&buffer, 0), "h") == 0);
    assert(strcmp(buffer_line_at(&buffer, 1), "i") == 0);

    assert(buffer_insert_char(&buffer, 1, 1, '!') == 0);
    assert(buffer_join_lines(&buffer, 0) == 0);
    assert(strcmp(buffer_line_at(&buffer, 0), "hi!") == 0);
    assert(buffer_delete_char(&buffer, 0, 2) == 0);
    assert(strcmp(buffer_line_at(&buffer, 0), "h!") == 0);
    assert(buffer_is_modified(&buffer));

    buffer_mark_clean(&buffer);
    assert(!buffer_is_modified(&buffer));
    buffer_free(&buffer);
}

static void test_round_trip(void)
{
    const char *text = "one\ntwo\n";
    FILE *input = file_with_text(text);

    Buffer buffer;
    assert(buffer_init(&buffer) == 0);
    assert(buffer_load(&buffer, input) == 0);

    FILE *output = tmpfile();
    assert(output != NULL);
    assert(buffer_save(&buffer, output) == 0);

    char contents[32] = {0};
    size_t length = read_file(output, contents, sizeof(contents) - 1);
    contents[length] = '\0';
    assert(strcmp(contents, text) == 0);

    fclose(output);
    fclose(input);
    buffer_free(&buffer);
}

int main(void)
{
    test_empty_file();
    test_long_lines_and_crlf();
    test_editing();
    test_round_trip();
    puts("buffer tests: ok");
    return 0;
}
