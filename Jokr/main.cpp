#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
static TTF_Font *baseFont = nullptr;
static SDL_Texture *titleTexture = nullptr;
static SDL_Rect titleRect = {0, 0, 0, 0};
static bool running = true;

void init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    TTF_Init();

    window = SDL_CreateWindow("Jokr", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 272, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    baseFont = TTF_OpenFont("res/font.ttf", 32);
    if (!baseFont)
    {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        exit(1);
    };

    SDL_Surface *titleSurface = TTF_RenderUTF8_Blended(baseFont, "Jokr", {0, 0, 0, 255});
    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_FreeSurface(titleSurface);
    SDL_QueryTexture(titleTexture, nullptr, nullptr, &titleRect.w, &titleRect.h);
    titleRect.x = 480 / 2 - titleRect.w / 2;
}

int main(int argc, char *argv[])
{
    init();

    SDL_Event event;
    while (running)
    {
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_CONTROLLERDEVICEADDED:
                SDL_GameControllerOpen(event.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                    running = false;
                break;
            }
        }

        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderPresent(renderer);
    }

    return 0;
}