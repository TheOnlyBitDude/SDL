#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits>

#include "res/OpenALSound.hpp"

#include "res/fnt/MS-DOS.h"

#include "res/snd/Elektrik.h"
#include "res/snd/Explode.h"
#include "res/snd/jetpack_fire.h"
#include "res/snd/Launch.h" 
#include "res/snd/smash.h"
#include "res/snd/Theme.h"
#include "res/snd/Warning.h"

#include "res/img/roland.h"
#include "res/img/BarryDead.h"  // generated with xxd -i "input" > "output"
#include "res/img/bg_rvrs.h"
#include "res/img/bg.h"
#include "res/img/booster.h"
#include "res/img/bullet.h"
#include "res/img/elektrik_vert.h"
#include "res/img/elektrik.h"
#include "res/img/floor_rvrs.h"
#include "res/img/floor.h"
#include "res/img/Fly1.h"
#include "res/img/Fly2.h"
#include "res/img/Fly3.h"
#include "res/img/FlyFall.h"
#include "res/img/Koin.h"
#include "res/img/Missile_Target.h"
#include "res/img/Rocket1.h"
#include "res/img/Rocket2.h"
#include "res/img/Rocket3.h"
#include "res/img/Rocket4.h"
#include "res/img/Roof.h"
#include "res/img/Walk1.h"
#include "res/img/Walk2.h"
#include "res/img/Walk3.h"
#include "res/img/Walk4.h"


SDL_Texture* load_texture_from_memory(SDL_Renderer* renderer, const unsigned char* data, unsigned int data_len, const char* asset_name)
{
    SDL_IOStream* io = SDL_IOFromConstMem((void*)data, data_len);
    if (!io) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[%s] SDL_IOFromConstMem error: %s", asset_name, SDL_GetError());
        return nullptr;
    }

    SDL_Texture* tex = IMG_LoadTexture_IO(renderer, io, true);
    if (!tex) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[%s] IMG_LoadTexture_IO error: %s", asset_name, SDL_GetError());

    return tex;
}

TTF_Font* load_font_from_memory(const unsigned char* data, unsigned int data_len, int pt_size)
{
    SDL_IOStream* io = SDL_IOFromConstMem((void*)data, data_len);
    if (!io) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_IOFromConstMem (font) error: %s", SDL_GetError());
        return nullptr;
    }

    TTF_Font* font = TTF_OpenFontIO(io, true, pt_size);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "TTF_OpenFontIO error: %s", SDL_GetError());
        return nullptr;
    }

    return font;
}

void null_seed() {
    srand(time(NULL));
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
        if (texture) SDL_RenderTexture(renderer, texture, nullptr, &rect);
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

    bool dead = false;

    SDL_Texture* Walk1;
    SDL_Texture* Walk2;
    SDL_Texture* Walk3;
    SDL_Texture* Walk4;
    SDL_Texture* Fly1;
    SDL_Texture* Fly2;
    SDL_Texture* Fly3;
    SDL_Texture* FlyFall;
    OpenALSound* jetpack_fire;

    void init_audio() {
        jetpack_fire = new OpenALSound(res_snd_jetpack_fire_wav, res_snd_jetpack_fire_wav_len);
    }


    void move(float *fall, float *player_x, float *player_y, float player_speed, float deltaTime, SDL_FRect *player_rect, SDL_FRect *obstacle_rect, SDL_FRect *roof_rect, SDL_FRect *floor_rect, SDL_FRect *reverse_floor_rect, int *running) {
        const bool *keyboard = SDL_GetKeyboardState(NULL);
        bool on_floor = SDL_HasRectIntersectionFloat(player_rect, floor_rect) || SDL_HasRectIntersectionFloat(player_rect, reverse_floor_rect);
        bool on_roof = SDL_HasRectIntersectionFloat(player_rect, roof_rect);

        if (keyboard[SDL_SCANCODE_SPACE]) {
            animation = "Fly";
            if (!jetpack_fire->isPlaying() && !dead) jetpack_fire->play();
            *player_y -= *fall;
            *fall += 0.75f;

            if (on_floor) *fall = 4.0f;
            if (on_roof) {
                *fall = 0.0f;
                *player_y = roof_rect->y + roof_rect->h;
            }
        } else {
            if (jetpack_fire->isPlaying()) jetpack_fire->stop();
            *fall -= 0.75f;
            *player_y -= *fall;

            if (on_floor) {
                animation = "Run";
                *fall = 0.0f;
                *player_y = floor_rect->y - player_rect->h;
            } else animation = "Fall";
            if (on_roof) *player_y = roof_rect->y + roof_rect->h + 1;
        }

        if (*fall > 10.0f) *fall = 10.0f;
        if (*fall < -20.0f) *fall = -20.0f;

        player_rect->x = *player_x;
        player_rect->y = *player_y;
    }

    void load_textures(SDL_Renderer* renderer) {
        Walk1 = load_texture_from_memory(renderer, res_img_Walk1_png, res_img_Walk1_png_len, "Walk1");
        Walk2 = load_texture_from_memory(renderer, res_img_Walk2_png, res_img_Walk2_png_len, "Walk2");
        Walk3 = load_texture_from_memory(renderer, res_img_Walk3_png, res_img_Walk3_png_len, "Walk3");
        Walk4 = load_texture_from_memory(renderer, res_img_Walk4_png, res_img_Walk4_png_len, "Walk4");
        Fly1 = load_texture_from_memory(renderer, res_img_Fly1_png, res_img_Fly1_png_len, "Fly1");
        Fly2 = load_texture_from_memory(renderer, res_img_Fly2_png, res_img_Fly2_png_len, "Fly2");
        Fly3 = load_texture_from_memory(renderer, res_img_Fly3_png, res_img_Fly3_png_len, "Fly3");
        FlyFall = load_texture_from_memory(renderer, res_img_FlyFall_png, res_img_FlyFall_png_len, "FlyFall");
    }

    void free_textures() {
        SDL_DestroyTexture(Walk1);
        SDL_DestroyTexture(Walk2);
        SDL_DestroyTexture(Walk3);
        SDL_DestroyTexture(Walk4);
        SDL_DestroyTexture(Fly1);
        SDL_DestroyTexture(Fly2);
        SDL_DestroyTexture(Fly3);
        SDL_DestroyTexture(FlyFall);
    }

    void animate(SDL_Texture** player_texture, int player_frame) {
        if (animation == "Run") {
            if (player_frame <= 10) *player_texture = Walk1;
            else if (player_frame <= 20) *player_texture = Walk2;
            else if (player_frame <= 30) *player_texture = Walk3;
            else if (player_frame <= 40) *player_texture = Walk4;
        } else if (animation == "Fly") {
            if (player_frame <= 5) *player_texture = Fly1;
            else if (player_frame <= 10) *player_texture = Fly2;
            else if (player_frame <= 15) *player_texture = Fly3;
            else if (player_frame <= 20) *player_texture = FlyFall;
            else if (player_frame <= 25) *player_texture = Fly1;
            else if (player_frame <= 30) *player_texture = Fly2;
            else if (player_frame <= 35) *player_texture = Fly3;
            else if (player_frame <= 40) *player_texture = FlyFall;
        } else *player_texture = FlyFall;
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

    OpenALSound* launchSound;
    OpenALSound* warningSound;

    Missile(SDL_Renderer* rend, float x, float y, float width, float height, int spd, int dur, std::string type)
        : renderer(rend), w(width), h(height), speed(spd), duration(dur),
            counter(0), i(0), wait(0), pre_launch(false), launched(false), f(false),
            type(type)
        {
        rect = { x, y, width, height };
        texture = load_texture_from_memory(renderer, res_img_Missile_Target_png, res_img_Missile_Target_png_len, "Missile_Target");

        launchSound = new OpenALSound(res_snd_Launch_wav, res_snd_Launch_wav_len); // inicializa el sonido
        warningSound = new OpenALSound(res_snd_Warning_wav, res_snd_Launch_wav_len);
        }

    SDL_Texture* Target;
    SDL_Texture* Rocket1;
    SDL_Texture* Rocket2;
    SDL_Texture* Rocket3;
    SDL_Texture* Rocket4;

    void load_textures(SDL_Renderer* renderer) {
        Target = load_texture_from_memory(renderer, res_img_Missile_Target_png, res_img_Missile_Target_png_len, "Missile_Target");
        Rocket1 = load_texture_from_memory(renderer, res_img_Rocket1_png, res_img_Rocket1_png_len, "Rocket1");
        Rocket2 = load_texture_from_memory(renderer, res_img_Rocket2_png, res_img_Rocket2_png_len, "Rocket2");
        Rocket3 = load_texture_from_memory(renderer, res_img_Rocket3_png, res_img_Rocket3_png_len, "Rocket3");
        Rocket4 = load_texture_from_memory(renderer, res_img_Rocket4_png, res_img_Rocket4_png_len, "Rocket4");
    }

    void free_textures() {
        SDL_DestroyTexture(Target);
        SDL_DestroyTexture(Rocket1);
        SDL_DestroyTexture(Rocket2);
        SDL_DestroyTexture(Rocket3);
        SDL_DestroyTexture(Rocket4);
    }

    void animate() {
        if (counter >= 0 && counter < 2) texture = Rocket1;
        else if (counter >= 2 && counter < 4) texture = Rocket2;
        else if (counter >= 4 && counter < 6) texture = Rocket3;
        else if (counter >= 6 && counter < 8) texture = Rocket4;

        counter++;
        if (counter >= 8) counter = 0;
    }

    void warning() {
        if (!pre_launch) {
            texture = Target;
            pos = static_cast<float>(20 + rand() % (668 - 20));
            rect.y = pos;
            launched = false;
            if (warningSound) warningSound->play(false);
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
            if (launchSound) launchSound->play(false);
        }

        if (wait == 35) {
            if (i != duration) {
                if (type == "wave"){
                    if (orientation == "Down") fall -= 1.125f;
                    else if (orientation == "Up") fall += 1.125f;
                    if (fall >= 12) orientation = "Down";
                    else if (fall <= -7) orientation = "Up";
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

            if (!f) f = true;

            rect.y = pos;
            animate();
        } else wait++;
    }

    void update() {
        warning();
    }

    void render() {
        if (texture) SDL_RenderTexture(renderer, texture, NULL, &rect);
    }

    bool collides_with(const SDL_FRect& player_rect) {
        return SDL_HasRectIntersectionFloat(&rect, &player_rect);
    }
};

class Electricity {
public:
    SDL_FRect rect;
    SDL_Texture* texture;
    SDL_Renderer* renderer;

    SDL_Texture* Horizontal;
    SDL_Texture* Vertical;

    int speed;
    std::string type;

    Electricity(SDL_Renderer* rend, float x, float y, float width, float height, int spd, std::string type)
        : renderer(rend), speed(spd), type(type),
          Horizontal(nullptr), Vertical(nullptr)
    {
        rect = { x, y, width, height };
        texture = nullptr;
    }

    void load_textures() {
        Horizontal = load_texture_from_memory(renderer, res_img_elektrik_png, res_img_elektrik_png_len, "elektrik");
        Vertical = load_texture_from_memory(renderer, res_img_elektrik_vert_png, res_img_elektrik_vert_png_len, "elektrik_vert");

        // After loading, assign the correct texture for this instance
        if (type == "horizontal") texture = Horizontal;
        else texture = Vertical;
    }

    void free_textures() {
        SDL_DestroyTexture(Horizontal);
        SDL_DestroyTexture(Vertical);
    }

    void launch() {
        rect.x -= static_cast<float>(speed);
    }

    void render() {
        if (texture) SDL_RenderTexture(renderer, texture, NULL, &rect);
    }

    bool collides_with(const SDL_FRect& player_rect) {
        return SDL_HasRectIntersectionFloat(&rect, &player_rect);
    }
};

void draw_hitbox(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color color, int thickness) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int i = 0; i < thickness; i++) {
        SDL_FRect r = {
            rect.x + i,
            rect.y + i,
            rect.w - 2 * i,
            rect.h - 2 * i
        };
        SDL_RenderRect(renderer, &r);
    }
}


int main(int argc, char *argv[]) {

    null_seed();
    int screen_width = 1366, screen_height = 768;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_Color white = {255, 255, 255, 255};

    bool hitboxes = false;

    if (!SDL_Init(SDL_INIT_VIDEO)) return 3;
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    for (int i = 1; i < argc; ++i) {  // start at 1 to skip the program name
        std::string arg = argv[i];
        if (arg == "--hitboxes") hitboxes = true;
    }
    if (!SDL_CreateWindowAndRenderer("PyPack Joyride beta", screen_width, screen_height, SDL_WINDOW_BORDERLESS, &window, &renderer)) return 3;
    if (!TTF_Init()) return 3;

    TTF_Font* font = load_font_from_memory(res_fnt_ModernDOS9x16_ttf, res_fnt_ModernDOS9x16_ttf_len, 32);

    TTF_SetFontHinting(font, TTF_HINTING_MONO);  // opcional pero recomendable


    SDL_Texture *player_texture = load_texture_from_memory(renderer, res_img_Walk1_png, res_img_Walk1_png_len, "Walk1");
    SDL_Texture *background = load_texture_from_memory(renderer, res_img_bg_jpg, res_img_bg_jpg_len, "bg");
    SDL_Texture *reverse_background = load_texture_from_memory(renderer, res_img_bg_rvrs_jpg, res_img_bg_rvrs_jpg_len, "bg_rvrs");
    SDL_Texture *floor = load_texture_from_memory(renderer, res_img_floor_png, res_img_floor_png_len, "floor");
    SDL_Texture *reverse_floor = load_texture_from_memory(renderer, res_img_floor_rvrs_png, res_img_floor_rvrs_png_len, "floor_rvrs");
    SDL_Texture *roof = load_texture_from_memory(renderer, res_img_roof_png, res_img_roof_png_len, "roof");
    SDL_Texture *bullet_texture = load_texture_from_memory(renderer, res_img_bullet_png, res_img_bullet_png_len, "bullet");


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


    std::vector<Electricity> electricities;

    TextLabel title(renderer);
    TextLabel Lost(renderer);
    title.setText("PyPack_Joyride!", font, white, static_cast<float>((screen_width / 2.5f) - title.rect.w), 50.0f);
    Lost.setText("You lost! Press anywhere to continue...", font, white, static_cast<float>((screen_width - Lost.rect.w) / 2.0f), 50.0f);


    std::string stage = "Title";
    SDL_FRect window_rect {0.0f, 0.0f, static_cast<float>(screen_width), static_cast<float>(screen_height)};


    barry.load_textures(renderer);
    barry.init_audio();
    for (auto& missile : missiles) missile.load_textures(renderer);

    OpenALSound Theme(res_snd_Theme_wav, res_snd_Theme_wav_len);
    OpenALSound Elektrik(res_snd_Elektrik_wav, res_snd_Elektrik_wav_len);
    OpenALSound Explode(res_snd_Explode_wav, res_snd_Explode_wav_len);
    Theme.setVolume(0.3f);
    
    while (running) {
        if (!Theme.isPlaying()) Theme.play(true);
        if (barry.dead && barry.jetpack_fire->isPlaying()) barry.jetpack_fire->stop();


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

                    if (SDL_PointInRectFloat(&mouse_point, &window_rect) && stage == "Title") stage = "Game";
                    if (SDL_PointInRectFloat(&mouse_point, &window_rect) && stage == "Lost") {
                        // Reset scene
                        for (auto& missile : missiles) {
                            missile.launched = false;
                            missile.rect.x = 1324.0f;
                            missile.rect.y = -missile.rect.h;
                            missile.f = 1;
                            missile.i = 0;
                            missile.wait = 0;
                            missile.pre_launch = false;
                            if (missile.launchSound->isPlaying()) missile.launchSound->stop();
                            if (missile.warningSound->isPlaying()) missile.warningSound->stop();
                        }

                        electricities.erase(std::remove_if(electricities.begin(), electricities.end(),
                            [](const Electricity& e) {
                                return true;
                            }), electricities.end());

                        fall = 0.0f;
                        player_y = floor_rect.y - player_rect.h;
                        stage = "Game";
                        barry.dead = false;
                    }
                    if (SDL_PointInRectFloat(&mouse_point, &player_rect)) SDL_Log("Haz hecho clic sobre el jugadór.");
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        if (stage == "Title") title.render();
        if (stage == "Lost") { 
            Lost.render();
            barry.dead = true;
        }
        if (stage == "Game") {

            // Randomize generations

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
            else if (lnch == 100 || lnch == 200 || lnch == 299 || lnch == 298) {
                electricities.emplace_back(renderer, static_cast<float>(screen_width), rand() % (screen_height - 100) + 50, 68.0f, 282.0f, 20, "vertical");
                electricities.back().load_textures();
            }
            else if (lnch == 50 || lnch == 80 || lnch == 90 || lnch == 91) {
                electricities.emplace_back(renderer, static_cast<float>(screen_width), rand() % (screen_height - 100) + 50, 282.0f, 68.0f, 20, "horizontal");
                electricities.back().load_textures();
            }
            for (auto& missile : missiles) {
                if (missile.launched == true) missile.update();
                if (missile.collides_with(player_rect)) {
                    stage = "Lost";
                    Explode.play();
                }
            }
            for (auto& elec : electricities) {
                elec.launch();
                if (elec.collides_with(player_rect)) {
                    stage = "Lost";
                    Elektrik.play();
                }
            }
            std::vector<Electricity> new_electricities;


            // Electricity laws

            for (auto& e : electricities) {
                bool hit_floor = SDL_HasRectIntersectionFloat(&e.rect, &floor_rect) ||
                                SDL_HasRectIntersectionFloat(&e.rect, &reverse_floor_rect);

                bool out_of_bounds = (e.rect.x < -e.rect.w);

                if (out_of_bounds) continue;
                else if (hit_floor) {
                    // Respawn at new random position

                    float new_x = static_cast<float>(screen_width);
                    float new_y = static_cast<float>(rand() % (screen_height - 100) + 50);
                    float width = 0.0f, height = 0.0f;

                    if (e.type == "vertical") {
                        width = 68.0f;
                        height = 282.0f;
                    } else if (e.type == "horizontal") {
                        width = 282.0f;
                        height = 68.0f;
                    }

                    // Add new electricity

                    new_electricities.emplace_back(renderer, new_x, new_y, width, height, 20, e.type);
                    new_electricities.back().load_textures();
                }
                else new_electricities.push_back(e);
            }
            electricities = std::move(new_electricities);


            // Player     

            barry.move(&fall, &player_x, &player_y, player_speed, deltaTime, &player_rect, &obstacle_rect, &roof_rect, &floor_rect, &reverse_floor_rect, &running); 
            barry.handle_input(bullets, bullet_texture, player_rect);
            

            // Bullets

            for (auto& bullet : bullets) bullet.shoot(player_rect.x);
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [&](const Bullet& b) {
                bool out_of_bounds = b.rect.x < -b.rect.w || b.rect.y > screen_height + b.rect.h;
                bool hit_floor = SDL_HasRectIntersectionFloat(&b.rect, &floor_rect) || SDL_HasRectIntersectionFloat(&b.rect, &reverse_floor_rect);
                return out_of_bounds || hit_floor;
            }), bullets.end());


            // Animations

            if (++player_frame > 40) player_frame = 0;
            barry.animate(&player_texture, player_frame);
            update_background_scroll(&floor_rect, &reverse_floor_rect, &bg_rect, &reverse_bg_rect);


            // Render

            SDL_RenderTexture(renderer, background, NULL, &bg_rect);
            SDL_RenderTexture(renderer, reverse_background, NULL, &reverse_bg_rect);
            SDL_RenderTexture(renderer, player_texture, NULL, &player_rect);
            SDL_RenderTexture(renderer, roof, NULL, &roof_rect);
            SDL_RenderTexture(renderer, floor, NULL, &floor_rect);
            SDL_RenderTexture(renderer, reverse_floor, NULL, &reverse_floor_rect);
            for (auto& bullet : bullets) bullet.render(renderer);
            for (auto& missile : missiles) missile.render();
            for (auto& elec : electricities) elec.render();


            // Render hitboxes

            if (hitboxes) {
                draw_hitbox(renderer, player_rect, SDL_Color{0, 255, 0, 255}, 2);
                draw_hitbox(renderer, obstacle_rect, SDL_Color{255, 0, 0, 255}, 2);
                draw_hitbox(renderer, roof_rect, SDL_Color{0, 0, 255, 255}, 2);
                draw_hitbox(renderer, floor_rect, SDL_Color{0, 0, 255, 255}, 2);
                draw_hitbox(renderer, reverse_floor_rect, SDL_Color{0, 0, 255, 255}, 2);
                for (auto& bullet : bullets) draw_hitbox(renderer, bullet.rect, SDL_Color{255, 255, 0, 255}, 2);
                for (auto& missile : missiles) draw_hitbox(renderer, missile.rect, SDL_Color{255, 0, 255, 255}, 2);
                for (auto& elec : electricities) draw_hitbox(renderer, elec.rect, SDL_Color{0, 255, 255, 255}, 2);
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
    barry.free_textures();
    for (auto& missile : missiles) missile.free_textures();
    for (auto& elec : electricities) elec.free_textures();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();
    return 0;
}