#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Event event;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (!SDL_CreateWindowAndRenderer("Hello SDL", 1366, 768, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    surface = SDL_LoadBMP("sample.bmp");
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        return 3;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        return 3;
    }
    SDL_DestroySurface(surface);


    float player_x, player_y, player_speed, player_width, player_height;
    player_x = 0;
    player_y = 0;
    player_speed = 2;
    player_width = 1366;
    player_height = 768;

    SDL_FRect dest = { player_x, player_y, player_width, player_height };

    while (1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            break;
        }
        else if( event.type == SDL_EVENT_KEY_DOWN )
        {
            if( event.key.key == SDLK_UP )
            {
                player_y -= player_speed;
            }
            else if( event.key.key == SDLK_DOWN )
            {
                player_y += player_speed;
            }
            else if( event.key.key == SDLK_LEFT )
            {
                player_x -= player_speed;
            }
            else if( event.key.key == SDLK_RIGHT )
            {
                player_x += player_speed;
            }
        }

        dest.x = player_x;
        dest.y = player_y;
        dest.w = player_width;
        dest.h = player_height;

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, &dest);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}