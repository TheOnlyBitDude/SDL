#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void null_seed() {
    srand(time(NULL));
}

void shoot_bullet(SDL_Texture **bullet_texture, SDL_FRect *bullet_rect, bool *bullet_active, SDL_Renderer *renderer, SDL_FRect *player_rect) {
    if (!(*bullet_active)) {
        SDL_Surface *bullet_surface = SDL_LoadBMP("bullet.BMP");
        if (!bullet_surface) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load bullet.BMP: %s", SDL_GetError());
            return;
        }

        *bullet_texture = SDL_CreateTextureFromSurface(renderer, bullet_surface);
        SDL_DestroySurface(bullet_surface);

        if (!(*bullet_texture)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create bullet texture: %s", SDL_GetError());
            return;
        }

        bullet_rect->w = 16.0f;
        bullet_rect->h = 16.0f;
        bullet_rect->x = player_rect->x + 24.0f + (rand() % 26 - 15);
        bullet_rect->y = player_rect->y + player_rect->h / 2.0f - bullet_rect->h / 2.0f;

        *bullet_active = true;
    } else {
        bullet_rect->y += 3.0f;
    }
}

void update_player(float *player_x, float *player_y, float player_speed, float deltaTime, SDL_FRect *player_rect, SDL_FRect *obstacle_rect, int *running, SDL_Renderer *renderer, SDL_Texture **bullet_texture, SDL_FRect *bullet_rect, bool *bullet_active) {
    const bool *keyboard = SDL_GetKeyboardState(NULL);
    if (keyboard[SDL_SCANCODE_UP]) *player_y -= player_speed * deltaTime;
    if (keyboard[SDL_SCANCODE_DOWN]) *player_y += player_speed * deltaTime;
    if (keyboard[SDL_SCANCODE_LEFT]) *player_x -= player_speed * deltaTime;
    if (keyboard[SDL_SCANCODE_RIGHT]) *player_x += player_speed * deltaTime;
    if (keyboard[SDL_SCANCODE_SPACE]) shoot_bullet(bullet_texture, bullet_rect, bullet_active, renderer, player_rect);

    player_rect->x = *player_x;
    player_rect->y = *player_y;

    if (SDL_HasRectIntersectionFloat(player_rect, obstacle_rect)) {
        SDL_Log("Collision detected, exiting...");
        *running = 0;
    }
}

int main(int argc, char *argv[]) {
    null_seed();
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Texture *player_texture;
    SDL_Texture *obstacle_texture;
    SDL_Texture *bullet_texture = NULL;
    SDL_Event event;

    SDL_FRect bullet_rect;
    bool bullet_active = false;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (!SDL_CreateWindowAndRenderer("PyPack Joyride pre-alpha", 1366, 768, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    SDL_Surface *icon = SDL_LoadBMP("icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load icon.bmp: %s", SDL_GetError());
    }

    surface = SDL_LoadBMP("sample.bmp");
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load sample.bmp: %s", SDL_GetError());
        return 3;
    }
    player_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!player_texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create player texture: %s", SDL_GetError());
        return 3;
    }

    surface = SDL_LoadBMP("colision.bmp");
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load colision.bmp: %s", SDL_GetError());
        return 3;
    }
    obstacle_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!obstacle_texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create obstacle texture: %s", SDL_GetError());
        return 3;
    }

    float player_x = 100.0f, player_y = 100.0f;
    float player_speed = 360.0f;
    float player_width = 64.0f, player_height = 64.0f;
    SDL_FRect player_rect = { player_x, player_y, player_width, player_height };
    SDL_FRect obstacle_rect = { 500.0f, 300.0f, 64.0f, 64.0f };

    int running = 1;
    Uint64 now = SDL_GetTicks(), last = now;
    float deltaTime = 0.0f;

    while (running) {
        now = SDL_GetTicks();
        deltaTime = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        update_player(&player_x, &player_y, player_speed, deltaTime, &player_rect, &obstacle_rect, &running, renderer, &bullet_texture, &bullet_rect, &bullet_active);

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, obstacle_texture, NULL, &obstacle_rect);
        SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
        if (bullet_active) {
            SDL_RenderTexture(renderer, bullet_texture, NULL, &bullet_rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(obstacle_texture);
    if (bullet_texture) SDL_DestroyTexture(bullet_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
