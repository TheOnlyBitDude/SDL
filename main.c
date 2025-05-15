#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void null_seed() {
    srand(time(NULL));
}

int main(int argc, char *argv[])
{
    null_seed();
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Texture *player_texture;
    SDL_Texture *obstacle_texture;
    SDL_Event event;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (!SDL_CreateWindowAndRenderer("Colisión SDL", 1366, 768, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    // --- Load Player ---
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

    // --- Load Obstacle ---
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

    // --- Define Player ---
    float player_x = 100.0f, player_y = 100.0f;
    float player_speed = 360.0f; // Frames Per Second
    float player_width = 64.0f, player_height = 64.0f;

    SDL_FRect player_rect = { player_x, player_y, player_width, player_height };

    // --- Define Obstacle ---
    SDL_FRect obstacle_rect = { 500.0f, 300.0f, 64.0f, 64.0f };

    int running = 1;
    Uint64 now = SDL_GetTicks(), last = now;
    float deltaTime = 0.0f;

    while (running) {
        // Delta time
        now = SDL_GetTicks();
        deltaTime = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        // Player Movement
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        if (keyboard[SDL_SCANCODE_UP]) player_y -= player_speed * deltaTime;
        if (keyboard[SDL_SCANCODE_DOWN]) player_y += player_speed * deltaTime;
        if (keyboard[SDL_SCANCODE_LEFT]) player_x -= player_speed * deltaTime;
        if (keyboard[SDL_SCANCODE_RIGHT]) player_x += player_speed * deltaTime;

        // Update player Rect
        player_rect.x = player_x;
        player_rect.y = player_y;

        // Detect Collision
        if (SDL_HasRectIntersectionFloat(&player_rect, &obstacle_rect)) {
            SDL_Log("Collision detected, exiting...");
            running = 0;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, obstacle_texture, NULL, &obstacle_rect);
        SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(obstacle_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
