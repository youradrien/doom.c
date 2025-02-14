#include "doom.h"


static void player_movement(game_state *g_)
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type)
    {
        case SDL_QUIT:
	    g_->quit = true;
	    break;
	case SDL_KEYDOWN:
            if(event.key.keysym.sym == SDLK_ESCAPE)
	        g_->quit = true;
            int d_x;
            int d_y;
            if(event.key.keysym.sym == SDLK_z)
            {
                d_x = cos(degToRad(g_->p.rotation)) * 2.0f;
                d_y = sin(degToRad(g_->p.rotation)) * 2.0f; 
            }
            if(event.key.keysym.sym == SDLK_s)
            {
                d_x = cos(degToRad(g_->p.rotation + 180.0f )) * 2.0f;
                d_y = sin(degToRad(g_->p.rotation + 180.0f )) * 2.0f;	         
            }
            if(event.key.keysym.sym == SDLK_q)
            {
                 d_x = cos(degToRad(g_->p.rotation + 90.0f )) * 2.0f;
                 d_y = sin(degToRad(g_->p.rotation + 90.0f )) * 2.0f;	         
            }
            if(event.key.keysym.sym == SDLK_d)
            {
                d_x = cos(degToRad(g_->p.rotation - 90.0f )) * 2.0f;
                d_y = sin(degToRad(g_->p.rotation - 90.0f )) * 2.0f;   
            }       
            if(event.key.keysym.sym == SDLK_z || event.key.keysym.sym == SDLK_s 
                || event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_q)
            {
                g_->p.y += d_y;
                g_->p.x += d_x;
            }
            if(event.key.keysym.sym == SDLK_a)
                g_->p.rotation ++;
            if(event.key.keysym.sym == SDLK_e)
                g_->p.rotation --;
            break;
    }
    if(g_->p.x < 0)
        g_->p.x = 0;
    if(g_->p.y < 0)
        g_->p.y = 0;
    if(g_->p.rotation > 360.0f)
        g_->p.rotation = 0.0f;
    if(g_->p.rotation < 360.0f)
        g_->p.rotation = 0.0f;
    }

static void set_pixel_color(game_state *g_, int x, int y, int c)
{
    // prevents segfaults
    if(x < 0 || y < 0 || x > WINDOW_WIDTH || y > WINDOW_HEIGHT)
        return;
    int ix = (WINDOW_WIDTH * y) + x;
    g_->buffer[ix] = (c);
}

// bresenham's algorithm
static void d_line(game_state *g_, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        set_pixel_color(g_, x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break; // complete line
        }

        int e2 = err * 2;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void d_rect(game_state *g_, int x, int y, int width, int height)
{
    if((x < 0) || (y < 0) ||(x + width) > WINDOW_WIDTH || (y + height) > WINDOW_HEIGHT)
        return;
    /*
    nah bro
    SDL_Rect rect = { x, y, width, height};
    SDL_SetRenderDrawColor(g_->renderer, 0, 0, 255, 255);

    SDL_RenderFillRect(g_->renderer, &rect);
    */
    
}


static void render(game_state *g_)
{
    // d_rect(g_, g_->p.x, g_->p.y, g_->p.x + 100, g_->p.y + 100);
    d_line(g_, WINDOW_WIDTH / 2, 100, WINDOW_WIDTH / 2, 500, 0xFFFFFF); 
}

static void multiThread_present(game_state *g_)
{
    // cpu pixel buffer => locked sdl texture 
    // [hardware acceleration?] => video memory alloction?
    void *px;
    int pitch;
    // lock texture before writing to it (SDL uses mutex and multithread for textures rendering)
    SDL_LockTexture(g_->texture, NULL, &px, &pitch); 
    // multi-threaded loop operations for mempcpying 
    // game_state.buffer ==> into SDL texture 
    #pragma omp parallel for
    {
        for(size_t u = 0; u < WINDOW_HEIGHT; u ++)
        {
	    memcpy(
	    	&((uint8_t*) px)[u * pitch],
                &g_->buffer[u * WINDOW_WIDTH],
                WINDOW_WIDTH * (sizeof(int))
             );
	}
    }
    SDL_UnlockTexture(g_->texture);

    SDL_SetRenderTarget(g_->renderer, NULL);
    SDL_SetRenderDrawColor(g_->renderer, 0, 0, 0, 0xFF);
    SDL_SetRenderDrawBlendMode(g_->renderer, SDL_BLENDMODE_NONE);

    SDL_RenderClear(g_->renderer);
    SDL_RenderCopy(g_->renderer, g_->texture, NULL, NULL);
    SDL_RenderPresent(g_->renderer);	
}


static void destroy_window(game_state *g_)
{
    SDL_DestroyTexture(g_->texture);
    SDL_DestroyRenderer(g_->renderer);
    SDL_DestroyWindow(g_->window);
    SDL_Quit();
 
    free(g_->buffer);
    return;
}

int init_doom(game_state **g_)
{  
    *g_ =  malloc(1 * sizeof(game_state));
    if (!*g_)
    {
        fprintf(stderr, "Error allocating GAME_STATE. \n");
        return (-1);
    }
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Error initializing SDL. \n");
        return -1;
    }
    (*g_)->window = SDL_CreateWindow(
        "doom.c",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT, 0
    );
    if(!(*g_)->window)
    {
        fprintf(stderr, "Error creating SDL Window. \n");
        return -1;
    }
    (*g_)->renderer = SDL_CreateRenderer(
        (*g_)->window, -1, 0 
    );
    if(!(*g_)->renderer) {
        fprintf(stderr, "Error creating SDL Renderer. \n");
        return -1;
    }   

    // screen size texture
    (*g_)->texture =
      SDL_CreateTexture(
            (*g_)->renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        );

    // initializing pixel buffer
    (*g_)->buffer = malloc(
            (WINDOW_WIDTH * WINDOW_HEIGHT
                * sizeof(int))
            );
    if (!(*g_)->buffer)
    {
        fprintf(stderr, "Error allocating game_buffer. \n");
        return (-1);
    }

    // screen size texture
    (*g_)->texture =
      SDL_CreateTexture(
         (*g_)->renderer,
         SDL_PIXELFORMAT_ABGR8888,
         SDL_TEXTUREACCESS_STREAMING,
         WINDOW_WIDTH,
         WINDOW_HEIGHT
    );
    if(!(*g_)->texture)
    {
        fprintf(stderr, "Error creating texture. \n");
        return (-1);
    }
    printf("Initialization successful. \n");
    (*g_)->quit = false;
    (*g_)->p.x = 0;
    (*g_)->p.y = 0;
    (*g_)->p.rotation = 0;
    return 0;
}

static void update(game_state *g_)
{
    // reset buffer pixels to black
    memset(g_->buffer, BLACK, WINDOW_WIDTH * WINDOW_HEIGHT * 4);
   
    // uint32_t timeout = SDL_GetTicks() + FRAME_TARGET_TIME;
    uint32_t time_toWait = FRAME_TARGET_TIME - (SDL_GetTicks() - g_->l_frameTime );
    // lock at frame target time
    // by comparing ticks 
    if(time_toWait > 0 && time_toWait <= FRAME_TARGET_TIME)
    {
	SDL_Delay(time_toWait);
    }  
    g_->deltaTime  = (SDL_GetTicks() -  g_->l_frameTime) / 1000.0f;
    g_->l_frameTime = SDL_GetTicks();	
    render(g_);
    multiThread_present(g_);   

}


int main(int argc, char **argv)
{
    game_state  *g_ = NULL;
   
    if(init_doom(&g_) == -1)
    {
        destroy_window(g_);
        return (EXIT_FAILURE);
    } 
    printf("Game is runnning.. [%dx%d] %d \n", WINDOW_WIDTH, WINDOW_HEIGHT, g_->p.x);
    while(!(g_->quit))
    {
        player_movement(g_);
        update(g_);
        //
    }
    destroy_window(g_);
    return (EXIT_SUCCESS); 
}
