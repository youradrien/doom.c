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
            if(event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z')
            {
                g_->pressed_keys[event.key.keysym.sym - 'a'] = 1; 
            }
            if(event.key.keysym.sym == SDLK_ESCAPE)
	        g_->quit = true;
           if(event.key.keysym.sym == SDLK_m)
	        g_->render_mode = !(g_->render_mode);
            break;
        case SDL_KEYUP:
            if(event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z')
            {
                g_->pressed_keys[event.key.keysym.sym - 'a'] = 0; 
            }           
            break;
    } 
    for(short int i = 0; i < 26; i ++)
    {
        if(g_->pressed_keys[i] == 1)
        {
            // q z d s
            if(i == 16 || i == 25 || i == 3 || i == 18)
            {
                float s = 0.0f;
                if(i == 16) s = -90.0f;
                if(i == 3) s = 90.0f;
                if(i == 18) s = -180.0f;
                float d_x = cos(degToRad((double)(g_->p.rotation  + s)));
                float d_y = sin(degToRad((double)(g_->p.rotation  + s))); 
                g_->p.y += 6.0f * d_y;
                g_->p.x += 6.0f * d_x;
            }
            // a
            if(i == 0)
                g_->p.rotation += 4;
            // e
            if(i == 4)
                g_->p.rotation -= 4;
        }
    } 
    if(g_->p.x < 0) g_->p.x = 0;
    if(g_->p.y < 0) g_->p.y = 0;
    if(g_->p.x >= (float)(WINDOW_WIDTH - 6)) g_->p.x = WINDOW_WIDTH - 6.0f;
    if(g_->p.y >= (float)(WINDOW_HEIGHT - 6)) g_->p.y = WINDOW_HEIGHT - 6.0f;
    if(g_->p.rotation > 360.0f) g_->p.rotation = 0.0f;
    if(g_->p.rotation < -360.0f) g_->p.rotation = 0.0f;
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
    for(uint32_t i = 0; i < width; i ++ )
    {
        d_line(g_,
            x + i, y,
            x + i, y + height,
            0xFF00FF
        );
    }
}


static void render(game_state *g_)
{
    if(g_->render_mode)
    {
        float r = degToRad(g_->p.rotation);
        float anchr = WINDOW_WIDTH / 2;
        // transform world view to 1st-person view
        //
        // 1. transform vertexes relative to the player
        float tx1 = anchr - (g_->p.x), ty1 = 100.0f - (g_->p.y);
        float tx2 = anchr - (g_->p.x), ty2 = 500.0f - (g_->p.y);
        // 2. rotate them around the players view
        /*
         * float tz1 = tx1 * cos(r) + ty1 * sin(r);
        float tz2 = tx2 * cos(r) + ty2 * sin(r);
        tx1 = tx1 * sin(r) - ty1 * cos(r);
        tx2 = tx2 * sin(r) - ty2 * cos(r);
        */
        d_line(g_, WINDOW_WIDTH / 2 + tx1, ty1, WINDOW_WIDTH /2 + tx2, ty2, 0xFFFFFF);
        d_line(g_, 
            anchr + 3, 
            WINDOW_HEIGHT / 2 + 3, 
            anchr + 3 + (50.0f * cos(degToRad(g_->p.rotation))),
            WINDOW_HEIGHT / 2 + 3 + (50.0f * sin(degToRad(g_->p.rotation))),
            0xFFFFFF
        );
        d_rect(g_, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 5, 5);     }
    else
    {
        // world-view
        d_line(g_, WINDOW_WIDTH / 2, 100, WINDOW_WIDTH / 2, 500, 0xFFFFFF); 
        // player square and line
        d_line(g_, g_->p.x + 3, g_->p.y + 3, 
           g_->p.x + 3 + (50.0f * cos(degToRad(g_->p.rotation))),
           g_->p.y + 3 + (50.0f * sin(degToRad(g_->p.rotation))),
           0xFFFFFF
        );
        d_rect(g_, g_->p.x, g_->p.y, 5, 5); 
    }
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
    // free(g_->lines);
    free(g_);
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
    (*g_)->p.x = WINDOW_WIDTH / 3;
    (*g_)->p.y = WINDOW_HEIGHT / 3;
    (*g_)->p.rotation = 0;
    (*g_)->render_mode = false;
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
    printf("Game is runnning.. [%dx%d] %f \n", WINDOW_WIDTH, WINDOW_HEIGHT, g_->p.x);
    while(!(g_->quit))
    {
        player_movement(g_);
        update(g_);
        //
    }
    destroy_window(g_);
    return (EXIT_SUCCESS); 
}
