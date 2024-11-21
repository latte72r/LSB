
#include "parser.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *tag_names[] = {"div", "span", "p", "a", "h1", "h2", "h3", "ul", "li", "em", "strong", "br", "img", "title", "section"};
static const int supported_count = sizeof(tag_names) / sizeof(tag_names[0]);

char *convert_names[] = {"&copy;", "&lt;", "&gt;", "&amp;", "&quot;", "&apos;"};
char *convert_values[] = {"©", "<", ">", "&", "\"", "'"};
static const int convert_count = sizeof(convert_names) / sizeof(convert_names[0]);

char user_input[20000];
Token *token;
TagKind tag_stack[MAX_TAGS];
int tag_count = 0;

// Reports an error and exit.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "\e[0;31m");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\e[0;39m");
    exit(1);
}

void warning(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "\e[0;33m");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\e[0;39m");
}

Token *new_token(TokenKind kind, Token *cur) {
    Token *tok = calloc(1, sizeof(Token));
    CssProperty *css_property = calloc(1, sizeof(CssProperty));
    tok->kind = kind;
    cur->next = tok;
    return tok;
}

bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

void stack_push(TagKind tag) {
    tag_stack[tag_count++] = tag;
    if (tag_count >= MAX_TAGS) {
        error("Stack overflow\n");
    }
}

TagKind stack_pop() {
    tag_count--;
    if (tag_count < 0) {
        error("Stack underflow\n");
    }
    return tag_stack[tag_count];
}

bool consume_space(char **p) {
    int count = 0;
    while (isspace(**p)) {
        (*p)++;
        count++;
    }
    return count > 0;
}

SDL_Color parse_color(char color_string[20]) {
    SDL_Color color;
    consume_space(&color_string);
    if (startswith(color_string, "(")) {
        color_string++;
        consume_space(&color_string);
        color.r = strtol(color_string, &color_string, 10);
        if (startswith(color_string, ",")) {
            color_string++;
        } else {
            error("colorのパースに失敗しました: %s\n", color_string);
        }
        consume_space(&color_string);
        color.g = strtol(color_string, &color_string, 10);
        if (startswith(color_string, ",")) {
            color_string++;
        } else {
            error("colorのパースに失敗しました: %s\n", color_string);
        }
        consume_space(&color_string);
        color.b = strtol(color_string, &color_string, 10);
        consume_space(&color_string);
        if (startswith(color_string, ")")) {
            color_string++;
        } else {
            error("colorのパースに失敗しました: %s\n", color_string);
        }
    } else {
        error("colorのパースに失敗しました: %s\n", color_string);
    }
    return color;
}

CssProperty *parse_css(char *css_style) {
    CssProperty *css_property = calloc(1, sizeof(CssProperty));
    char buffer[20];
    consume_space(&css_style);
    while (*css_style) {
        consume_space(&css_style);
        if (!*css_style) {
            break;
        }
        if (startswith(css_style, "color:")) {
            css_style += 6;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            css_property->color = parse_color(buffer);
        } else if (startswith(css_style, "font-size:")) {
            css_style += 10;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            css_property->font_size = strtol(buffer, NULL, 10);
        } else if (startswith(css_style, "font-weight:")) {
            css_style += 12;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            if (startswith(buffer, "bold")) {
                css_property->font_bold = true;
            } else {
                css_property->font_bold = false;
            }
        } else if (startswith(css_style, "font-style:")) {
            css_style += 11;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            if (startswith(buffer, "italic")) {
                css_property->font_italic = true;
            } else {
                css_property->font_italic = false;
            }
        } else if (startswith(css_style, "text-decoration:")) {
            css_style += 15;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            if (startswith(buffer, "underline")) {
                css_property->text_underline = true;
            } else {
                css_property->text_underline = false;
            }
        } else {
            int i = 0;
            int j = 0;
            bool space2 = false;
            while (*(css_style + i) != ';') {
                if (*(css_style + i) == ':') {
                    space2 = true;
                } else if (!space2) {
                    buffer[j] = *(css_style + i);
                    j++;
                }
                i++;
            }
            buffer[j] = '\0';
            css_style += (i + 1);
            if (buffer[0] != '\0') {
                warning("CSSプロパティを無視しました: %s\n", buffer);
            }
            continue;
        }
    }
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;
    TagKind tag;
    CssProperty *css_property;
    bool tag_selected;
    bool spaced;

    while (*p) {
        spaced = consume_space(&p);
        if (!*p) {
            break;
        }
        tag_selected = false;

        if (startswith(p, "<!--")) {
            p += 4;
            while (!startswith(p, "-->")) {
                p++;
            }
            p += 3;
            continue;
        } else if (startswith(p, "<!DOCTYPE")) {
            p += 9;
            while (!startswith(p, ">")) {
                p++;
            }
            p++;
            continue;
        }

        // 特殊文字
        for (int c = 0; c < convert_count; c++) {
            if (startswith(p, convert_names[c])) {
                cur = new_token(PLAIN_TEXT, cur);
                strcpy(cur->text, convert_values[c]);
                p += strlen(convert_names[c]);
                continue;
            }
        }

        // 終了タグ
        if (startswith(p, "</")) {
            p += 2;
            cur = new_token(END_TAG, cur);
            consume_space(&p);
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    tag = c;
                    p += strlen(tag_names[c]);
                    tag_selected = true;
                    break;
                }
            }
            if (!tag_selected) {
                char buffer[200];
                int i = 0;
                while (*(p + i) != '>') {
                    buffer[i] = *(p + i);
                    i++;
                }
                buffer[i] = '\0';
                p += (i + 1);
                warning("終了タグを無視しました: %s\n", buffer);
                continue;
            }
            while (*p != '>') {
                p++;
            }
            p++;
            if (stack_pop() != tag) {
                error("開始タグと終了タグの対応が取れていません: %s\n", tag_names[tag]);
            }
            cur->tag = tag;
            continue;
        }

        // 単独タグ
        if (startswith(p, "<br")) {
            cur = new_token(START_TAG_ONLY, cur);
            cur->tag = TAG_BR;
            p += 3;
            while (*p != '>') {
                p++;
            }
            p++;
            continue;
        } else if (startswith(p, "<img")) {
            warning("imgタグは無視されます\n");
            cur = new_token(START_TAG_ONLY, cur);
            cur->tag = TAG_IMG;
            p += 4;
            while (*p != '>') {
                p++;
            }
            p++;
            continue;
        }

        // 開始タグ
        if (startswith(p, "<")) {
            p++;
            cur = new_token(START_TAG, cur);
            consume_space(&p);
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    tag = c;
                    p += strlen(tag_names[c]);
                    tag_selected = true;
                    break;
                }
            }
            if (!tag_selected) {
                char buffer[200];
                int i = 0;
                int j = 0;
                bool space2 = false;
                while (*(p + i) != '>') {
                    if (isspace(*(p + i))) {
                        space2 = true;
                    } else if (!space2) {
                        buffer[j] = *(p + i);
                        j++;
                    }
                    i++;
                }
                buffer[j] = '\0';
                p += (i + 1);
                warning("開始タグを無視しました: %s\n", buffer);
                continue;
            }
            consume_space(&p);
            if (startswith(p, "id=\"")) {
                p += 4;
                char buffer[20];
                int i = 0;
                while (*(p + i) != '\"') {
                    buffer[i] = *(p + i);
                    i++;
                }
                buffer[i] = '\0';
                p += (i + 1);
                strcpy(cur->html_id, buffer);
                warning("idは利用できません\n");
            }
            consume_space(&p);
            if (startswith(p, "class=\"")) {
                p += 7;
                char buffer[20];
                int i = 0;
                while (*(p + i) != '\"') {
                    buffer[i] = *(p + i);
                    i++;
                }
                buffer[i] = '\0';
                p += (i + 1);
                strcpy(cur->html_class, buffer);
                warning("classは利用できません\n");
            }
            consume_space(&p);
            if (startswith(p, "style=\"")) {
                p += 7;
                char buffer[40];
                int i = 0;
                while (*(p + i) != '\"') {
                    buffer[i] = *(p + i);
                    i++;
                }
                buffer[i] = '\0';
                p += (i + 1);
                css_property = parse_css(buffer);
                cur->css_property = css_property;
                // printf("styleを認識しました: %s\n", buffer);
            }
            consume_space(&p);
            while (*p != '>') {
                p++;
            }
            p++;
            stack_push(tag);
            cur->tag = tag;
            continue;
        }

        if ((*p != '<') && (*p != '>')) {
            char buffer[200];
            int i = 0;
            int length = 0;
            if (spaced) {
                buffer[length] = ' ';
                length++;
            }
            while ((*(p + i) != '<') && (*(p + i) != '>')) {
                int count = 0;
                while (isspace(*(p + i))) {
                    i++;
                    count++;
                }
                if (count > 0) {
                    buffer[length] = ' ';
                    length++;
                }
                if ((*(p + i) == '<') || (*(p + i) == '>')) {
                    break;
                }
                buffer[length] = *(p + i);
                length++;
                i++;
            }
            buffer[length] = '\0';
            cur = new_token(PLAIN_TEXT, cur);
            strcpy(cur->text, buffer);
            p += i;
            continue;
        } else {
            error("トークナイズできません: %s\n", *p);
        }
    }

    new_token(TK_EOF, cur);
    return head.next;
}

Token *parse_html(char *file_name) {
    FILE *fp;
    int chr;
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        error("%s file not open!\n", file_name);
    }
    int i = 0;
    while ((chr = fgetc(fp)) != EOF) {
        user_input[i] = chr;
        i++;
    }
    user_input[i] = '\0';
    fclose(fp);

    token = tokenize();
    return token;
}
