#ifndef EDITOR_H
#define EDITOR_H

int editor_open_file(const char *filename);
int editor_close_file(int fd);
int editor_append_line(int fd, const char *text);
int editor_print_file(int fd);
int editor_print_line(int fd, int line_number);
int editor_delete_line(int fd, int line_number);
int editor_insert_line(int fd, int line_number, const char *text);
int editor_search_word(int fd, const char *word);
#endif
