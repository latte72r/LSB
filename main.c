
#include <ctype.h>
#include <gtk/gtk.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAGS 200

typedef enum { START_TAG, END_TAG, PLAIN_TEXT, TK_EOF } TokenKind;
typedef enum { TAG_HTML, TAG_BODY, TAG_DIV, TAG_SPAN, TAG_P, TAG_A, TAG_H1, TAG_H2, TAG_H3, TAG_UL, TAG_LI } TagKind;
char *tag_names[] = {"html", "body", "div", "span", "p", "a", "h1", "h2", "h3", "ul", "li"};
int tag_sizes[] = {4, 4, 3, 4, 1, 1, 2, 2, 2, 2, 2};
static const int supported_count = sizeof(tag_sizes) / sizeof(tag_sizes[0]);

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    TagKind tag;
    char text[200];
    char html_id[20];
    char html_class[20];
};

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

// Create a new token and add it as the next token of `cur`.
Token *new_token(TokenKind kind, Token *cur) {
    Token *tok = calloc(1, sizeof(Token));
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

void consume_space(char **p) {
    while (isspace(**p)) {
        (*p)++;
    }
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;
    TagKind tag;
    bool tag_selected;

    while (*p) {
        consume_space(&p);
        if (!*p) {
            break;
        }
        tag_selected = false;

        // 終了タグ
        if (startswith(p, "</")) {
            p += 2;
            cur = new_token(END_TAG, cur);
            consume_space(&p);
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    tag = c;
                    p += tag_sizes[c];
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

        // 開始タグ
        if (startswith(p, "<")) {
            p++;
            cur = new_token(START_TAG, cur);
            consume_space(&p);
            for (int c = 0; c < supported_count; c++) {
                if (startswith(p, tag_names[c])) {
                    tag = c;
                    p += tag_sizes[c];
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
                printf("idを認識しました: %s\n", buffer);
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
                printf("classを認識しました: %s\n", buffer);
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

            // 英数字の部分を抽出
            while ((*(p + i) != '<') && (*(p + i) != '>')) {
                buffer[i] = *(p + i);
                i++;
            }
            buffer[i] = '\0';
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

GtkWidget *box_widgets[100];

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません\n");
        return 1;
    }
    FILE *fp;
    int chr;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("%s file not open!\n", argv[1]);
        return 1;
    }
    int i = 0;
    while ((chr = fgetc(fp)) != EOF) {
        user_input[i] = chr;
        i++;
    }
    user_input[i] = '\0';
    fclose(fp);

    GtkWidget *window;
    int index = 0;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Hello");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./style.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    box_widgets[0] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), box_widgets[0]);
    gtk_widget_set_halign(box_widgets[0], GTK_ALIGN_START);

    token = tokenize();

    while (token->kind != TK_EOF) {
        switch (token->kind) {
        case START_TAG:
            // printf("START_TAG: %s\n", tag_names[token->tag]);
            int orientation;
            int expand;
            if ((token->tag == TAG_HTML) || (token->tag == TAG_BODY) || (token->tag == TAG_DIV) ||
                (token->tag == TAG_UL)) {
                orientation = GTK_ORIENTATION_VERTICAL;
            } else {
                orientation = GTK_ORIENTATION_HORIZONTAL;
            }
            box_widgets[index + 1] = gtk_box_new(orientation, 0);
            gtk_box_pack_start(GTK_BOX(box_widgets[index]), box_widgets[index + 1], FALSE, FALSE, 0);
            gtk_widget_set_margin_top(box_widgets[index + 1], 4);
            gtk_widget_set_margin_bottom(box_widgets[index + 1], 4);
            gtk_style_context_add_class(gtk_widget_get_style_context(box_widgets[index + 1]), tag_names[token->tag]);
            if (token->html_id[0] != '\0') {
                gtk_widget_set_name(box_widgets[index + 1], token->html_id);
            }
            if (token->html_class[0] != '\0') {
                gtk_style_context_add_class(gtk_widget_get_style_context(box_widgets[index + 1]), token->html_class);
            }
            if (token->tag == TAG_LI) {
                GtkWidget *label = gtk_label_new(" ・ ");
                gtk_box_pack_start(GTK_BOX(box_widgets[index + 1]), label, FALSE, FALSE, 0);
            }
            index++;
            break;
        case END_TAG:
            // printf("END_TAG: %s\n", tag_names[token->tag]);
            index--;
            break;
        case PLAIN_TEXT:
            // printf("PLAIN_TEXT: \"%s\"\n", token->text);
            GtkWidget *label = gtk_label_new(token->text);
            gtk_box_pack_start(GTK_BOX(box_widgets[index]), label, FALSE, FALSE, 0);
            break;
        }
        token = token->next;
    }

    // ウィンドウの破棄信号を接続
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
