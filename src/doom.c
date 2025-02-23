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
                float d_x = cos(DEG2RAD((double)(g_->p.rotation  + s)));
                float d_y = sin(DEG2RAD((double)(g_->p.rotation  + s))); 
                g_->p.pos.y += 2.0f * d_y;
                g_->p.pos.x += 2.0f * d_x;
            }
            // a
            if(i == 0)
                g_->p.rotation -= 1;
            // e
            if(i == 4)
                g_->p.rotation += 1;
        }
    } 
    if(g_->p.pos.x < 0) g_->p.pos.x = 0;
    if(g_->p.pos.y < 0) g_->p.pos.y = 0;
    if(g_->p.pos.x >= (float)(WINDOW_WIDTH - 6)) g_->p.pos.x = WINDOW_WIDTH - 6.0f;
    if(g_->p.pos.y >= (float)(WINDOW_HEIGHT - 6)) g_->p.pos.y = WINDOW_HEIGHT - 6.0f;
    if(g_->p.rotation > 360.0f) g_->p.rotation = 0.0f;
    if(g_->p.rotation < -360.0f) g_->p.rotation = 0.0f;
    //printf("p->rot: %f° \n", g_->p.rotation);
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

static void vert_line(game_state *g_, int x, int y0, int y1, uint32_t color)
{
    for(uint32_t i = y0; i < y1; i++)
    {
        const int ix = (WINDOW_WIDTH * i) + x;
        g_->buffer[ix] = (color);
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


// rotate vector v by angle a (in radians)
static inline v2 rotate_v2(v2 v, f32 a)
{
    return (v2){
        (v.x * cos(a)) - (v.y * sin(a)),
        (v.x * sin(a)) + (v.y * cos(a))
    };
}

// calculate intersection of two segments
// https://en.wikipedia.org/wiki/Line-line_intersection
// (return NAN, NAN) if no intersection
static inline v2 intersect_segments(v2 a0, v2 a1, v2 b0, v2 b1)
{
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y))
            - ((a0.y - a1.y) * (b0.x - b1.x));


    // When two lines are parallel or coincident, denominator is zero.
    if(fabsf(d) < 0.00001f){ 
        return (v2) {NAN, NAN};
    }

    const f32 
        t = ( (a0.x - b0.x) * (b0.y - b1.y)
               - (a0.y - b0.y) * (b0.x - b1.x) )/ d,
        u = ( (a0.x - b0.x) * (a0.y - a1.y)
             - (a0.y - b0.y) * (a0.x - a1.x) ) / d;
    // printf("recomputation:  t = %f u = %f \n", t, u);    
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1) ? 
        (v2){
            a0.x + (t * (a1.x - a0.x)),
            a0.y + (t * (a1.y - a0.y))
        } : (v2) {NAN, NAN};
}

// convert angle to [-(FOV/2)...+(FOV/2)] => [x in 0..WINDOW_WIDTH] 

/*static inline int screen_angle_to_x(f32 angle) {
    return
        ((int) (SCREEN_WIDTH / 2))
            * (1.0f - tan(((angle + (HFOV / 2.0)) / HFOV) * PI_2 - PI_4));
}*/
// clamp values fix behavior near asymptote (PI/2):
// - as the angle approaches (𝜋/2, -𝜋/2) (90°, -90°)
// - tan(θ) approaches negative-positive infinity
static inline int screen_angle_to_x(game_state *g_, f32 angle) 
{ 
    // catch asymptote correctly
    // (with epsilon) 
    if(
        (fabs(angle) <= (1.571) + 0.02f)
        && (fabs(angle) >= (1.571) - 0.02f)
    ){
        // then clamp it by epsilon
        angle += (angle < 0 ? (+EPSILON) : (-EPSILON));
    }
    return 
        // screenclamp(
            ((int) (WINDOW_WIDTH / 2))
                * (1.0f - tan(
                    ((angle + (HFOV / 2.0)) / HFOV) 
                        * PI_2 - PI_4));
        // );
}

// 2D COORDINATES (world space)
// -> 3D COORDAINTES (camera space)
static inline v2 world_to_camera(game_state *g_, v2 p)
{
    // 1. translate
    const v2 t = (v2){
        p.x - g_->p.pos.x, 
        p.y - g_->p.pos.y
    };
    // 2. rotate
    return (v2)
        {
            t.x * (sin(DEG2RAD(g_->p.rotation))) - t.y * (cos(DEG2RAD(g_->p.rotation))),
            t.x * (cos(DEG2RAD(g_->p.rotation))) + t.y * (sin(DEG2RAD(g_->p.rotation)))
        };
}

// (radian)
// noramlize angle to +/-PI
static inline f32 normalize_angle(f32 a)
{
    return a - (TAU * floor((a + M_PI) / TAU));
}


// 2D Coordinates to 3D
// 3. Wall-Cliping (test if lines intersects with player viewcone/viewport by using vector2 cross product)
// 4. Perspective transformation  (scaling lines according to their dst from player, for appearance of perspective (far-away objects are smaller).
// 5. [COMING SOON ? ] Z-buffering (track depth of pixel to avoid rendering useless surfaces)
static void render(game_state *g_)
{
    // transform world view to 1st-person view   
    if(g_->render_mode)
    {
        for(uint32_t i = 0; i < g_->scene._walls_ix; i ++)
        {     
            const wall *l = &(g_->scene._walls[i]);

            const v2
                zdr = rotate_v2((v2){ 0.0f, 1.0f}, + (HFOV / 2.0f)),
                zdl = rotate_v2((v2){ 0.0f, 1.0f}, - (HFOV / 2.0f)),
                znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR }, // z-near left
                znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR }, // z-near right
                zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR }, // z-far left
                zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR }; // z-far right

            // line-pos converted from [world-space] to [camera-space]
            const v2 
                op0 = (v2)(world_to_camera(g_, l->a)),
                op1 = (v2)(world_to_camera(g_, l->b));
            
            // compute cliped positions
            v2 cp0 = op0, cp1 = op1;

            // both are behind player -> wall behind camera
            if(cp0.y <= 0 && cp1.y <= 0){
                printf("BRRR \n");
                continue;
            }

            // angle-clip against view frustum
            f32
                ac0 = normalize_angle((double)(- (atan2(cp0.y, cp0.x)  - PI_2))),
                ac1 = normalize_angle((double)(- (atan2(cp1.y, cp1.x) - PI_2)));

            printf("[A: %d°]\t[B: %d] \n", (int)(radToDeg(ac0)), (int)(radToDeg(ac1)));
            bool slim_thick = false;
            // clip against frustum (if wall intersects with clip planes)
            if(  
                (cp0.y < ZNEAR)
                || (cp1.y < ZNEAR)
                || (ac0 > +(HFOV / 2)) // 1
                // || (ac0 < -(HFOV / 2)) // 2
                || (ac1 < -(HFOV / 2)) // 1
               // || (ac1 > +(HFOV / 2)) // 2
            ){
                // todo: make case-2 work
                //
                // know where the intersection is on the (-HFOV/2 or HFOV/2) line delimiting player's fov
                v2 
                    left = intersect_segments(cp0, cp1, znl, zfl),
                    right = intersect_segments(cp0, cp1, znr, zfr);


                // recompute angles ac0, ac1
                if(!isnan(left.x)){
                    cp0 = left;
                    ac0 = normalize_angle((double)(- (atan2(cp0.y, cp0.x) - PI_2)));
                }
                if(!isnan(right.x)){
                    cp1 = right;
                    ac1 = normalize_angle((double) (- (atan2(cp1.y, cp1.x) - PI_2)));    
                }
            }
            
            // wrong side of the wall
            if(ac0 < ac1)
            {
                printf("lowkey catchted here tho mb \n");
                continue;
            }

            // wall attributes
            // todo: parse wad to get real values of wall floor and ceiling heights
            float z_floor = 12.0f;
            float z_ceiling = 0.0f;

            // ignore far -HFOV/2..+HFOV/2
            // check if angles are entirely far of bounds
            if( (ac0 < -(HFOV / 2) && ac1 < -(HFOV / 2))
                || (ac0 > +(HFOV / 2) && ac1 > +(HFOV/ 2))
            ){
                continue;
            }
            // wall y-scale
            const f32 
                sy0 = ifnan((float)(VFOV * WINDOW_HEIGHT) / cp0.y, 1e10);
            const f32
                sy1 = ifnan((float)(VFOV * WINDOW_HEIGHT) / cp1.y, 1e10);

            // wall x's
            const int
                    wx0 = screenclamp(
                            screen_angle_to_x(g_, ac0)
                        ),
                    wx1 = screenclamp(
                            screen_angle_to_x(g_, ac1)
                        );
            // wall y's
            const int 
                   wy_b0 = (WINDOW_HEIGHT / 2) + (int) (z_floor * sy0),
                   wy_b1 = (WINDOW_HEIGHT / 2) + (int) (z_floor * sy1),
                   wy_t0 = (WINDOW_HEIGHT / 2) + (int) (z_ceiling * sy0),
                   wy_t1 = (WINDOW_HEIGHT / 2) + (int) (z_ceiling * sy1);

            /*
            printf(
                    "\033[1m------------- WALL[%d]: ---------------\033[0m\
                    \n\033[32m x-degrees:\t A[x=%d °=%d] B[x=%d °=%d]   \033[0m \
                    \n\033[33m yscale:\t A[%f] B[%f]   \033[0m \
                    \n\033[34m top-bottom:\t A[%d] B[%d]    \033[0m \n",
                    i,
                    wx0, (int)(radToDeg(ac0)), wx1, (int)(radToDeg(ac1)), sy0, sy1, (int)cp0.y, (int)cp1.y
            );
            */
            
            // wall-outines
            // bottom-top-left-right
            d_line(g_, wx0, wy_b0, wx1, wy_b1, 0xFF00FF);
            d_line(g_, wx0, wy_t1, wx1, wy_t1, 0xFF00FF);
            d_line(g_, wx0, wy_b0, wx0, wy_t0, 0xFF00FF);
            d_line(g_, wx1, wy_b1, wx1, wy_t1, 0xFF00FF);
            // ver
            // wall-filled
            //
            // todo:
            // texture-mapping
        }
    }
    else
    {

      // player square and line 
        for(uint32_t i = 0; i < FOV; i ++)
        {
            d_line(g_, g_->p.pos.x + 3, g_->p.pos.y + 3, 
               g_->p.pos.x + 3 + (200.0f * cos(DEG2RAD((float)((g_->p.rotation - (FOV / 2)) + i)))),
               g_->p.pos.y + 3 + (200.0f * sin(DEG2RAD((float)((g_->p.rotation - (FOV/2)) + i)))),
               0xAAAAAA
            );        
        }
        d_line(g_, g_->p.pos.x + 3, g_->p.pos.y + 3, 
           g_->p.pos.x + 3 + (50.0f * cos(DEG2RAD(g_->p.rotation))),
           g_->p.pos.y + 3 + (50.0f * sin(DEG2RAD(g_->p.rotation))),
           0xFFFF00
        );        
        d_rect(g_, g_->p.pos.x, g_->p.pos.y, 5, 5); 
        // world-view
        // walls
        for(uint32_t i = 0; i < g_->scene._walls_ix; i ++)
        {
            d_line(g_, 
                g_->scene._walls[i].a.x, g_->scene._walls[i].a.y, 
                g_->scene._walls[i].b.x, g_->scene._walls[i].b.y,
                0xFFFFFF
            );
            // a
            d_rect(g_, g_->scene._walls[i].a.x - 2,  g_->scene._walls[i].a.y - 2, 4, 4); 
            // b
            d_rect(g_, g_->scene._walls[i].b.x - 2,  g_->scene._walls[i].b.y - 2, 4, 4); 
        }

    }
}

static void scene(game_state *g_)
{ 
    g_->scene._walls = (wall *) malloc(100 * sizeof(wall));
    if (!g_->scene._walls)
    {

    }
    wall *l = &(g_->scene._walls[0]);

    l->a.x = WINDOW_WIDTH / 2;  l->b.x = WINDOW_WIDTH/2;
    l->a.y = 100;               l->b.y = 200;
    l->color = 0xFF00FF;

    /*
    l = &(g_->scene._walls[1]);
    l->a.x = WINDOW_WIDTH / 2; l->b.x = WINDOW_WIDTH/2 + 300;
    l->a.y = 100;              l->b.y = 100;
    l->color = 0xFFFF00;
    */ 
    g_->scene._walls_ix = 1;
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
    (*g_)->p.pos.x = WINDOW_WIDTH / 3;
    (*g_)->p.pos.y = WINDOW_HEIGHT / 3;
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
    scene(g_);
    printf("Game is runnning.. [%dx%d]\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    while(!(g_->quit))
    {
        player_movement(g_);
        update(g_);
        //
    }
    destroy_window(g_);
    return (EXIT_SUCCESS); 
}
