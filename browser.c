
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "parser.h"

SDL_Window *window;
SDL_Renderer *renderer;

TTF_Font *font_p;
TTF_Font *font_h1;
TTF_Font *font_h2;
TTF_Font *font_h3;

typedef struct {
    SDL_Texture *textTexture;
    int width;
    int height;
} WidgetCache;

WidgetCache caches[MAX_TAGS];

const int win_padding_x = 20;
const int win_padding_y = 20;
const int line_space = 10;
const int scroll_step = 20;
int window_height = 480;
int window_width = 720;

const int font_size_p = 16;
const int font_size_h1 = 48;
const int font_size_h2 = 32;
const int font_size_h3 = 20;

int scroll_width = 0;
int scroll_height = 0;

int scroll_offset_x = 0;
int scroll_offset_y = 0;

int changed = true;

void quit_sdl() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void draw_window(Token *token) {
    SDL_Surface *textSurface;
    SDL_Texture *textTexture;
    int width, height;
    int index = 0;
    int cor_x = 0;
    int cor_y = 0;
    int last_width = 0;
    int last_height = 0;
    int max_width = 0;
    bool new_line = true;
    bool is_title = false;
    char prefix[10] = "";
    char indent[10] = "";
    int font_style = TTF_STYLE_NORMAL;
    TTF_Font *font = font_p;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    while (token->kind != TK_EOF) {
        new_line = ((token->css_property->display == DISPLAY_BLOCK) || new_line);
        switch (token->kind) {
        case START_TAG:
            // printf("START_TAG: %s\n", tag_names[token->tag]);
            switch (token->tag) {
            case TAG_TITLE:
                is_title = true;
                break;
            case TAG_H1:
                last_height += 20;
                font = font_h1;
                break;
            case TAG_H2:
                last_height += 15;
                font = font_h2;
                break;
            case TAG_H3:
                last_height += 10;
                font = font_h3;
                break;
            case TAG_P:
                font = font_p;
                break;
            case TAG_LI:
                strcpy(prefix, " •  ");
                strcpy(indent, "    ");
                break;
            default:
                break;
            }
            break;
        case START_TAG_ONLY:
            // printf("START_TAG_ONLY: %s\n", tag_names[token->tag]);
            switch (token->tag) {
            case TAG_BR:
                cor_x = 0;
                cor_y += last_height + line_space;
                new_line = true;
                break;

            default:
                break;
            }
            break;
        case END_TAG:
            // printf("END_TAG: %s\n", tag_names[token->tag]);
            new_line = (token->css_property->display == DISPLAY_BLOCK);
            if ((token->tag == TAG_H1) || (token->tag == TAG_H2) || (token->tag == TAG_H3) || (token->tag == TAG_P)) {
                font = font_p;
            } else if (token->tag == TAG_TITLE) {
                is_title = false;
            } else if (token->tag == TAG_LI) {
                prefix[0] = '\0';
                indent[0] = '\0';
            }
            break;
        case PLAIN_TEXT:
            // printf("PLAIN_TEXT: \"%s\"\n", token->text);
            if (is_title) {
                SDL_SetWindowTitle(window, token->text);
                break;
            }
            index += 1;
            if (new_line) {
                cor_x = 0;
                cor_y += last_height;
                new_line = false;
            } else {
                cor_x += last_width;
            }
            if (prefix[0] != '\0') {
                TTF_SetFontStyle(font, TTF_STYLE_BOLD);
                textSurface = TTF_RenderUTF8_Blended(font, prefix, (SDL_Color){0, 0, 0});
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_QueryTexture(textTexture, NULL, NULL, &width, &height);
                SDL_Rect dstrect = {win_padding_x + cor_x - scroll_offset_x, win_padding_y + cor_y - scroll_offset_y, width, height};
                SDL_RenderCopy(renderer, textTexture, NULL, &dstrect);
                SDL_FreeSurface(textSurface);
                cor_x += width;
                prefix[0] = '\0';
            } else if (indent[0] != '\0') {
                textSurface = TTF_RenderUTF8_Blended(font, indent, (SDL_Color){0, 0, 0});
                textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_QueryTexture(textTexture, NULL, NULL, &width, &height);
                SDL_Rect dstrect = {win_padding_x + cor_x - scroll_offset_x, win_padding_y + cor_y - scroll_offset_y, width, height};
                SDL_RenderCopy(renderer, textTexture, NULL, &dstrect);
                SDL_FreeSurface(textSurface);
                cor_x += width;
            }
            if (!caches[index].textTexture && !changed) {
                textTexture = caches[index].textTexture;
                width = caches[index].width;
                height = caches[index].height;
                SDL_Rect dstrect = {win_padding_x + cor_x - scroll_offset_x, win_padding_y + cor_y - scroll_offset_y, width, height};
                SDL_RenderCopy(renderer, textTexture, NULL, &dstrect);
                last_width = width;
                last_height = height + line_space;
                break;
            }
            font_style = TTF_STYLE_NORMAL;
            if (token->css_property->font_weight == FONT_BOLD) {
                font_style |= TTF_STYLE_BOLD;
            }
            if (token->css_property->font_style == FONT_ITALIC) {
                font_style |= TTF_STYLE_ITALIC;
            }
            if (token->css_property->text_decoration == TEXT_UNDERLINE) {
                font_style |= TTF_STYLE_UNDERLINE;
            }
            TTF_SetFontStyle(font, font_style);
            textSurface = TTF_RenderUTF8_Blended(font, token->text, token->css_property->color);
            textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_QueryTexture(textTexture, NULL, NULL, &width, &height);
            SDL_Rect dstrect = {win_padding_x + cor_x - scroll_offset_x, win_padding_y + cor_y - scroll_offset_y, width, height};
            SDL_RenderCopy(renderer, textTexture, NULL, &dstrect);
            SDL_FreeSurface(textSurface);
            max_width = (cor_x + width) > max_width ? (cor_x + width) : max_width;
            last_width = width;
            last_height = height + line_space;
            caches[index] = (WidgetCache){textTexture, width, height};
            break;
        default:
            break;
        }
        token = token->next;
    }

    scroll_width = max_width + win_padding_x * 2;
    scroll_height = cor_y + last_height + win_padding_y * 2;

    SDL_RenderPresent(renderer);

    changed = false;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("引数の個数が正しくありません\n");
    }
    Token *token = parse_html(argv[1]);

    // SDLの初期化
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        error("SDL_Init Error: %s\n", SDL_GetError());
    }

    // SDL_ttfの初期化
    if (TTF_Init() == -1) {
        error("TTF_Init Error: %s\n", TTF_GetError());
    }

    // ウィンドウを作成
    window = SDL_CreateWindow("LSB", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    if (!window) {
        error("SDL_CreateWindow Error: %s\n", SDL_GetError());
    }

    // ウィンドウにアイコンを設定
    SDL_Surface *icon = SDL_LoadBMP("./icon.bmp");
    if (!icon) {
        warning("SDL_LoadBMP Error: %s\n", SDL_GetError());
    } else {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    // レンダラーを作成
    renderer = SDL_CreateRenderer(window, -2, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    if (!renderer) {
        error("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    }

    // フォントを開く
    char *font_path = "./RictyDiminished.ttf";
    font_p = TTF_OpenFont(font_path, font_size_p);
    font_h1 = TTF_OpenFont(font_path, font_size_h1);
    font_h2 = TTF_OpenFont(font_path, font_size_h2);
    font_h3 = TTF_OpenFont(font_path, font_size_h3);
    if (!font_p || !font_h1 || !font_h2 || !font_h3) {
        error("TTF_OpenFont Error: %s\n", TTF_GetError());
    }

    for (int i = 0; i < MAX_TAGS; i++) {
        caches[i] = (WidgetCache){NULL, 0, 0};
    }
    draw_window(token);

    bool running = true;
    bool changed = true;
    SDL_Event event;

    while (running) {
        SDL_PollEvent(&event);
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                running = false;
            } else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                window_width = event.window.data1;
                window_height = event.window.data2;

                if (scroll_offset_x <= 0 || scroll_width < window_width) {
                    scroll_offset_x = 0;
                } else if (scroll_offset_x > scroll_width - window_width) {
                    scroll_offset_x = scroll_width - window_width;
                }

                if (scroll_offset_y <= 0 || scroll_height < window_height) {
                    scroll_offset_y = 0;
                } else if (scroll_offset_y > scroll_height - window_height) {
                    scroll_offset_y = scroll_height - window_height;
                }

                // printf("window_width: %d, window_height: %d\n", window_width, window_height);
                SDL_RenderSetLogicalSize(renderer, window_width, window_height);
                changed = true;
            }
        }

        if (event.type == SDL_MOUSEWHEEL) {
            if (event.wheel.x < 0) {
                scroll_offset_x -= scroll_step;
            } else if (event.wheel.x > 0) {
                scroll_offset_x += scroll_step;
            }

            if (event.wheel.y < 0) {
                scroll_offset_y += scroll_step;
            } else if (event.wheel.y > 0) {
                scroll_offset_y -= scroll_step;
            }

            if (scroll_offset_x <= 0 || scroll_width < window_width) {
                scroll_offset_x = 0;
            } else if (scroll_offset_x > scroll_width - window_width) {
                scroll_offset_x = scroll_width - window_width;
            }

            if (scroll_offset_y <= 0 || scroll_height < window_height) {
                scroll_offset_y = 0;
            } else if (scroll_offset_y > scroll_height - window_height) {
                scroll_offset_y = scroll_height - window_height;
            }

            while (SDL_PollEvent(&event)) {
                continue;
            }

            changed = true;
        }

        if (changed) {
            draw_window(token);
            changed = false;
        }

        SDL_Delay(20);
    }

    TTF_CloseFont(font_p);
    TTF_CloseFont(font_h1);
    TTF_CloseFont(font_h2);
    TTF_CloseFont(font_h3);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
