#define SDL_MAIN_HANDLED
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <pngDecoder.h>

static float get_delta_time(float *prev, float *new)
{
    *prev = *new;
    *new = SDL_GetPerformanceCounter();
    float frequency = SDL_GetPerformanceFrequency();
    return (*new - *prev) / frequency;
}

static int graphics_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture, const char *file)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    *window = SDL_CreateWindow("SDL is active!", 100, 100, 512, 512, 0);
    if (!*window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return -1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        return -1;
    }

    int width;
    int height;
    int channels;
    unsigned char* pixels = parse_png(file, &width, &height, &channels);

    if (!pixels)
    {
        SDL_Log("Unable to open image");
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return -1;
    }

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!*texture)
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        free(pixels);
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return -1;
    }
    SDL_UpdateTexture(*texture, NULL, pixels, width * 4);
    SDL_SetTextureAlphaMod(*texture, 255);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
    free(pixels);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Please write png filename");
        return -1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int ret = graphics_init(&window, &renderer, &texture, argv[1]);
    if (ret < 0)
    {
        goto quit;
    }

    int running = 1;
    float last_PC = 0;
    float new_PC = SDL_GetPerformanceCounter();
    float delta_time = 0;
    while (running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
        }
        SDL_PumpEvents();
        delta_time = get_delta_time(&last_PC, &new_PC);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect target_rect = {100, 100, 100, 100};
        SDL_RenderCopy(renderer, texture, NULL, &target_rect);

        SDL_RenderPresent(renderer);
    }

quit:
    SDL_Quit();
    return ret;
}