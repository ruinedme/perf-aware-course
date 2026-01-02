#ifndef SAX_JSON_H
#define SAX_JSON_H

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    void (*start_object)(void *ud);
    void (*end_object)(void *ud);
    void (*start_array)(void *ud);
    void (*end_array)(void *ud);
    void (*key)(void *ud, const char *key);
    void (*string)(void *ud, const char *value);
    void (*number)(void *ud, const char *num_text, size_t len);
    void (*boolean)(void *ud, bool boolean_value);
    void (*null_value)(void *ud);
    void (*error)(void *ud, const char *msg, size_t pos);
} json_sax_handler_t;

/// @brief 
/// @param filename The file to parse. If filename is NULL, defaults to stdin
/// @param h SAX callback handlers
/// @param ud User Data struct that is passed to the handlers
/// @return True if parsing completed successfully. False on any error
bool parse_file_with_sax(const char *filename, const json_sax_handler_t *h, void *ud);

#endif
