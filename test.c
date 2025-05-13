#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = SDL_CreateWindow("SDL3 Image Movement",
                                          100, 100, 800, 600, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL, 0);

    SDL_Surface *imageSurface = IMG_Load("image.png");
    if (!imageSurface) {
        SDL_Log("Failed to load image: %s", IMG_GetError());
        return 1;
    }

    SDL_Texture *imageTexture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    SDL_DestroySurface(imageSurface);

    SDL_Rect imageRect = {100, 100, 100, 100}; // initial position and size
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:  imageRect.x -= 10; break;
                    case SDLK_RIGHT: imageRect.x += 10; break;
                    case SDLK_UP:    imageRect.y -= 10; break;
                    case SDLK_DOWN:  imageRect.y += 10; break;
                    case SDLK_ESCAPE: running = false; break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, imageTexture, NULL, &imageRect);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(imageTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();

    return 0;
}
