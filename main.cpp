#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void null_seed() {
    srand(time(NULL));
}

SDL_Texture* load_texture(SDL_Renderer *renderer, const char *file_path) {
    SDL_Surface *surface = SDL_LoadBMP(file_path);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load BMP: %s", SDL_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture: %s", SDL_GetError());
        return NULL;
    }

    return texture;
}

class Barry {
    public:
        void move(float *fall, float *player_x, float *player_y, float player_speed, float deltaTime, SDL_FRect *player_rect, SDL_FRect *obstacle_rect, SDL_FRect *roof_rect, SDL_FRect *floor_rect, SDL_FRect *reverse_floor_rect, int *running) {
            const bool *keyboard = SDL_GetKeyboardState(NULL);
            bool on_floor = SDL_HasRectIntersectionFloat(player_rect, floor_rect) || SDL_HasRectIntersectionFloat(player_rect, reverse_floor_rect);
            bool on_roof = SDL_HasRectIntersectionFloat(player_rect, roof_rect);

            // Gravity applied when key is held (simulate Mono/Space or Dual/Up)
            if (keyboard[SDL_SCANCODE_UP]) {
                *player_y -= *fall;
                *fall += 0.75f;

                if (on_floor) {
                    *fall = 4.0f;
                }
                if (on_roof) {
                    *fall = 0.0f;
                    *player_y = roof_rect->y + roof_rect->h;  // avoid sticking
                }
            } else { // Gravity when not holding the key
                *fall -= 0.75f;
                *player_y -= *fall;

                if (on_floor) {
                    *fall = 0.0f;
                    *player_y = floor_rect->y - player_rect->h;
                } else {
                    // Could set `kind = "fall"` here if needed
                }

                if (on_roof) {
                    *player_y = roof_rect->y + roof_rect->h + 1;
                }
            }

            // Clamp fall speed
            if (*fall > 10.0f) *fall = 10.0f;
            if (*fall < -20.0f) *fall = -20.0f;

            // Update rect position
            player_rect->x = *player_x;
            player_rect->y = *player_y;

            // Collision with obstacle
            if (SDL_HasRectIntersectionFloat(player_rect, obstacle_rect)) {
                SDL_Log("Collision detected, exiting...");
                *running = 0;
            }
        }
};

class Missile {};

int main(int argc, char *argv[])
{
    null_seed();
    int screen_width = 1366;
    int screen_height = 768;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Texture *player_texture;
    SDL_Texture *obstacle_texture;
    SDL_Texture *background;
    SDL_Texture *reverse_background;
    SDL_Texture *floor;
    SDL_Texture *reverse_floor;
    SDL_Texture *roof;
    SDL_Event event;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (!SDL_CreateWindowAndRenderer("PyPack Joyride pre-alpha", screen_width, screen_height, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    SDL_Surface *icon = SDL_LoadBMP("icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load icon.bmp: %s", SDL_GetError());
    }

    player_texture = load_texture(renderer, "player.bmp");
    if (!player_texture) return 3;

    obstacle_texture = load_texture(renderer, "colision.bmp");
    if (!obstacle_texture) return 3;

    background = load_texture(renderer, "res/img/bg.bmp");
    if (!background) return 3;

    reverse_background = load_texture(renderer, "res/img/bg_rvrs.bmp");
    if (!reverse_background) return 3;

    SDL_FRect bg_rect = { 0, 0, 2740, 1000};

    SDL_FRect reverse_background_rect = { 2740, 0, 2740, 1000};

    floor = load_texture(renderer, "res/img/floor.bmp");
    if (!floor) return 3;

    reverse_floor = load_texture(renderer, "res/img/floor_rvrs.bmp");
    if (!reverse_floor) return 3;

    roof = load_texture(renderer, "res/img/roof.bmp");
    if (!roof) return 3;

    SDL_FRect roof_rect = { 0, -40, screen_width, 40 };

    SDL_FRect floor_rect = { 0.0f, screen_height-50, 2740, 50} ;

    SDL_FRect reverse_floor_rect = { 2740.0f, screen_height-50, 2740, 50 };



    // --- Define Player ---
    float player_x = 20.0f, player_y = 675.0f;
    float player_speed = 360.0f; // Frames Per Second
    float player_width = 64.0f, player_height = 74.0f;

    SDL_FRect player_rect = { player_x, player_y, player_width, player_height };

    // --- Define Obstacle ---
    SDL_FRect obstacle_rect = { 500.0f, 300.0f, 64.0f, 64.0f };

    int running = 1;
    Uint64 now = SDL_GetTicks(), last = now;
    float deltaTime = 0.0f;
    int player_frame = 0;
    float fall = 0;

    Barry barry;

    while (running) {
        barry.move(&fall, &player_x, &player_y, player_speed, deltaTime, &player_rect, &obstacle_rect, &roof_rect, &floor_rect, &reverse_floor_rect, &running);
        // Delta time
        now = SDL_GetTicks();
        deltaTime = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }
        if (player_frame == 40) {
            player_frame = 0;
        } else {
            player_frame++;
            if (10>=player_frame && player_frame>=0) {
                player_texture = load_texture(renderer, "res/img/Walk1.bmp");
            }
            else if (20>=player_frame && player_frame>=11) {
                player_texture = load_texture(renderer, "res/img/Walk2.bmp");
            }
            else if (30>=player_frame && player_frame>=21) {
                player_texture = load_texture(renderer, "res/img/Walk3.bmp");
            }
            else if (40>=player_frame && player_frame>=31) {
                player_texture = load_texture(renderer, "res/img/Walk4.bmp");
            }
        }

        floor_rect.x -= 20.0f;
        reverse_floor_rect.x -= 20.0f;
        bg_rect.x -= 20;
        reverse_background_rect.x -= 20;

        if (floor_rect.x<= -2740.0f) {
            floor_rect.x = 2740.0f;
        }

        if (reverse_floor_rect.x<= -2740.0f) {
            reverse_floor_rect.x = 2740.0f;
        }

        if (bg_rect.x <= -2740) {
            bg_rect.x = reverse_background_rect.x + 2740;
        }

        if (reverse_background_rect.x <= -2740) {
            reverse_background_rect.x = bg_rect.x + 2740;
        }

        floor_rect.x = floor_rect.x;
        reverse_floor_rect.x = reverse_floor_rect.x;

        const bool *keyboard = SDL_GetKeyboardState(NULL);


        // Render
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, background, NULL, &bg_rect);
        SDL_RenderTexture(renderer, reverse_background, NULL, &reverse_background_rect);
        SDL_RenderTexture(renderer, obstacle_texture, NULL, &obstacle_rect);
        SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
        SDL_RenderTexture(renderer, roof, NULL, &roof_rect);
        SDL_RenderTexture(renderer, floor, NULL, &floor_rect);
        SDL_RenderTexture(renderer, reverse_floor, NULL, &reverse_floor_rect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(background);
    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(obstacle_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
