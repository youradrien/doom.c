#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

#define FPS 60
#define POV 120
#define FRAME_TARGET_TIME (1000/FPS)
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200
#define BLACK 0


static  struct
{
    int     x,y; 
    float   rotation;
    int l; // vertical lookup 

}       player;

static struct
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture *texture;

    uint32_t *buffer;

    bool quit;
	
    float deltaTime;
    uint32_t l_frameTime;
} game_state;

float degToRad(float a) { return a*M_PI/180.0;}

static void player_movement()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type)
    {
        case SDL_QUIT:
	          game_state.quit = false;
	          break;
	case SDL_KEYDOWN:
	          if(event.key.keysym.sym == SDLK_ESCAPE)
	            game_state.quit = false;
	          break;
    }
    int d_x = cos(degToRad(player.rotation)) * 10.0f;
    int d_y = sin(degToRad(player.rotation)) * 10.0f;

    // rotate 
}

static void verline(int x, int y0, int y1, uint32_t color)
{
   
}


static void render_3d()
{
    verline(10, 10, 100, 200); 
}

static void present()
{
    // cpu pixel buffer => locked sdl texture 
    // [hardware acceleration?] => video memory alloction?
    void *px;
    int pitch;
    // lock texture before writing to it
    int z = SDL_LockTexture(game_state.texture, NULL, &px, &pitch);
    if(z != 0)
        printf("e \n");
    // #pragma omp parallel for
    {
        for(size_t u = 0; u < WINDOW_HEIGHT; u ++)
        {
	    memcpy(
	    	&((uint8_t*) px)[u * pitch],
                &game_state.buffer[u * WINDOW_WIDTH],
                WINDOW_WIDTH * 4  
             );
	}
    }
    SDL_UnlockTexture(game_state.texture);

    SDL_SetRenderTarget(game_state.renderer, NULL);
    SDL_SetRenderDrawColor(game_state.renderer, 0, 0, 0, 0xFF);
    SDL_SetRenderDrawBlendMode(game_state.renderer, SDL_BLENDMODE_NONE);

    // printf("parallel for ends.\n");
    SDL_RenderClear(game_state.renderer);

    SDL_RenderCopy(game_state.renderer, game_state.texture, NULL, NULL);
    SDL_RenderPresent(game_state.renderer);	
}


static void destroy_window()
{
    SDL_DestroyTexture(game_state.texture);
    SDL_DestroyRenderer(game_state.renderer);
    SDL_DestroyWindow(game_state.window);
    SDL_Quit();
 
    free(game_state.buffer);
    return;
}

static int init_doom()
{  
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Error initializing SDL. \n");
        return -1;
    }
    game_state.window = SDL_CreateWindow(
        "Doom",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT, 0
    );
    if(!game_state.window)
    {
        fprintf(stderr, "Error creating SDL Window. \n");
        return -1;
    }
    game_state.renderer = SDL_CreateRenderer(
        game_state.window, -1, 0 
    );
    if(!game_state.renderer) {
        fprintf(stderr, "Error creating SDL Renderer. \n");
        return -1;
    }   

    // screen size texture
    game_state.texture =
      SDL_CreateTexture(
         game_state.renderer,
         SDL_PIXELFORMAT_ABGR8888,
         SDL_TEXTUREACCESS_STREAMING,
         WINDOW_WIDTH,
         WINDOW_HEIGHT
    );

    // initializing pixel buffer
    game_state.buffer = malloc(
            (WINDOW_WIDTH * WINDOW_HEIGHT * 4) * 
            (sizeof(int))
    );
    if (!game_state.buffer){
        fprintf(stderr, "Error allocating game_buffer. \n");
        return (-1);
    }

    // screen size texture
    game_state.texture =
      SDL_CreateTexture(
         game_state.renderer,
         SDL_PIXELFORMAT_ABGR8888,
         SDL_TEXTUREACCESS_STREAMING,
         WINDOW_WIDTH,
         WINDOW_HEIGHT
    );
    if(!game_state.texture)
    {
        fprintf(stderr, "Error creating texture. \n");
        return (-1);
    }

    game_state.quit = false;
    player.x = 0;
    player.y = 0;  
    return 0;
}

void update(void)
{
    // reset buffer pixels to black
    memset(game_state.buffer, BLACK, WINDOW_WIDTH * WINDOW_HEIGHT * 4);
    // uint32_t timeout = SDL_GetTicks() + FRAME_TARGET_TIME;
    uint32_t time_toWait = FRAME_TARGET_TIME - (SDL_GetTicks() - game_state.l_frameTime );
    // lock at frame target time
    // by comparing ticks 
    if(time_toWait > 0 && time_toWait <= FRAME_TARGET_TIME)
    {
	SDL_Delay(time_toWait);
    } 
    //while(!SDL_TICKS_PASSED(SDL_GetTicks(), timeout ) )
    game_state.deltaTime  = (SDL_GetTicks() -  game_state.l_frameTime) / 1000.0f;
    game_state.l_frameTime = SDL_GetTicks();
   	
    present();   
}


int main(void)
{
    if(init_doom() == -1){
        destroy_window();
        return (EXIT_FAILURE);
    } 
    while(!(game_state.quit))
    {
        player_movement();
        update();
    }
    printf("Game is runnning.. [%dx%d]\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    return 0; 
}
