
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#ifndef BROWSER_PARSER_H
#define BROWSER_PARSER_H

#define MAX_TAGS 2000

static char *tag_names[] = {
    "div", "span", "strong", "img", "title", "section", "pre", "script", "p",
    "a",   "h1",   "h2",     "h3",  "ul",    "li",      "em",  "br"};
static const int supported_count = sizeof(tag_names) / sizeof(tag_names[0]);

static char *convert_names[] = {"&copy;", "&lt;",   "&gt;",
                                "&amp;",  "&quot;", "&apos;"};
static char *convert_values[] = {"©", "<", ">", "&", "\"", "'"};
static const int convert_count =
    sizeof(convert_names) / sizeof(convert_names[0]);

typedef enum {
  START_TAG,
  END_TAG,
  START_TAG_ONLY,
  PLAIN_TEXT,
  TK_EOF
} TokenKind;

typedef enum {
  TAG_DIV,
  TAG_SPAN,
  TAG_STRONG,
  TAG_IMG,
  TAG_TITLE,
  TAG_SECTION,
  TAG_PRE,
  TAG_SCRIPT,
  TAG_P,
  TAG_A,
  TAG_H1,
  TAG_H2,
  TAG_H3,
  TAG_UL,
  TAG_LI,
  TAG_EM,
  TAG_BR
} TagKind;

typedef enum { FONT_NORMAL, FONT_BOLD, FONT_ITALIC } FontKind;
typedef enum { TEXT_NONE, TEXT_UNDERLINE } TextDecoration;
typedef enum { DISPLAY_NONE, DISPLAY_BLOCK, DISPLAY_INLINE } Display;

typedef struct CssProperty CssProperty;

struct CssProperty {
  SDL_Color color;
  int font_size;
  FontKind font_weight;
  FontKind font_style;
  TextDecoration text_decoration;
  Display display;
};

typedef struct Token Token;

struct Token {
  TokenKind kind;
  Token *next;
  Token *parent;
  TagKind tag;
  CssProperty *css_property;
  char text[4000];
  char html_id[200];
  char html_class[200];
};

void error(char *fmt, ...);

void warning(char *fmt, ...);

Token *parse_html(char *file_name);

#endif
