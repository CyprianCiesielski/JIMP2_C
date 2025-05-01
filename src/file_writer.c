#include "file_writer.h"
#include <stdint.h> 

// Funkcja do zapisu danych do pliku tekstowego
void write_text(const char *filename, const ParsedData *data) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Nie można otworzyć pliku tekstowego do zapisu");
        exit(EXIT_FAILURE);
    }

    // Zapis pierwszej linii
    fprintf(file, "%d\n", *(data->line1));

    // Zapis drugiej linii
    for (int i = 0; i < data->line2_count; i++) {
        fprintf(file, "%d", data->line2[i]);
        if (i < data->line2_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    // Zapis trzeciej linii
    for (int i = 0; i < data->line3_count; i++) {
        fprintf(file, "%d", data->line3[i]);
        if (i < data->line3_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    // Zapis czwartej linii
    for (int i = 0; i < data->edge_count; i++) {
        fprintf(file, "%d", data->edges[i]);
        if (i < data->edge_count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");

    fclose(file);
}

// Funkcja pomocnicza do kodowania liczby w formacie vByte
void encode_vbyte(FILE *file, int value) {
    while (value >= 128) {
        uint8_t byte = (value & 0x7F) | 0x80; // 7 bitów wartości + MSB = 1
        fwrite(&byte, sizeof(uint8_t), 1, file);
        value >>= 7; // Przesunięcie o 7 bitów
    }
    uint8_t byte = value & 0x7F; // Ostatni bajt z MSB = 0
    fwrite(&byte, sizeof(uint8_t), 1, file);
}

// Zapisywanie do pliku binarnego
void write_binary(const char *filename, const ParsedData *data) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Nie można otworzyć pliku binarnego do zapisu");
        exit(EXIT_FAILURE);
    }

    // Separator w formacie little-endian
    const uint64_t separator = 0xDEADBEEFCAFEBABE;

    // Zapis pierwszej linii (liczba wierzchołków)
    encode_vbyte(file, *(data->line1));

    // Zapis separatora między pierwszym a drugim wierszem
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Zapis drugiej linii (lista sąsiadów)
    for (int i = 0; i < data->line2_count; i++) {
        encode_vbyte(file, data->line2[i]);
    }

    // Zapis separatora między drugim a trzecim wierszem
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Zapis trzeciej linii (row_pointers)
    for (int i = 0; i < data->line3_count; i++) {
        encode_vbyte(file, data->line3[i]);
    }

    // Zapis separatora między trzecim a czwartym wierszem
    fwrite(&separator, sizeof(uint64_t), 1, file);

    // Zapis czwartej linii (krawędzie)
    for (int i = 0; i < data->edge_count; i++) {
        encode_vbyte(file, data->edges[i]);
    }

    fclose(file);
}