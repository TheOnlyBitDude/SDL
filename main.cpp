#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

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

class Bullet {
public:
    SDL_FRect rect;
    SDL_Texture* texture;
    bool init_x;

    Bullet(SDL_Texture* tex, float start_x, float start_y)
        : texture(tex), init_x(true)
    {
        rect = { start_x, start_y, 10.0f, 45.0f };
    }

    void shoot(float barry_x) {
        if (init_x) {
            rect.x = barry_x + 24 + (rand() % 26 - 15);
            init_x = false;
        } else {
            rect.x -= 3.0f;
            rect.y += 35.0f;
        }
    }

    void render(SDL_Renderer* renderer) {
        SDL_RenderTexture(renderer, texture, NULL, &rect);
    }
};

class Barry {
public:
    void move(float *fall, float *player_x, float *player_y, float player_speed, float deltaTime, SDL_FRect *player_rect, SDL_FRect *obstacle_rect, SDL_FRect *roof_rect, SDL_FRect *floor_rect, SDL_FRect *reverse_floor_rect, int *running) {
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        bool on_floor = SDL_HasRectIntersectionFloat(player_rect, floor_rect) || SDL_HasRectIntersectionFloat(player_rect, reverse_floor_rect);
        bool on_roof = SDL_HasRectIntersectionFloat(player_rect, roof_rect);

        if (keyboard[SDL_SCANCODE_SPACE]) {
            *player_y -= *fall;
            *fall += 0.75f;

            if (on_floor) *fall = 4.0f;
            if (on_roof) {
                *fall = 0.0f;
                *player_y = roof_rect->y + roof_rect->h;
            }
        } else {
            *fall -= 0.75f;
            *player_y -= *fall;

            if (on_floor) {
                *fall = 0.0f;
                *player_y = floor_rect->y - player_rect->h;
            }
            if (on_roof) {
                *player_y = roof_rect->y + roof_rect->h + 1;
            }
        }

        if (*fall > 10.0f) *fall = 10.0f;
        if (*fall < -20.0f) *fall = -20.0f;

        player_rect->x = *player_x;
        player_rect->y = *player_y;

        if (SDL_HasRectIntersectionFloat(player_rect, obstacle_rect)) {
            SDL_Log("Collision detected, exiting...");
            *running = 0;
        }
    }

    void animate(SDL_Renderer *renderer, SDL_Texture **player_texture, int player_frame) {
        if (*player_texture) {
            SDL_DestroyTexture(*player_texture);
            *player_texture = NULL;
        }

        if (player_frame <= 10) {
            *player_texture = load_texture(renderer, "res/img/Walk1.bmp");
        } else if (player_frame <= 20) {
            *player_texture = load_texture(renderer, "res/img/Walk2.bmp");
        } else if (player_frame <= 30) {
            *player_texture = load_texture(renderer, "res/img/Walk3.bmp");
        } else if (player_frame <= 40) {
            *player_texture = load_texture(renderer, "res/img/Walk4.bmp");
        }
    }

    void handle_input(std::vector<Bullet>& bullets, SDL_Texture* bullet_texture, const SDL_FRect& player_rect) {
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        if (keyboard[SDL_SCANCODE_SPACE]) {
            bullets.emplace_back(bullet_texture, player_rect.x, player_rect.y+player_rect.h);
            bullets.emplace_back(bullet_texture, player_rect.x, player_rect.y+player_rect.h);
        }
    }
};

void update_background_scroll(SDL_FRect *floor_rect, SDL_FRect *reverse_floor_rect, SDL_FRect *bg_rect, SDL_FRect *reverse_bg_rect) {
    floor_rect->x -= 20.0f;
    reverse_floor_rect->x -= 20.0f;
    bg_rect->x -= 20.0f;
    reverse_bg_rect->x -= 20.0f;

    if (floor_rect->x <= -2740.0f) floor_rect->x = 2740.0f;
    if (reverse_floor_rect->x <= -2740.0f) reverse_floor_rect->x = 2740.0f;
    if (bg_rect->x <= -2740.0f) bg_rect->x = reverse_bg_rect->x + 2740.0f;
    if (reverse_bg_rect->x <= -2740.0f) reverse_bg_rect->x = bg_rect->x + 2740.0f;
}

int main(int argc, char *argv[]) {
    null_seed();
    int screen_width = 1366, screen_height = 768;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    if (!SDL_Init(SDL_INIT_VIDEO)) return 3;
    if (!SDL_CreateWindowAndRenderer("PyPack Joyride pre-alpha", screen_width, screen_height, SDL_WINDOW_RESIZABLE, &window, &renderer)) return 3;

    SDL_Texture *player_texture = load_texture(renderer, "player.bmp");
    SDL_Texture *obstacle_texture = load_texture(renderer, "colision.bmp");
    SDL_Texture *background = load_texture(renderer, "res/img/bg.bmp");
    SDL_Texture *reverse_background = load_texture(renderer, "res/img/bg_rvrs.bmp");
    SDL_Texture *floor = load_texture(renderer, "res/img/floor.bmp");
    SDL_Texture *reverse_floor = load_texture(renderer, "res/img/floor_rvrs.bmp");
    SDL_Texture *roof = load_texture(renderer, "res/img/roof.bmp");
    SDL_Texture *bullet_texture = load_texture(renderer, "bullet.bmp");

    SDL_FRect bg_rect = { 0, 0, 2740, 1000 }, reverse_bg_rect = { 2740, 0, 2740, 1000 };
    SDL_FRect roof_rect = { 0.0f, -40.0f, static_cast<float>(screen_width), 40.0f };
    SDL_FRect floor_rect = { 0.0f, static_cast<float>(screen_height - 50), 2740.0f, 50.0f };
    SDL_FRect reverse_floor_rect = { 2740.0f, static_cast<float>(screen_height - 50), 2740.0f, 50.0f };
    SDL_FRect player_rect = { 20.0f, 675.0f, 64.0f, 74.0f };
    SDL_FRect obstacle_rect = { 500.0f, 300.0f, 64.0f, 64.0f };

    float player_x = 20.0f, player_y = 675.0f, player_speed = 360.0f, fall = 0.0f;
    int running = 1, player_frame = 0;
    float deltaTime = 0.0f;
    Uint64 now = SDL_GetTicks(), last = now;

    Barry barry;
    std::vector<Bullet> bullets;

    while (running) {
        barry.move(&fall, &player_x, &player_y, player_speed, deltaTime, &player_rect, &obstacle_rect, &roof_rect, &floor_rect, &reverse_floor_rect, &running);

        now = SDL_GetTicks();
        deltaTime = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = 0;
        }

        barry.handle_input(bullets, bullet_texture, player_rect);

        for (auto& bullet : bullets) {
            bullet.shoot(player_rect.x);
        }

        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
            return b.rect.x < -20.0f;
        }), bullets.end());

        if (++player_frame > 40) player_frame = 0;
        barry.animate(renderer, &player_texture, player_frame);
        update_background_scroll(&floor_rect, &reverse_floor_rect, &bg_rect, &reverse_bg_rect);

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, background, NULL, &bg_rect);
        SDL_RenderTexture(renderer, reverse_background, NULL, &reverse_bg_rect);
        SDL_RenderTexture(renderer, obstacle_texture, NULL, &obstacle_rect);
        SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
        SDL_RenderTexture(renderer, roof, NULL, &roof_rect);
        SDL_RenderTexture(renderer, floor, NULL, &floor_rect);
        SDL_RenderTexture(renderer, reverse_floor, NULL, &reverse_floor_rect);

        for (auto& bullet : bullets) {
            bullet.render(renderer);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(background);
    SDL_DestroyTexture(reverse_background);
    SDL_DestroyTexture(floor);
    SDL_DestroyTexture(reverse_floor);
    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(obstacle_texture);
    SDL_DestroyTexture(roof);
    SDL_DestroyTexture(bullet_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}