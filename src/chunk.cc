#include "chunk.hpp"

using namespace aup;

void Chunk::emit(uint8_t byte, int line, int column)
{
    code.push_back(byte);
    lines.push_back(line);
    columns.push_back(column);
}

char *readFile(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == nullptr) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return nullptr;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = new char[fileSize + 1];
    if (buffer == nullptr) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return nullptr;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        fclose(file);
        delete[] buffer;
        return nullptr;
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    if (size) *size = bytesRead;
    return buffer;
}

Source::Source(const char *file)
{
    buffer = readFile(file, &size);

    const char *s;
    if ((s = strrchr(file, '/')) != nullptr) s++;
    if ((s = strrchr(file, '\\')) != nullptr) s++;
    if (s == nullptr) s = file;

    fname = strdup(s);
}

Source::~Source()
{
    delete[] fname;
    delete[] buffer;
}

bool Source::validate()
{
    return buffer != nullptr && size > 0;
}