
#include "parser.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char user_input[80000];
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

Token *new_token(TokenKind kind, Token *cur, Token *parent) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->parent = parent;
    CssProperty *css_property = calloc(1, sizeof(CssProperty));
    css_property->color = (SDL_Color){0, 0, 0};
    css_property->font_size = 100;
    css_property->font_weight = FONT_NORMAL;
    css_property->font_style = FONT_NORMAL;
    css_property->text_decoration = TEXT_NONE;
    css_property->display = DISPLAY_BLOCK;
    tok->css_property = css_property;
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

void parse_color(char color_string[7], SDL_Color *color) {
    char buffer[3];
    buffer[2] = '\0';
    if (startswith(color_string, "#")) {
        buffer[0] = color_string[1];
        buffer[1] = color_string[2];
        color->r = strtol(buffer, NULL, 16);
        buffer[0] = color_string[3];
        buffer[1] = color_string[4];
        color->g = strtol(buffer, NULL, 16);
        buffer[0] = color_string[5];
        buffer[1] = color_string[6];
        color->b = strtol(buffer, NULL, 16);
    } else {
        error("colorのパースに失敗しました: %s\n", color_string);
    }
    // printf("color: (%d, %d, %d)\n", color->r, color->g, color->b);
}

CssProperty *parse_css(char *css_style, CssProperty *css_property) {
    char buffer[200];
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
            parse_color(buffer, &(css_property->color));
        } else if (startswith(css_style, "font-size:")) {
            css_style += 10;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            if (buffer[i - 1] != '%') {
                buffer[i] = '\0';
                error("font-sizeのパースに失敗しました: %s\n", buffer);
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
            if (startswith(buffer, "normal")) {
                css_property->font_weight = FONT_NORMAL;
            } else if (startswith(buffer, "bold")) {
                css_property->font_weight = FONT_BOLD;
            } else {
                error("font-weightのパースに失敗しました: %s\n", buffer);
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
            if (startswith(buffer, "normal")) {
                css_property->font_style = FONT_NORMAL;
            } else if (startswith(buffer, "italic")) {
                css_property->font_style = FONT_ITALIC;
            } else {
                error("font-styleのパースに失敗しました: %s\n", buffer);
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
            if (startswith(buffer, "none")) {
                css_property->text_decoration = TEXT_NONE;
            } else if (startswith(buffer, "underline")) {
                css_property->text_decoration = TEXT_UNDERLINE;
            } else {
                error("text-decorationのパースに失敗しました: %s\n", buffer);
            }
        } else if (startswith(css_style, "display:")) {
            css_style += 8;
            consume_space(&css_style);
            int i = 0;
            while (*css_style != ';') {
                buffer[i] = *css_style;
                i++;
                css_style++;
            }
            css_style++;
            buffer[i] = '\0';
            if (startswith(buffer, "none")) {
                css_property->display = DISPLAY_NONE;
            } else if (startswith(buffer, "block")) {
                css_property->display = DISPLAY_BLOCK;
            } else if (startswith(buffer, "inline")) {
                css_property->display = DISPLAY_INLINE;
            } else {
                error("displayのパースに失敗しました: %s\n", buffer);
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
    return css_property;
}

void consume_style(Token *cur, char **p) {
    char buffer[200];
    consume_space(p);
    if (startswith(*p, "id=\"")) {
        (*p) += 4;
        int i = 0;
        int n = 0;
        while (*(*p + i) != '\"') {
            buffer[n] = *(*p + i);
            i++;
            n++;
        }
        buffer[n] = '\0';
        (*p) += (i + 1);
        strcpy(cur->html_id, buffer);
        warning("idは利用できません\n");
    }
    consume_space(p);
    if (startswith(*p, "class=\"")) {
        *p += 7;
        int i = 0;
        int n = 0;
        while (*(*p + i) != '\"') {
            buffer[n] = *(*p + i);
            i++;
            n++;
        }
        buffer[n] = '\0';
        (*p) += (i + 1);
        strcpy(cur->html_class, buffer);
        warning("classは利用できません\n");
    }
    consume_space(p);
    if (startswith(*p, "style=\"")) {
        (*p) += 7;
        int i = 0;
        int n = 0;
        while (*(*p + i) != '\"') {
            buffer[n] = *(*p + i);
            i++;
            n++;
        }
        buffer[n] = '\0';
        (*p) += (i + 1);
        parse_css(buffer, cur->css_property);
        // printf("styleを認識しました: %s\n", buffer);
    }
    consume_space(p);
    while (**p != '>') {
        (*p)++;
    }
    (*p)++;
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    head.css_property = calloc(1, sizeof(CssProperty));
    head.css_property->color = (SDL_Color){0, 0, 0};
    head.css_property->font_size = 100;
    head.css_property->font_weight = FONT_NORMAL;
    head.css_property->font_style = FONT_NORMAL;
    head.css_property->text_decoration = TEXT_NONE;
    head.css_property->display = DISPLAY_BLOCK;
    Token *cur = &head;
    Token *parent = &head;
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
                cur = new_token(PLAIN_TEXT, cur, parent);
                strcpy(cur->text, convert_values[c]);
                p += strlen(convert_names[c]);
                continue;
            }
        }

        // 終了タグ
        if (startswith(p, "</")) {
            p += 2;
            consume_space(&p);
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    if (*(p + strlen(tag_names[c])) != '>') {
                        continue;
                    }
                    tag = c;
                    p += strlen(tag_names[c]);
                    tag_selected = true;
                    // printf("終了タグを登録しました: %s\n", tag_names[tag]);
                    break;
                }
            }
            if (!tag_selected) {
                char buffer[800];
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
            css_property = parent->css_property;
            parent = parent->parent;
            cur = new_token(END_TAG, cur, parent);
            cur->tag = tag;
            cur->css_property = css_property;
            continue;
        }

        // 単独タグ
        if (startswith(p, "<br")) {
            cur = new_token(START_TAG_ONLY, cur, parent);
            cur->tag = TAG_BR;
            p += 3;
            while (*p != '>') {
                p++;
            }
            p++;
            continue;
        } else if (startswith(p, "<img")) {
            warning("imgタグは無視されます\n");
            cur = new_token(START_TAG_ONLY, cur, parent);
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
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    if (!isspace(*(p + strlen(tag_names[c]))) && *(p + strlen(tag_names[c])) != '>') {
                        continue;
                    }
                    tag = c;
                    tag_selected = true;
                    break;
                }
            }
            if (!tag_selected) {
                char buffer[800];
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
            } else if (startswith(p, "div")) {
                p += 3;
                consume_space(&p);
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_DIV;
                stack_push(TAG_DIV);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "span")) {
                p += 4;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_SPAN;
                stack_push(TAG_SPAN);
                cur->css_property->display = DISPLAY_INLINE;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "h1")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_H1;
                stack_push(TAG_H1);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->font_weight = FONT_BOLD;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "h2")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_H2;
                stack_push(TAG_H2);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->font_weight = FONT_BOLD;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "h3")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_H3;
                stack_push(TAG_H3);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->font_weight = FONT_BOLD;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "ul")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_UL;
                stack_push(TAG_UL);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "li")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_LI;
                stack_push(TAG_LI);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "em")) {
                p += 2;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_EM;
                stack_push(TAG_EM);
                cur->css_property->display = DISPLAY_INLINE;
                cur->css_property->font_style = FONT_ITALIC;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "strong")) {
                p += 6;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_STRONG;
                stack_push(TAG_STRONG);
                cur->css_property->display = DISPLAY_INLINE;
                cur->css_property->font_weight = FONT_BOLD;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "title")) {
                p += 5;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_TITLE;
                stack_push(TAG_TITLE);
                cur->css_property->display = DISPLAY_NONE;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "section")) {
                p += 7;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_SECTION;
                stack_push(TAG_SECTION);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "script")) {
                p += 6;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_SCRIPT;
                stack_push(TAG_SCRIPT);
                cur->css_property->display = DISPLAY_NONE;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "pre")) {
                p += 3;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_PRE;
                stack_push(TAG_PRE);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "p")) {
                p += 1;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_P;
                stack_push(TAG_P);
                cur->css_property->display = DISPLAY_BLOCK;
                cur->css_property->color = cur->parent->css_property->color;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
                consume_style(cur, &p);
                parent = cur;
            } else if (startswith(p, "a")) {
                p += 1;
                cur = new_token(START_TAG, cur, parent);
                cur->tag = TAG_A;
                stack_push(TAG_A);
                cur->css_property->display = DISPLAY_INLINE;
                cur->css_property->color = (SDL_Color){0, 0, 255};
                cur->css_property->text_decoration = TEXT_UNDERLINE;
                cur->css_property->font_size = cur->parent->css_property->font_size;
                cur->css_property->font_weight = cur->parent->css_property->font_weight;
                cur->css_property->font_style = cur->parent->css_property->font_style;
                consume_style(cur, &p);
                parent = cur;
            } else {
                error("開始タグを認識できません: %s\n", p);
            }
            // printf("開始タグを登録しました: %s\n", tag_names[cur->tag]);
            continue;
        }

        // プレーンテキスト
        if ((*p != '<') && (*p != '>')) {
            if (parent->tag == TAG_SCRIPT) {
                while (!startswith(p, "</script>")) {
                    p++;
                }
                continue;
            }
            char buffer[4000];
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
            cur = new_token(PLAIN_TEXT, cur, parent);
            strcpy(cur->text, buffer);
            p += i;
            cur->css_property->display = DISPLAY_INLINE;
            cur->css_property->color = cur->parent->css_property->color;
            cur->css_property->font_size = cur->parent->css_property->font_size;
            cur->css_property->font_weight = cur->parent->css_property->font_weight;
            cur->css_property->font_style = cur->parent->css_property->font_style;
            cur->css_property->text_decoration = cur->parent->css_property->text_decoration;
            continue;
        } else {
            error("トークナイズできません: %s\n", *p);
        }
    }

    new_token(TK_EOF, cur, parent);
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
