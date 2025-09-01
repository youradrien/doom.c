#include "doom.h"
#include <stdint.h>

// rotate vector v by angle a (in radians)
static inline v2 rotate_v2(v2 v, f32 a)
{
    return (v2){
        (v.x * cos(a)) - (v.y * sin(a)),
        (v.x * sin(a)) + (v.y * cos(a))
    };
}

// dot product
static inline f32 dot(v2 a, v2 b)
{
    return a.x * b.x + a.y * b.y;
}

// squared length
static inline f32 length2(v2 v)
{
    return v.x * v.x + v.y * v.y;
}

// distance from point P to segment AB
static f32 p_dist_to_AB(v2 A, v2 B, v2 P)
{
    const v2 
        AB = (v2){ B.x - A.x, B.y - A.y },
        AP = (v2){ P.x - A.x, P.y - A.y };


    // projection factor t of P onto AB (0=at A, 1=at B)
    const f32 t = clamp( (dot(AP, AB) / length2(AB)), 
            0.0f, 1.0f
    ); // clamp to [0,1] stay within segment
  
    // nearest point on AB
    v2 nearest = { A.x + t * AB.x, A.y + t * AB.y };
    // distance from P to nearest point
    v2 diff = { P.x - nearest.x, P.y - nearest.y };

    return sqrtf(length2(diff));
}

static void player_collision(game_state *g_, v2 dir)
{
    if(!g_->p._sect)
        return ;

    const v2 attempted_move = (v2){
            g_->p.pos.x + dir.x,
            g_->p.pos.y + dir.y
    };
    // player attempted move: oldpos -> newpos
    for(uint32_t i = 0; i < g_->p._sect->n_walls; i ++)
    {
        const wall *w = g_->p._sect->walls[i];
        if(!w->_op_sect // solid wall
            || fabsf(g_->p._sect->floor - w->_op_sect->floor) > 4.0f) // or sections-steps too high
        {
            const f32 dist = p_dist_to_AB(*(w->a), *(w->b), attempted_move);
            if(dist < PLAYER_RADIUS)
            {
                return ;
                //// push player out, or slide along wall
                const v2 v = (v2){
                    w->b->x - w->a->x, 
                    w->b->y - w->a->y
                };
                const f32 
                    len = sqrtf(v.x * v.x + v.y * v.y);
                const v2 wall_dir = (v2){ 
                    v.x / len,
                    v.y / len 
                };
                const f32 d = dot(attempted_move, wall_dir);
                const v2 move = (v2){
                    wall_dir.x * d,
                    wall_dir.y * d
                }; 
                //  slide along wall
                g_->p.pos = (v2){
                    g_->p.pos.x + move.x,
                    g_->p.pos.y + move.y
                };
                return ;
            }        
        }
    }
    g_->p.pos = (attempted_move);
}

static void player_sector_pos(game_state *g_)
{
    // each sect
    for(uint32_t i = 0; i < g_->scene._sectors_count; i++)
    {
        sector *s = &g_->scene._sectors[i];
        if (!s || !s->vertices[0]
            || s->n_vertices <= 1
            || s->n_vertices > 512
            || s->floor > 6)
        {
            continue;
        }
        uint32_t crossingz = 0;
        for(uint32_t j = 0; j < s->n_vertices; j++)
        {
            v2 
                *a = s->vertices[j], 
                *b = j == s->n_vertices - 1 ? s->vertices[0] : s->vertices[j + 1];
            if(!a || !b)
                continue;
            if(
                (a->y > g_->p.pos.y) != (b->y > g_->p.pos.y)
                && (g_->p.pos.x < (b->x - a->x) * (g_->p.pos.y - a->y) / (b->y - a->y) + a->x ))
            {
               crossingz++; 
            }
        }
        if(crossingz % 2 == 1)
        {
            g_->p._sect = (s);
        }
    }
}


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
            
            //modifiers (Ctrl, Shift)
            SDL_Keymod d_mods = SDL_GetModState();
            if (d_mods & KMOD_CTRL) {
                g_->p._crawl = true;
            }
            if (d_mods & KMOD_SHIFT) {
                g_->p._crouch = true;
            }
            break;
        case SDL_KEYUP:
            if(event.key.keysym.sym >= 'a' && event.key.keysym.sym <= 'z')
            {
                g_->pressed_keys[event.key.keysym.sym - 'a'] = 0; 
            }
            //modifiers (Ctrl, Shift)
            SDL_Keymod mods = SDL_GetModState();
            if (mods & KMOD_CTRL) {
                g_->p._crawl = false;
            }
            if (mods & KMOD_SHIFT) {
                g_->p._crouch = false;
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
                // g_->p.pos.y += 0.4f * d_y;
                // g_->p.pos.x += 0.4f * d_x;
                const v2 mov = (v2){
                    0.4f * d_x,
                    0.4f * d_y
                };
                player_collision(g_, mov);
                player_sector_pos(g_);
            }
            // a
            if(i == 0)
                g_->p.rotation -= 2.5f;
            // e
            if(i == 4)
                g_->p.rotation += 2.5f;
        }
    } 
    if(g_->p.pos.x < 0) g_->p.pos.x = 0;
    if(g_->p.pos.y < 0) g_->p.pos.y = 0;
    if(g_->p.pos.x >= (float)(WINDOW_WIDTH - 6)) g_->p.pos.x = WINDOW_WIDTH - 6.0f;
    if(g_->p.pos.y >= (float)(WINDOW_HEIGHT - 6)) g_->p.pos.y = WINDOW_HEIGHT - 6.0f;
    if(g_->p.rotation > 360.0f) g_->p.rotation = 0.0f;
    if(g_->p.rotation < -360.0f) g_->p.rotation = 0.0f;
    if(g_->p._crawl)
        g_->p.height = 0;
    if (g_->p._crouch)
        g_->p.height = 4;
}



static inline void set_pixel_color(game_state *g_, int x, int y, int c)
{
    // bounds
    if((unsigned int)(x) >= WINDOW_WIDTH || (unsigned int)(y) >= WINDOW_HEIGHT)
        return;
    const int 
        ix = (WINDOW_WIDTH * y) + x;
    if (ix >= WINDOW_WIDTH * WINDOW_HEIGHT || ix < 0)
    {
        return ;
    }
    g_->buffer[ix] = (c);
}

// bresenham's algorithm
static void d_line(game_state *g_, int x0, int y0, int x1, int y1, uint32_t color)
{
    const int dx = abs(x1 - x0);
    const int dy = abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1)
    {
        set_pixel_color(g_, x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break; // complete line
        }

        const int e2 = err * 2;
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
    if(y1 < y0 || x < 0 || x > WINDOW_WIDTH)
        return;
    if(y0 < 0) 
        y0 = 0;
    if(y1 >= WINDOW_HEIGHT)
        y1 = WINDOW_HEIGHT - 1;
    for(uint32_t i = y0; i < y1; i++)
    {
        set_pixel_color(g_, x, i, color);
    }
}

static void d_rect(game_state *g_, int x, int y, int width, int height, int color)
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
            color == 0 ? 0xFFFFFF : color 
        );
    }
}


// calculate intersection of two segments
// https://en.wikipedia.org/wiki/Line-line_intersection
// (return NAN, NAN) if no intersection
static inline v2 intersect_segments(v2 a0, v2 a1, v2 b0, v2 b1) {
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y))
            - ((a0.y - a1.y) * (b0.x - b1.x));

    if (fabsf(d) < 0.000001f) { 
        return (v2) { NAN, NAN }; 
    }

    const f32
        t = (((a0.x - b0.x) * (b0.y - b1.y))
                - ((a0.y - b0.y) * (b0.x - b1.x))) / d,
        u = (((a0.x - b0.x) * (a0.y - a1.y))
                - ((a0.y - b0.y) * (a0.x - a1.x))) / d;
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1) ?
        ((v2) {
            a0.x + (t * (a1.x - a0.x)),
            a0.y + (t * (a1.y - a0.y)) })
        : ((v2) { NAN, NAN });
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
static inline int screen_angle_to_x(f32 angle) 
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
    // translate
    const v2 t = (v2){
        p.x - g_->p.pos.x, 
        p.y - g_->p.pos.y
    };
    // rotate
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

static inline uint32_t abgr_mul(uint32_t col, uint32_t a)
{ 
    const uint32_t 
        br = ((col & 0xFF00FF) * a) >> 8, 
        g = ((col & 0x00FF00) * a) >> 8; 

    return 0xFF000000 | (br & 0xFF00FF) | (g & 0x00FF00); 
}


// 5. [SOON ? ] Z-buffering (track depth of pixel to avoid rendering useless surfaces)
static void render_doomnukem(game_state *g_)
{
    if(!g_ || !g_->p._sect 
        || g_->scene._sectors_count <= 1)
    {
        return ;
    }
    // reset and init
    memset(g_->scene.visited, 0, sizeof(g_->scene.visited));
    queue_entry stack[MAX_QUEUE];
    uint32_t sp = 0;
    for (uint32_t i = 0; i < WINDOW_WIDTH; i++)
    {
        g_->scene.y_hi[i] = WINDOW_HEIGHT - 1;
        g_->scene.y_lo[i] = 0;
    }


    // push player_sector with full screen
    stack[sp++] = (queue_entry){ g_->p._sect, 0, WINDOW_WIDTH - 1};
    // while queue-stack isnt empty
    while(sp > 0)
    {
        const queue_entry entry = stack[--sp];
        const sector 
                *curr_sect = entry.s;
        
        if(!curr_sect 
             || g_->scene.visited[curr_sect->id]
             || curr_sect -> n_walls <= 2)
        {
            continue;
        }

        if(curr_sect == g_->p._sect)
            g_->scene.visited[curr_sect->id] = (1);

        // render sector
        const v2
                zdr = rotate_v2((v2){ 0.0f, 1.0f}, + (HFOV / 2.0f)),
                zdl = rotate_v2((v2){ 0.0f, 1.0f}, - (HFOV / 2.0f)),
                znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR }, // z-near left
                znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR }, // z-near right
                zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR }, // z-far left
                zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR }; // z-far right
        // each wall
        // (only walls facing the player gets rendered)
        for(uint32_t i = 0; i < curr_sect->n_walls; i++)
        {
              const wall 
                *l = /* (g_->scene._walls_queue[
                        i
                        (g_->scene._w_queue[i])
                    ]) */
                    (curr_sect->walls[i])
                ;
            if(!l || !l->a || !l->b)
                continue ;
   
            // linepos from [world-space] to [camera-space]
            const v2 
                op0 = (v2)(world_to_camera(g_, *l->a)),
                op1 = (v2)(world_to_camera(g_, *l->b));
           
            // compute cliped positions
            v2 cp0 = op0, cp1 = op1;

            // both are behind player -> wall behind camera
            if( (cp0.y <= 0 && cp1.y <= 0)
                || (cp0.y < ZNEAR && cp1.y < ZNEAR) )
            {
                continue;
            }

            // angle-clip against view frustum
            f32
                ac0 = normalize_angle(-(atan2(cp0.y, cp0.x) - PI_2)),
                ac1 = normalize_angle(-(atan2(cp1.y, cp1.x) - PI_2));

            /* 
            bool below_ = true;
            const v2  
                wall_normal = (v2){
                    (l->b->x - l->a->x),
                    (l->b->y - l->a->y)
                },
                p_to_wall = (v2){
                    g_->p.pos.x - l->a->x,
                    g_->p.pos.y - l->a->y
                };
            const f32
                cross = wall_normal.x * p_to_wall.y - wall_normal.y * p_to_wall.x;
            if(cross < 0.0f) // doesnt face each other
                below_ = false;
            if(cross == 0.0f)
                continue ;
            */

            // clip against frustum (if wall intersects with clip planes)
            if(  
                (cp0.y < ZNEAR)
                || (cp1.y < ZNEAR)
                || (ac0 > +(HFOV / 2)) // 1
                // || (ac0 < -(HFOV / 2)) // 2
                || (ac1 < -(HFOV / 2)) // 1
                // || (ac1 > +(HFOV / 2)) // 2
            ){ 
                // know where the intersection is on the (-HFOV/2 or HFOV/2) line delimiting player's fov
                v2 
                    left = intersect_segments(cp0, cp1, znl, zfl),
                    right = intersect_segments(cp0, cp1, znr, zfr);
                if((
                    !(ac0 > +(HFOV / 2) || ac1 < -(HFOV / 2))
                ) && (
                    ac0 < -(HFOV / 2) || ac1 > +(HFOV / 2) 
                )
                ){
                    //printf("SORTIE-GAUCHE \n");
                }else
                {
                    // printf("SORTIE-DROITE \n");
                } 

                // recompute angles ac0, ac1         
                if(!isnan(left.x)){
                    cp0 = left;
                    ac0 = normalize_angle( - (atan2(cp0.y, cp0.x) - PI_2));
                }
                if(!isnan(right.x)){
                    cp1 = right;
                    ac1 = normalize_angle(- (atan2(cp1.y, cp1.x) - PI_2));    
                }
            }

            // wrong side of the wall
            if( (ac0 < ac1) )
            {
                continue;
            }            

            // ignore far -HFOV/2..+HFOV/2
            // (both angles are entirely far of bounds)
            if( (ac0 < -(HFOV / 2.0f) && ac1 < -(HFOV / 2.0f))
                || (ac0 > +(HFOV / 2.0f) && ac1 > +(HFOV/ 2.0f)) 
            ){
                continue;
            }
            
            // wall true x's (before portal clamp)
            const int
                    wx0 = screenclamp (
                            screen_angle_to_x(ac0)
                        ),
                    wx1 = screenclamp (
                            screen_angle_to_x(ac1)
                        );
            
            // bounds check against portal window
            if (wx0 > entry.x2) 
            { continue; }
            if (wx1 < entry.x1) 
            { continue; }

            // stick to entry x-boundaries
            const int 
                x0 = clamp(wx0, entry.x1, entry.x2),
                x1 = clamp(wx1, entry.x1, entry.x2);

            // degenerate
            if (x1 <= x0
                || wx0 == wx1)
            {
                continue ;
            }


            // wall floor && ceiling
            const f32 
                z_floor = curr_sect->floor - g_->p.height,
                z_ceiling = curr_sect->ceil - g_->p.height,
                z_portal_floor = l->_op_sect ? (l->_op_sect->floor - g_->p.height) : (0),
                z_portal_ceiling = l->_op_sect ? (l->_op_sect->ceil - g_->p.height) : (0); 

            // impossible
            if(z_floor >= z_ceiling){
                continue;
            }

            // wall y'scale
            const f32 
                sy0 = ifnan((float)(VFOV * WINDOW_HEIGHT) / cp0.y, 1e10);
            const f32
                sy1 = ifnan((float)(VFOV * WINDOW_HEIGHT) / cp1.y, 1e10);


            // wall computed left(y's) -> right(y's)
            const int 
                    // floors
                    y_f0 = (WINDOW_HEIGHT / 2) - (int) (z_floor * (sy0)),
                    y_f1 = (WINDOW_HEIGHT / 2) - (int) (z_floor * (sy1)),
                    // ceilings
                    y_c0 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * (sy0)),
                    y_c1 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * (sy1)),
                    // portal_floors
                    p_f0 = (WINDOW_HEIGHT / 2) - (int) ((z_portal_floor) * sy0),
                    p_f1 = (WINDOW_HEIGHT / 2) - (int) (z_portal_floor * sy1),
                    // portal_ceilings
                    p_c0 = (WINDOW_HEIGHT / 2) - (int) (z_portal_ceiling * sy0),
                    p_c1 = (WINDOW_HEIGHT / 2) - (int) (z_portal_ceiling * sy1);
 
            const int wallshade =
                (16) * (sin (
                    atan2f (
                        l->b->x - l->a->x,
                        l->b->y - l->b->y)
                ) + 1.0f);

            for(int x = x0; x <= x1; x ++)
            {
                // calculate progress along x-axis
                // via wx{0,1} (true walls x's bfore portal clamping) 
                // so that walls cut off due to portal edges have proper heights
                const int shade = ((!l->_op_sect) && (x == x0 || x == x1)) 
                        ? 192 : (255 - wallshade);
                const f32 
                        xp = ifnan((x - wx0) / (f32)(wx1 - wx0), 0);
                const int 
                        bot = y_f0 + (y_f1 - y_f0) 
                            * (xp),
                        top = y_c0 + (y_c1 - y_c0) 
                            * (xp),
                        c_bot = clamp(bot, g_->scene.y_lo[x], g_->scene.y_hi[x]),
                        c_top = clamp(top, g_->scene.y_lo[x], g_->scene.y_hi[x]);

                if(c_bot <= 0 && c_top <= 0)
                    continue;
                // floor
                //
                //
                for(int y = bot; y < WINDOW_HEIGHT; y++)
                {
                    /* float dy = y - (WINDOW_HEIGHT / 2);
                    float row_dist = g_->p.height / dy;

                    // ray direction for this x
                    float angle = angle_for_screen_x(g_, x);
                    float dx = cosf(angle), dy = sinf(angle);

                    float world_x = g_->p.x + row_dist * dx;
                    float world_y = g_->p.y + row_dist * dy; */

                    // floor shade or texture
                    uint32_t floor_color = 0x0000FF; // or sample_floor(world_x, world_y);
                    // set_pixel_color(g_, x, y, floor_color);
                }
                // ceiling
                //
                //
                for(int y = 0; y < top; y++)
                {
                    /* float dy = y - (WINDOW_HEIGHT / 2);
                    float row_dist = g_->p.height / dy;

                    // ray direction for this x
                    float angle = angle_for_screen_x(g_, x);
                    float dx = cosf(angle), dy = sinf(angle);

                    float world_x = g_->p.x + row_dist * dx;
                    float world_y = g_->p.y + row_dist * dy; */

                    // floor shade or texture
                    uint32_t floor_color = 0xFF0000; // or sample_floor(world_x, world_y);
                    // set_pixel_color(g_, x, y, floor_color);
                }


                // walls with verlines
                // non-portal
                if(!l->_op_sect) 
                {
                    vert_line(g_, 
                        (x), 
                        c_top,
                        c_bot,
                        abgr_mul(0xFFD0D0D0, shade)
                    );
                }
                // portal
                else 
                {
                    // outlines
                    set_pixel_color(g_, (x), c_top, 0x444444);
                    set_pixel_color(g_, (x), c_bot, 0x555555);
                    const int 
                            portl_top = p_c0 + (p_c1 - p_c0) 
                                * (xp),
                            portl_bot = p_f0 + (p_f1 - p_f0) 
                                * (xp),
                            c_portl_top = clamp(portl_top, g_->scene.y_lo[x], g_->scene.y_hi[x]),
                            c_portl_bot = clamp(portl_bot, g_->scene.y_lo[x], g_->scene.y_hi[x]);
                    // top wall
                    if(l->_op_sect->ceil < curr_sect->ceil)
                    {
                        vert_line(g_, 
                            (x), 
                            c_top,
                            portl_top,
                            abgr_mul(0xFFD0D0D0, shade)
                        );
                        set_pixel_color(g_, (x), c_portl_top, 0xAAAAAA);
                    }
                    // bottom wall
                    if(l->_op_sect->floor > curr_sect->floor)
                    {
                        vert_line(g_, 
                            (x), 
                            portl_bot,
                            c_bot,
                            abgr_mul(0x777777, shade)
                        );
                        set_pixel_color(g_, (x), c_portl_bot, 0xAAAAAA);
                    }

                    g_->scene.y_lo[x] =
                        clamp(
                            max(max(c_top, c_portl_top), g_->scene.y_lo[x]),
                            0, WINDOW_HEIGHT - 1);
                    g_->scene.y_hi[x] =
                        clamp(
                            min(min(c_bot, c_portl_bot), g_->scene.y_hi[x]),
                            0, WINDOW_HEIGHT - 1); 
                    
                }
            }
   
            // A-B rects
            //d_rect(g_, wx1 - 2, y_c1 - 13, 4, 4, 0x00FF00);
      
            // bottom-top-left-right
            /*
            d_line(g_, x0, y_f0, x1, y_f1, 0xFFFFFF);
            d_line(g_, x0, y_c0, x1, y_c1, 0xFFFFFF);
            d_line(g_, x0, y_f0, x0, y_c0, 0xFFFFFF);
            d_line(g_, x1, y_f1, x1, y_c1, 0xFFFFFF);
            */

            // push wall(portal) to queue
            if(l->_op_sect)
            {
                if( //!g_->scene.visited[l->_op_sect->id] && 
                    sp < MAX_QUEUE)
                {
                    stack[sp++] = (queue_entry){ 
                        l->_op_sect,
                        x0,
                        x1 
                    };
                }
            }
        }
    }
}









static void draw_bsp(game_state *g_, BSP_node* root, int x, int y, int spacing, int depth)
{
    if(!root)
        return;
    if(spacing < 7)
        spacing = 7;

    if(root->_back)
    {
        d_line(g_, x, y, x - spacing, y + 10, 0xFFFFFF);
        draw_bsp(g_, root->_back, x - spacing, y + 10, spacing / 1.5, depth + 1);
    }

    if(root->_front)
    {
        d_line(g_, x, y, x + spacing, y + 10, 0xFFFFFF);
        draw_bsp(g_, root->_front, x + spacing, y + 10, spacing / 1.5, depth + 1);
    }
    if(root == g_->p._node)
        d_rect(g_, x - 2, y - 2, 4, 4, 0xFF00FF);
    else
        d_rect(g_, x - 2, y - 2, 4, 4, 0xFF0000);
}
// [MINIMAP]
static void render_minimap(game_state *g_)
{
    // grid
    for(uint16_t i = 0; i < WINDOW_WIDTH -1; i += MAP_TILE) // vert
        d_line(g_, i * MAP_ZOOM, 0, i * MAP_ZOOM, WINDOW_HEIGHT, 0x005500);
    for(uint16_t j = 0; j < WINDOW_HEIGHT; j += MAP_TILE) // horz
        d_line(g_, 0, j * MAP_ZOOM, WINDOW_WIDTH, j * MAP_ZOOM, 0x005500);
    // player fov lines 
    for(uint16_t i = 0; i < FOV; i ++)
    {
        d_line(g_, (g_->p.pos.x + 3) * MAP_ZOOM, (g_->p.pos.y + 3) * MAP_ZOOM,  
            MAP_ZOOM * (g_->p.pos.x + (200.0f * cos(DEG2RAD((float)((g_->p.rotation - (FOV / 2)) + i))))),
            MAP_ZOOM * (g_->p.pos.y + (200.0f * sin(DEG2RAD((float)((g_->p.rotation - (FOV/2)) + i))))),
            i == FOV / 2 ? 0xFFFF00 : 0x333333
        );        
    }
    // player cube 
    d_rect(g_, g_->p.pos.x * MAP_ZOOM, g_->p.pos.y * MAP_ZOOM, 5, 5, 0xFFFF00);
    // sectors
    if(g_->scene._sectors_count > 1)
    {
        for(int za = 0; za < 2; za++)
        {
            for(uint16_t i = 0; i < g_->scene._walls_ix; i++)
            {
                wall *l = &(g_->scene._walls[i]);
                const v2
                        *a = l->a,//g_->scene._sectors[i].vertices[j],
                        *b = l->b;// g_->scene._sectors[i].vertices[
                if(za == 1)
                {
                    if(g_->scene.visited[g_->scene._walls[i]._sector->id] == 1)
                        d_line(g_, (int)(a->x * MAP_ZOOM), (int)(a->y * MAP_ZOOM), (int)(b->x * MAP_ZOOM), (int)(b->y * MAP_ZOOM), 
                               (l->_op_sect ) ? 0xAAAA00 : 0xFFFF00);
                }
                else{
                    d_line(g_, (int)(a->x * MAP_ZOOM), (int)(a->y * MAP_ZOOM), (int)(b->x * MAP_ZOOM), (int)(b->y * MAP_ZOOM),
                        ( l->_op_sect ) ? 0x0000FF : 0xFFFFFF);
                }
            }
        }
    }

    // vertices
    if(g_->scene._verts_count > 1)
    {
        for(uint16_t i = 0; i < g_->scene._verts_count; i ++)
        {
            v2 a = g_->scene._verts[i];
            a.x *= MAP_ZOOM; a.y *= MAP_ZOOM;
            const int T = 4;
            int color = 0x00FF00;
            d_line(g_, (int)(a.x - T), (int)(a.y - T), (int)(a.x + T), (int)(a.y - T), color);
            d_line(g_, (int)(a.x - T), (int)(a.y + T), (int)(a.x + T), (int)(a.y + T), color);
            d_line(g_, (int)(a.x - T), (int)(a.y - T), (int)(a.x - T), (int)(a.y + T), color);
            d_line(g_, (int)(a.x + T), (int)(a.y - T), (int)(a.x + T), (int)(a.y + T), color); 
        }
    }
    /*if(g_->scene.bsp_head->_front 
        || g_->scene.bsp_head->_back)
    {
        draw_bsp(g_, g_->scene.bsp_head, WINDOW_WIDTH / 2, 450, 120, 0);
    }
    */
}




// print bsp tree
static void printBSP(BSP_node *node, int depth)
{
    if (!node) 
        return;
    for (int i = 0; i < depth; i++)
        printf("\033[34m│   ");
    if (node->_partition) {
        printf("├── \033[36m [NODE] Partition: [A](%.1f, %.1f) → [B](%.1f, %.1f) \n",
              (node->_partition)->a->x, (node->_partition)->a->y,
                (node->_partition)->b->x, (node->_partition)->b->y);
    } else {
        printf("\033[35m └── [LEAF] \033[0m \n");
        return;
    }
    printBSP(node->_front, depth + 1);
    printBSP(node->_back, depth + 1);
}




// txt to data
static void parse_txt(game_state *g_, const char *f)
{
    const int read_len = 2000;
    char bfr[read_len];
    if(!f || !g_)
        return ;
    int fd = open(f, O_RDONLY);

    if(fd < 0)
    {
        perror("Error opening the file \n");
        return ;
    }
    // read until EOF
    size_t bytes;
    while((bytes = read(fd, bfr, sizeof(bfr) - 1)) > 0)
    {
        if (bytes == -1) {
            perror("Error reading the file");
            close(fd);
            return ;
        }
        bfr[bytes] = '\0';

        // each \t ' ' \n
        char line[32 + 1];  // Make sure the buffer is large enough for 32 characters + null terminator
        char v_line[60 + 1];
        int read_n = 0, stored_n = 0;
        while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len)
        {
            line[read_n] = '\0';
            // reached end of file
            if(stored_n >= bytes){
                break;
            }
            // VERTICES
            if(!strncmp(line, "vertex", 6))
            {
                stored_n += 8;
                float y = -1.0f;
                while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len) 
                { 
                    line[read_n] = '\0';
                    if(y == -1)
                    {
                        y = atof(line); 
                    }
                    else
                    {
                        const float x = atof(line); 
                        const uint16_t c = g_->scene._verts_count;
                        g_->scene._verts = (v2 *)realloc(g_->scene._verts, sizeof(v2) * (c + 1));
                        if(!g_->scene._verts)
                            return;
                        g_->scene._verts[c] = (v2){
                            x,
                            y 
                        };
                        g_->scene._verts_count ++;
                    }
                    
                    if(*(bfr + stored_n + (read_n)) == '\n')
                        break;                       
                    stored_n += read_n;
                }
            }
            // SECTORS
            else if(!strncmp(line, "sector", 6))
            {
                int 
                    c = 0, 
                    b = 0;  
                const int 
                    k = g_->scene._sectors_count;
                stored_n += 8;
                g_->scene._sectors = (sector *) realloc(
                        g_->scene._sectors, 
                        sizeof(sector) * (k + 1)
                    );
                if (!g_->scene._sectors)
                    return ;

                g_->scene._sectors[k].portals = (sector **)calloc(sizeof(sector *), 1);
                g_->scene._sectors[k].n_portals = 0;
                g_->scene._sectors[k].n_vertices = 0;
                g_->scene._sectors[k].id = (k); // <- lowky useful

                while(sscanf(bfr + stored_n, "%50s%n", v_line, &read_n) && stored_n < read_len) 
                { 
                    v_line[read_n] = '\0';
                    // sect->floor && ceil
                    if(c == 0){
                        g_->scene._sectors[k].floor = (float)(atoi(v_line));
                    }
                    else if (c == 1){
                        g_->scene._sectors[k].ceil = (float)(atoi(v_line));
                    }
                    else
                    {
                        if(!b) // register sect->vertices
                        {
                            const int 
                                v_ = g_->scene._sectors[k].n_vertices,
                                ix = atoi(v_line);
                            if(!(ix < 0 || ix > g_->scene._verts_count))
                            { 
                                g_->scene._sectors[k].vertices[v_] = (v2 *)(&(g_->scene._verts[ix]));
                                g_->scene._sectors[k].n_vertices = (v_ + 1);
                            }
                        }
                        else// register sect->portals    
                        { 
                            const int 
                                    a = atoi(v_line),
                                    ix = g_->scene._sectors[k].n_portals;
                            g_->scene._sectors[k].portals = (sector **)(
                                realloc(g_->scene._sectors[k].portals, sizeof(sector *) * (ix + 1))
                            );
                            g_->scene._sectors[k].portals[ix] = (sector *) (malloc(sizeof(sector)));
                            if(!g_->scene._sectors[k].portals 
                                || ! g_->scene._sectors[k].portals[ix])
                                return ;
                            if(a < 0){ // 'x'
                            g_->scene._sectors[k].portals[ix]->id = (-1);
                            }else// 'number [0-9]'
                            {
                                (g_->scene._sectors[k].portals[ix])->id = (a);
                            }
                            g_->scene._sectors[k].n_portals ++; 
                        }
                        if(*(bfr + stored_n + (read_n)) == '\t')
                        {
                            b = 1; 
                        }
                    }
                    if(*(bfr + stored_n + (read_n)) == '\n')
                    { 
                        g_->scene._sectors_count++;
                        // stored_n += read_n;
                        break;
                    }               
                    stored_n += read_n;
                    c++;
                }
            }
            else
            { 
                stored_n += (read_n);
            }
        }
    }
    close(fd);
    printf("\033\[32mSUCCESS LOADING '%s'  [%d vertices, %d sectors].\033\[0m \n", f, g_->scene._verts_count, g_->scene._sectors_count);
    // scale vertices to map_grid
    for(uint16_t i = 0; i < g_->scene._verts_count; i++)
    {
        g_->scene._verts[i].x += 2;
        g_->scene._verts[i].y += 2;

        g_->scene._verts[i].x *= MAP_TILE;
        g_->scene._verts[i].y *= MAP_TILE;
    }
    // each sector
    for(uint16_t i = 0; i < g_->scene._sectors_count; i++)
    {
        if(g_->scene._sectors[i].n_vertices < 2 
            || !(&g_->scene._sectors[i])
            || g_->scene._sectors[i].n_vertices > 256
        )
        {
            continue;
        }
        printf("\033[35msector[%d]  %.0f %.0f    ", i, g_->scene._sectors[i].floor, g_->scene._sectors[i].ceil);
        g_->scene._sectors[i].n_walls = 0;
        int saved_ix = g_->scene._sectors[i].portals[0]->id;
        for(int p = 0; p < g_->scene._sectors[i].n_portals - 1; p++)
        {
            g_->scene._sectors[i].portals[p]->id = g_->scene._sectors[i].portals[p + 1]->id;
        }
        g_->scene._sectors[i].portals[g_->scene._sectors[i].n_portals - 1]->id = saved_ix;
        // each vertices
        float signed_area = 0.0f;
        for(uint16_t j = 0; j < g_->scene._sectors[i].n_vertices; j++)
        {
            if(g_->scene._sectors[i].n_vertices <= 1)
                continue ;
            v2 
                *a = g_->scene._sectors[i].vertices[j],
                *b = (j == g_->scene._sectors[i].n_vertices -1) ? 
                        (g_->scene._sectors[i].vertices[0])
                        :   
                        (g_->scene._sectors[i].vertices[j + 1]);
            signed_area += (b->x - a->x) * (b->y + a->y);

            int d = 0;
            while(d < g_->scene._verts_count && (&g_->scene._verts[d] != (v2 *)(a)))
                d++;
            printf("%d ", d);    
            
            const sector
                *s = &(g_->scene._sectors[i]);
            if( !g_->scene._walls )
            {
                fprintf(stderr, "realloc failed\n");
                return ;
            }
            // craft new wall !
            wall *l = &(g_->scene._walls[g_->scene._walls_ix]);
            // .verts [A]-[B]
            l->a = (a);
            l->b = (b);
            // ._sector
            l->_sector = (sector *)(s);
            l->_op_sect = NULL;
            // wall portal-sector
            if(g_->scene._sectors[i].portals[j]->id >= 0
                && g_->scene._sectors[i].portals[j]->id < g_->scene._sectors_count)
            {
                const int ix = g_->scene._sectors[i].portals[j]->id;
                l->_op_sect = (sector *)((&g_->scene._sectors[ix != -1 ? ix + 1 : ix]));
            }
             
            if (g_->scene._sectors[i].n_walls >= 256) {
                exit(EXIT_FAILURE);
            }
            // sector wall
            if(l)
            {
                g_->scene._sectors[i].walls[
                    g_->scene._sectors[i].n_walls
                ] = (l);
            }
            g_->scene._walls_ix++;
            g_->scene._sectors[i].n_walls++;
        }
        printf("    ");
        // each portals ??
        for(uint16_t j = 0; j < g_->scene._sectors[i].n_portals; j++)
        {
            if(!g_->scene._sectors[i].portals[j])
                printf("x ");
            else
                printf("%d ",(g_->scene._sectors[i].portals[j])->id);
        }
        if(signed_area > 0)
            printf(" ❌");
        else 
            printf("✅");
        printf("\033[0m\n");
    } 
    // assign player sector
    if(g_->scene._sectors_count > 2)
    {
        g_->p._sect = &g_->scene._sectors[1];
    }
}










// detect nearest wall from a wall (below or before, depends on side)
static wall *heuristic_bsp(const game_state *g_, wall *tracked_wall, wall *_region, unsigned int _region_len)
{ 
    // all [linedefs/walls] of the map
    // each [linedef/wall] has :
    //  - linedefs in front
    //  - linedefs in back 
    //  - splits
    // [cost] is based on 2 factors
    // 1. Balance: How evenly it splits the front and back groups.
    // 2. Splits: How many linedefs get split into two.i
    if(g_->scene._walls_ix > 262000 || !_region || !_region_len) // quickly prevent stack overflows... 
    {
        return NULL;
    }
    int s_score = INT_MAX;
    wall *selected_wall = NULL;
    for(uint16_t i = 0; i < _region_len; i ++)
    {
        const wall *w = &(_region[i]); 
        if(w->_in_bsp
            || w == tracked_wall)
            continue;
        const v2 ac[2] = {
            *w->a,
            *w->b
        };
        // score format is : [linedefs in front | linedefs in back | splits]
        int scores[3] = {0, 0, 0};
        // re-loop each [linedefs/walls]
        for(uint16_t j = 0; j < _region_len; j++)
        {
            // either comparing to itself or wall alr in the bsp tree
            if(&(_region[j]) == w
                || (&_region[j]) == tracked_wall)
                continue;
            const wall 
                *j_w = &(_region[j]);
            f32
                j_eq[2] = {0.0f, 0.0f};
            // D = (x − P1.x) * (P2.y − P1.y)−(y − P1.y) * (P2.x−P1.x)
            // P1 and P2 are the [linedef/wall] endpoints
            // D > 0 → Point is in front.
            // D < 0 → Point is in back.
            // D = 0 → Point is exactly on the partition.
            for(uint16_t k = 0; k < 2; k++)
            {
                const f32 
                    eq = (ac[k].x - j_w->a->x) * (j_w->b->y - j_w->a->y)
                            - (ac[k].y - j_w->a->y) * (j_w->b->x - j_w->a->x);
                j_eq[k] = (eq);
            }
            // front
            if(j_eq[0] > 0 && j_eq[1] > 0){
                scores[0]++;
            }// back
            else if (j_eq[0] < 0 && j_eq[1] < 0){
                scores[1]++;
            }else{
                // if linedef lies exactly on partition (D = 0), assign it to front
                if(j_eq[0] == 0 && j_eq[1] == 0)
                    scores[0]++;
                else
                    scores[2]++;
            }
        } 
        const int score = (abs(
                scores[1] - scores[0]
            ) + 
            (scores[2] * 8)
        );
        if(score < s_score)
        {
            s_score = (score);
            selected_wall = (wall *)(w);
        } 
        if(score == s_score){ }
    }
    return (selected_wall);
}







static BSP_node *bsp_(game_state *g_, wall **_region, unsigned int _region_size)
{
    // let leafs exists
    //if(!_region || !_region_size)  return (NULL);
    return (NULL);
    BSP_node* node = (BSP_node*)malloc(sizeof(BSP_node));
    
    node->_partition = NULL;
    node->_front = NULL;
    node->_back = NULL;
    if(_region_size < 1){
        return (node); 
    }
    // allocate new region
    wall 
        **front_region = (wall **)malloc(sizeof(wall *) * (_region_size)),
        **back_region = (wall **)malloc(sizeof(wall *) * (_region_size));

    wall *_W = (_region[0]);
    // wall *_W = (heuristic_bsp(g_, , _region, _region_size));
    if(!front_region || !back_region)
        return (NULL);
    int
        front_len = 0,
        back_len = 0;
    // attribute [left/right] new splitted regions
    for(uint16_t i = 0; i < _region_size; i ++)
    {
        const wall * 
            l_def = (_region[i]);
        if(l_def == _W) // ofc bro
            continue;
        wall *A = (_W);
        wall *B = (wall *) (l_def);
        // 1. compute dir vector A
        v2 
            dir_A = { A->b->x - A->a->x, A->b->y - A->a->y },
            normal_A = { -dir_A.y, dir_A.x },
            vec_AB = { B->a->x - A->a->x, B->a->y - A->a->y };
        
        // 2. compute dot product (vec_AB * normal_A)
        const f32
            dot = vec_AB.x * normal_A.x + vec_AB.y * normal_A.y;
        
        // fill front_ (colinears go in front)
        if(dot >= 0.0f)
        {
            front_region[front_len] = ((wall *)(l_def));
            front_len++;
        }
        // fill back_
        if (dot < 0.0f) 
        {
            back_region[back_len] = ((wall *)(l_def));
            back_len++;
        }
    }
    node->_partition = (_W);
    if(!_W || g_->scene._bsp_depth > 400){
        return (node);
    }
    g_->scene._bsp_depth++;
    node->_front = bsp_(g_, front_region, front_len);
    node->_back = bsp_(g_, back_region, back_len);
    return (node);
}





static void init_bsp(game_state *g_)
{ 
    size_t n = g_->scene._walls_ix;
    if (g_->scene._walls_ix <= 0 || g_->scene._walls_ix > 512) {
        fprintf(stderr, "Invalid wall count: %d\n", g_->scene._walls_ix);
        return;
    }
    // INIT (BSP) -> SPLITTED REGIONS
    // ---------------------
    BSP_node* node = (BSP_node*)malloc(sizeof(BSP_node));
    wall *splitter = &(g_->scene._walls[g_->scene._walls_ix / 2]);
    // allocate new region
    wall 
        **front_region = (wall **)malloc(sizeof(wall *) * (n)),
        **back_region = (wall **)malloc(sizeof(wall *) * (n));
    if(!front_region || !back_region)
        return ;
    int
        front_len = 0,
        back_len = 0; 
    // initliaze whole left/right] new splitted regions
    for(uint16_t i = 0; i < g_->scene._walls_ix; i ++)
    {
        const wall * 
            l_def = &(g_->scene._walls[i]);
        if(l_def == splitter) // ofc bro
            continue;
        wall *A = (splitter);
        wall *B = (wall *) (l_def);
        // direction vector of A
        const v2 
            dir_A = { A->b->x - A->a->x, A->b->y - A->a->y },
            normal_A = { -dir_A.y, dir_A.x },
            v_ABa = { B->a->x - A->a->x, B->a->y - A->a->y },
            v_ABb = { B->b->x - A->a->x, B->b->y - A->a->y };
        // dot product of vec_AB with normal_A
        const f32
            dotA = v_ABa.x * normal_A.x + v_ABa.y * normal_A.y,
            dotB = v_ABb.x * normal_A.x + v_ABb.y * normal_A.y;
        
        // fill front_
        if( (dotA > 0.0f && dotB > 0.0f)
          || (dotA == 0.0F && dotB == 0.0f))
        {
            front_region[front_len] = ((wall *)(l_def));
            front_len++;
        }
        // fill back_
        if (dotA < 0.0f && dotB < 0.0f) 
        {
            back_region[back_len] = ((wall *)(l_def));
            back_len++;
        }
    }
    // start whole bsp tree
    g_->scene.bsp_head = (node);
    node->_partition = splitter;
    node->_back = bsp_(g_, back_region, back_len);
    node->_front = bsp_(g_, front_region, front_len);
}




static void destroy_doom(game_state *g_)
{
    SDL_DestroyTexture(g_->texture);
    SDL_DestroyRenderer(g_->renderer);
    SDL_DestroyWindow(g_->window);
    SDL_Quit();
 
    // scene memory
    free(g_->scene._verts);
    free(g_->scene._walls);
    free(g_->scene._sectors);

    free(g_->buffer);
    free(g_);
    
    return;
}



static int init_doom(game_state **g_)
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
    (*g_)->quit = false;
    (*g_)->p.pos.x = 15;
    (*g_)->p.pos.y = 15;
    (*g_)->p.rotation = 0;
    (*g_)->p.height = 7.0f;
    (*g_)->p.y_pos = 0.0f;
    (*g_)->render_mode = true;
    (*g_)->scene._walls_ix = 0;
    (*g_)->scene._walls = (wall *) malloc(512 * sizeof(wall));
    (*g_)->scene._verts = (v2 *)malloc(sizeof(v2));
    if(!(*g_)->scene._verts)
        return (-1);
    (*g_)->scene._verts[0] = (v2){
        0.0f,
        0.0f
    };
    (*g_)->scene._sectors = (sector *) malloc(sizeof(sector));
    if(!(*g_)->scene._sectors)
        return (-1);
    (*g_)->scene._verts_count = 1;
    (*g_)->scene._sectors_count = 1;
    memset((*g_)->scene.visited, 0, sizeof(((*g_)->scene.visited)) );
    // memset static arrays
    // (prevents irregular segfaults)
    for(uint16_t i = 0; i< (*g_)->scene._sectors_count; i++)
    {
        memset((*g_)->scene._sectors[i].walls, 0, sizeof(s_wall *));
        memset((*g_)->scene._sectors[i].vertices, 0, sizeof(v2 *));
    }
    size_t total = 0;

    // static structs
    total += sizeof(game_state);
    total += sizeof(t_scene);  // g_->scene is embedded
    total += sizeof(player);   // g_->p is embedded
    // dynamically allocated fields
    total += (*g_)->scene._verts_count * sizeof(v2);
    total += (*g_)->scene._walls_ix * sizeof(wall);
    total += (*g_)->scene._sectors_count * sizeof(sector);
    total += sizeof((*g_)->scene._queue);   // Static array of queue_entry
    total += sizeof((*g_)->scene.visited);  // Static array of char

    // scene sectors: count portals if malloc'ed
    for (uint32_t i = 0; i < (*g_)->scene._sectors_count; i++) {
        total += (*g_)->scene._sectors[i].n_portals * sizeof(sector *);
    }

    // rendering-buffer
    total += sizeof(uint32_t) * (WINDOW_WIDTH * WINDOW_HEIGHT /* your screen resolution */);

    printf("\033[32mSUCCESS init game_state (approx): %zu bytes (%.2f KB)\n", total, total / 1024.0);
    return 0;
}




static void update(game_state *g_)
{
    // reset buffer pixels to black
    memset(g_->buffer, 0x222222, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
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

    if(g_->render_mode)
    {    
        if(g_->p._sect){
            render_doomnukem(g_);
        }
    }
    else
    {
        render_minimap(g_);
    }
    //      -> multithread present(g_) <-
    // cpu pixel buffer => locked sdl texture 
    // [hardware acceleration?] => video memory alloction?
    void *px;
    int pitch;
    // lock texture before writing to it (SDL uses mutex and multithread for textures rendering)
    SDL_LockTexture(g_->texture, NULL, &px, &pitch); 
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



int main(int argc, char **argv)
{
    game_state  *g_ = NULL;

    if(init_doom(&g_) == -1)
    {
        destroy_doom(g_);
        return (EXIT_FAILURE);
    }

    // parse map
    parse_txt(g_, "maps/bruh.txt"); 
    init_bsp(g_);

    printf("\033[32mDOOM RUNNING [%dx%d] \033[0m \n", WINDOW_WIDTH, WINDOW_HEIGHT);


    // [game loop]
    while(!(g_->quit))
    {
        player_movement(g_); 
        update(g_);
    }
    destroy_doom(g_);
    return (EXIT_SUCCESS); 
}
