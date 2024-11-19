
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAGS 200

typedef enum { START_TAG, END_TAG, START_TAG_ONLY, PLAIN_TEXT, TK_EOF } TokenKind;

typedef enum {
    TAG_DIV,
    TAG_SPAN,
    TAG_P,
    TAG_A,
    TAG_H1,
    TAG_H2,
    TAG_H3,
    TAG_UL,
    TAG_LI,
    TAG_EM,
    TAG_STRONG,
    TAG_BR,
    TAG_IMG,
    TAG_TITLE
} TagKind;
typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    TagKind tag;
    char text[200];
    char html_id[20];
    char html_class[20];
};

void error(char *fmt, ...);

void warning(char *fmt, ...);

Token *parse_html(char *file_name);
