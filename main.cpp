#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>

void null_seed() {
    srand(time(NULL));
}


SDL_Texture* load_texture(SDL_Renderer *renderer, const char *file_path) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, file_path);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load image '%s': %s", file_path, SDL_GetError());
        return NULL;
    }
    return texture;
}

class TextLabel {
public:
    SDL_Texture* texture;
    SDL_FRect rect;
    SDL_Color color;
    TTF_Font* font;
    SDL_Renderer* renderer;

    TextLabel(SDL_Renderer* rend)
        : texture(nullptr), font(nullptr), renderer(rend) {}

    ~TextLabel() {
        if (texture) SDL_DestroyTexture(texture);
    }

    bool setText(const std::string& text, TTF_Font* font, SDL_Color color, float x, float y) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        this->font = font;
        this->color = color;

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
        if (!surface) {
            SDL_Log("Error al renderizar texto: %s", SDL_GetError());
            return false;
        }

        texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_Log("Error al crear textura desde superficie: %s", SDL_GetError());
            SDL_DestroySurface(surface);
            return false;
        }

        rect = { x, y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
        SDL_DestroySurface(surface);
        return true;
    }

    void render() const {
        if (texture) {
            SDL_RenderTexture(renderer, texture, nullptr, &rect);
        }
    }
};

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
    std::string animation = "Run";
    void move(float *fall, float *player_x, float *player_y, float player_speed, float deltaTime, SDL_FRect *player_rect, SDL_FRect *obstacle_rect, SDL_FRect *roof_rect, SDL_FRect *floor_rect, SDL_FRect *reverse_floor_rect, int *running) {
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        bool on_floor = SDL_HasRectIntersectionFloat(player_rect, floor_rect) || SDL_HasRectIntersectionFloat(player_rect, reverse_floor_rect);
        bool on_roof = SDL_HasRectIntersectionFloat(player_rect, roof_rect);

        if (keyboard[SDL_SCANCODE_SPACE]) {
            animation = "Fly";
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
                animation = "Run";
                *fall = 0.0f;
                *player_y = floor_rect->y - player_rect->h;
            } else {
                animation = "Fall";
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
        
        if (animation == "Run") {
            if (player_frame <= 10) {
                *player_texture = load_texture(renderer, "res/img/Walk1.png");
            } else if (player_frame <= 20) {
                *player_texture = load_texture(renderer, "res/img/Walk2.png");
            } else if (player_frame <= 30) {
                *player_texture = load_texture(renderer, "res/img/Walk3.png");
            } else if (player_frame <= 40) {
                *player_texture = load_texture(renderer, "res/img/Walk4.png");
            }
        } else if (animation == "Fly") {
            if (player_frame <= 5) {
                *player_texture = load_texture(renderer, "res/img/Fly1.png");
            } else if (player_frame <= 10) {
                *player_texture = load_texture(renderer, "res/img/Fly2.png");
            } else if (player_frame <= 15) {
                *player_texture = load_texture(renderer, "res/img/Fly3.png");
            } else if (player_frame <= 20) {
                *player_texture = load_texture(renderer, "res/img/FlyFall.png");
            } else if (player_frame <= 25) {
                *player_texture = load_texture(renderer, "res/img/Fly1.png");
            } else if (player_frame <= 30) {
                *player_texture = load_texture(renderer, "res/img/Fly2.png");
            } else if (player_frame <= 35) {
                *player_texture = load_texture(renderer, "res/img/Fly3.png");
            } else if (player_frame <= 40) {
                *player_texture = load_texture(renderer, "res/img/FlyFall.png");
            }
        } else {
            *player_texture = load_texture(renderer, "res/img/FlyFall.png");
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

class Missile {
public:
    SDL_FRect rect;
    SDL_Texture* texture;
    SDL_Renderer* renderer;

    int duration;
    int speed;
    int counter;
    int i;
    int wait;
    bool pre_launch;
    bool launched;
    bool f;
    float pos;
    float w, h;
    float fall = 0;
    std::string type;
    std::string orientation = "Down";

    Missile(SDL_Renderer* rend, float x, float y, float width, float height, int spd, int dur, std::string type)
        : renderer(rend), w(width), h(height), speed(spd), duration(dur),
          counter(0), i(0), wait(0), pre_launch(false), launched(false), f(false),
          type(type)
    {
        rect = { x, y, width, height };
        texture = load_texture(renderer, "res/img/Rocket1.png");
    }


    void animate() {
        if (counter >= 0 && counter < 2)
            texture = load_texture(renderer, "res/img/Rocket1.png");
        else if (counter >= 2 && counter < 4)
            texture = load_texture(renderer, "res/img/Rocket2.png");
        else if (counter >= 4 && counter < 6)
            texture = load_texture(renderer, "res/img/Rocket3.png");
        else if (counter >= 6 && counter < 8)
            texture = load_texture(renderer, "res/img/Rocket4.png");

        counter++;
        if (counter >= 8)
            counter = 0;
    }


    void warning() {
        if (!pre_launch) {
            pos = static_cast<float>(20 + rand() % (668 - 20));
            rect.y = pos;
            launched = false;

            // Warning
        }
        launch();
    }

    void launch() {
        if (!pre_launch) {
            i = 0;
            rect.x = 1324.0f;
            pre_launch = true;
            wait = 0;
            f = false;
        }

        if (wait == 34) {
            // Launch
        }

        if (wait == 35) {
            if (i != duration) {
                if (type == "wave"){
                    if (orientation == "Down") fall -= 0.75;
                    else if (orientation == "Up") fall += 0.75;
                    if (fall >= 10) orientation = "Down";
                    else if (fall <= -10) orientation = "Up";
                    pos += static_cast<float>(fall);
                    rect.x -= static_cast<float>(speed);
                    i++;
                }
                else {
                    pos += static_cast<float>(fall);
                    rect.x -= static_cast<float>(speed);
                    i++;
                }
            } else {
                i = 0;
                wait = 0;
                pre_launch = false;
                launched = false;
            }

            if (!f)
                f = true;

            rect.y = pos;
            animate();
        } else {
            wait++;
        }
    }

    void update() {
        warning();
    }

    void render() {
        if (texture)
            SDL_RenderTexture(renderer, texture, NULL, &rect);
    }

    bool collides_with(const SDL_FRect& player_rect) {
        return SDL_HasRectIntersectionFloat(&rect, &player_rect);
    }
};


int main(int argc, char *argv[]) {

    for (int i = 1; i < argc; ++i) {  // start at 1 to skip the program name
        std::string arg = argv[i];
        if (arg == "--arg1") {
            int temporal_variable;
        }
    }

    null_seed();
    int screen_width = 1366, screen_height = 768;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};

    if (!SDL_Init(SDL_INIT_VIDEO)) return 3;
    if (!SDL_CreateWindowAndRenderer("PyPack Joyride beta", screen_width, screen_height, SDL_WINDOW_RESIZABLE, &window, &renderer)) return 3;
    if (!TTF_Init()) return 3;

    TTF_Font* font = TTF_OpenFont("res/fnt/ModernDOS9x16.ttf", 24);

    SDL_Texture *player_texture = load_texture(renderer, "res/img/Walk1.png");
    SDL_Texture *background = load_texture(renderer, "res/img/bg.jpg");
    SDL_Texture *reverse_background = load_texture(renderer, "res/img/bg_rvrs.jpg");
    SDL_Texture *floor = load_texture(renderer, "res/img/floor.png");
    SDL_Texture *reverse_floor = load_texture(renderer, "res/img/floor_rvrs.png");
    SDL_Texture *roof = load_texture(renderer, "res/img/roof.png");
    SDL_Texture *bullet_texture = load_texture(renderer, "res/img/bullet.png");

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

    // Crear misiles
    std::vector<Missile> missiles;
    for (int i = 0; i < 7; ++i) {
        std::string type = (i >= 4) ? "wave" : "normal";
        missiles.emplace_back(renderer, 0, 2147483647, 93.0f, 34.0f, 50, 52, type);
    }

    TextLabel title(renderer);
    TextLabel Lost(renderer);
    title.setText("PyPack_Joyride!", font, white, static_cast<float>((screen_width / 2.5f) - title.rect.w), 50.0f);
    Lost.setText("You lost! Press anywhere to continue...", font, white, static_cast<float>((screen_width - Lost.rect.w) / 2.0f), 50.0f);


    std::string stage = "Title";
    SDL_FRect window_rect {0.0f, 0.0f, static_cast<float>(screen_width), static_cast<float>(screen_height)};


    while (running) {


        // Ticks y eventos

        now = SDL_GetTicks();
        deltaTime = (now - last) / 1000.0f;
        last = now;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = 0;
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_FPoint mouse_point = {
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y)
                    };

                    
                    // Detección de click

                    if (SDL_PointInRectFloat(&mouse_point, &window_rect)) stage = "Game";
                    if (SDL_PointInRectFloat(&mouse_point, &player_rect)) {
                        SDL_Log("Haz hecho clic sobre el jugadór.");
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        if (stage == "Title"){
            title.render();
        }
        if (stage == "Lost"){
            Lost.render();
        }
        if (stage == "Game") {

            // Misiles

            int lnch = rand() % 300 + 1;
            if ((lnch == 126 || lnch == 173 || lnch == 111)) {
                missiles[0].update();
                missiles[0].launched = true;
            }
            else if ((lnch == 222 || lnch == 109 || lnch == 63)) {
                missiles[1].update();
                missiles[1].launched = true;
            }
            else if ((lnch == 35 || lnch == 17 || lnch == 39)) {
                missiles[2].update();
                missiles[2].launched = true;
            }
            else if ((lnch == 2 || lnch == 44 || lnch == 22)) {
                missiles[3].update();
                missiles[3].launched = true;
            }
            else if ((lnch == 134 || lnch == 127 || lnch == 159)) {
                missiles[4].update();
                missiles[4].launched = true;
            }
            else if ((lnch == 241 || lnch == 149 || lnch == 197)) {
                missiles[5].update();
                missiles[5].launched = true;
            }
            else if ((lnch == 250 || lnch == 110 || lnch == 180)) {
                missiles[6].update();
                missiles[6].launched = true;
            }
            else if ((lnch == 300 || lnch == 175 || lnch == 74)) {
                missiles[7].update();
                missiles[7].launched = true;
            }
            for (auto& missile : missiles) {
                if (missile.launched == true) {
                    missile.update();
                }
                if (missile.collides_with(player_rect)) {
                    stage = "Lost";
                }
            }
            

            // Jugador     

            barry.move(&fall, &player_x, &player_y, player_speed, deltaTime, &player_rect, &obstacle_rect, &roof_rect, &floor_rect, &reverse_floor_rect, &running);
            barry.handle_input(bullets, bullet_texture, player_rect);
            

            // Balas

            for (auto& bullet : bullets) {
                bullet.shoot(player_rect.x);
            }
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [&](const Bullet& b) {
                bool out_of_bounds = b.rect.x < -b.rect.w || b.rect.y > screen_height + b.rect.h;
                bool hit_floor = SDL_HasRectIntersectionFloat(&b.rect, &floor_rect) || SDL_HasRectIntersectionFloat(&b.rect, &reverse_floor_rect);
                return out_of_bounds || hit_floor;
            }), bullets.end());


            // Animaciones y renderizados

            if (++player_frame > 40) player_frame = 0;
            barry.animate(renderer, &player_texture, player_frame);
            update_background_scroll(&floor_rect, &reverse_floor_rect, &bg_rect, &reverse_bg_rect);


            SDL_RenderTexture(renderer, background, NULL, &bg_rect);
            SDL_RenderTexture(renderer, reverse_background, NULL, &reverse_bg_rect);
            SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
            SDL_RenderTexture(renderer, roof, NULL, &roof_rect);
            SDL_RenderTexture(renderer, floor, NULL, &floor_rect);
            SDL_RenderTexture(renderer, reverse_floor, NULL, &reverse_floor_rect);

            for (auto& bullet : bullets) {
                bullet.render(renderer);
            }

            for (auto& missile : missiles) {
                missile.render();
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(background);
    SDL_DestroyTexture(reverse_background);
    SDL_DestroyTexture(floor);
    SDL_DestroyTexture(reverse_floor);
    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(roof);
    SDL_DestroyTexture(bullet_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();
    return 0;
}