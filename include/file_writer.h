#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "file_reader.h"

void write_text(const char *filename, const ParsedData *data);
void write_binary(const char *filename, const ParsedData *data);
void encode_vbyte(FILE *file, int value);

#endif // FILE_WRITER_H