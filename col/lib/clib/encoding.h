#ifndef ENCODING_H
#define ENCODING_H

#include <zlog.h>
extern zlog_category_t *c;

char *get_encoded_msg(char *buffer, char *charset);

#endif