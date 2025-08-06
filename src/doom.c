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
                g_->p.pos.y += 2.0f * d_y;
                g_->p.pos.x += 2.0f * d_x;
                
            }
            // a
            if(i == 0)
                g_->p.rotation -= 1.75f;
            // e
            if(i == 4)
                g_->p.rotation += 1.75f;
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

static void player_sector_pos(game_state *g_)
{
    // each sect
    f32 last_area = FLT_MAX;

    for(uint32_t i = 0; i < g_->scene._sectors_count; i++)
    {
        sector *s = &g_->scene._sectors[i];
        if (!s || !s->vertices[0]
            || s->n_vertices <= 0 
            || s->n_vertices > 512)
            continue;
        uint32_t crossingz = 0;
        f32 area = 0.0f;
        if(s->n_vertices < 2 
            || !(s->floor <= g_->p.y_pos && g_->p.y_pos <= s->ceil) )
            continue;
        for(uint32_t j = 0; j < s->n_vertices; j++)
        {
            v2 
                *a = s->vertices[j], 
                *b = j == s->n_vertices - 1 ? s->vertices[0] : s->vertices[j + 1];
            if(!a || !b)
                continue;
            area += (a->x * b->y) - (b->x * a->y);
            if((a->y > g_->p.pos.y) != (b->y > g_->p.pos.y)
                && (g_->p.pos.x < (b->x - a->x) * (g_->p.pos.y - a->y) / (b->y - a->y) + a->x ))
            {
               crossingz++; 
            }
        }
        area = fabsf(area * 0.5f);
        if(crossingz % 2 == 1)
        {
            if(area < last_area)
            {
                last_area = area;
                g_->p._sect = (s);
            }
        }
    }
}


static inline void set_pixel_color(game_state *g_, int x, int y, int c)
{
    // segfaults
    if(x < 0)
        x = 0;
    if(x > WINDOW_WIDTH || y > WINDOW_HEIGHT || y < 0)
        return;
    const int ix = (WINDOW_WIDTH * y) + x;
    if (ix >= WINDOW_WIDTH * WINDOW_HEIGHT || ix < 0){
        // fprintf(stderr, "OOB pixel access at (%d,%d) -> ix=%d\n", x, y, ix);
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
    if(y0 < 0) y0 = 0;
    if(y1 > WINDOW_HEIGHT) y1 = WINDOW_HEIGHT;
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
/*
static inline v2 intersect_segments(v2 a0, v2 a1, v2 b0, v2 b1)
{
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y))
            - ((a0.y - a1.y) * (b0.x - b1.x));
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
*/
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



// 2D Coordinates to 3D
// 3. wall-Cliping (test if lines intersects with player viewcone/viewport by using vector2 cross product)
// 4. perspective transformation  (scaling lines according to their dst from player, for appearance of perspective (far-away objects are smaller).
// 5. [SOON ? ] Z-buffering (track depth of pixel to avoid rendering useless surfaces)
// 
static void render_doomnukem(game_state *g_)
{
    uint32_t 
        queue_head = 0,
        queue_tail = 0;
    memset(g_->scene.visited, 0, sizeof(g_->scene.visited));
    sector *s = g_->p._sect;
    if(!s)
        return ;

    // push sector into rendering-queue
    if (queue_tail >= MAX_QUEUE ) //||)  g_->visited[s->id]) 
        return;
    // visited[s->id] = 1;
    g_->scene._queue[queue_tail++] = (queue_entry){ s };

    // while queue isnt empty
    while(queue_head < queue_tail)
    {
        // dequeue a sector
        queue_entry curr = g_->scene._queue[queue_head++];
        const sector *curr_sect = curr.s;

        // sector rendering logic:
        // (push a sector in -> queue if he has any portal)
        // render sector
        for(uint64_t i = 0; i < curr_sect->n_walls; i++)
        {
            /*if(!&curr_sect->vertices[i])
                continue ;
            wall w = ((wall){
                curr_sect->vertices[i],
                (i == curr_sect->n_vertices - 1) 
                    ? curr_sect->vertices[i + 1] : curr_sect->vertices[0]
            });
            w = g_->scene._walls[0];*/
            

            const wall 
                *l = /* (g_->scene._walls_queue[
                        i
                        (g_->scene._w_queue[i])
                    ]) */
                    (curr_sect->walls[i])
                ;
            const v2
                zdr = rotate_v2((v2){ 0.0f, 1.0f}, + (HFOV / 2.0f)),
                zdl = rotate_v2((v2){ 0.0f, 1.0f}, - (HFOV / 2.0f)),
                znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR }, // z-near left
                znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR }, // z-near right
                zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR }, // z-far left
                zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR }; // z-far right

            // linepos from [world-space] to [camera-space]
            const v2 
                op0 = (v2)(world_to_camera(g_, *l->a)),
                op1 = (v2)(world_to_camera(g_, *l->b));
           
            // compute cliped positions
            v2 cp0 = op0, cp1 = op1;

            // both are behind player -> wall behind camera
            if(cp0.y <= 0 && cp1.y <= 0){
                continue;
            }

            // angle-clip against view frustum
            f32
                ac0 = normalize_angle(-(atan2(cp0.y, cp0.x) - PI_2)),
                ac1 = normalize_angle(-(atan2(cp1.y, cp1.x) - PI_2));

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
            {
                below_ = false;
            }
            if(cross == 0.0f)
            {
                continue ;
            }       
            // clip against frustum (if wall intersects with clip planes)
            if(  
                (cp0.y < ZNEAR)
                || (cp1.y < ZNEAR)
                || (ac0 > +(HFOV / 2)) // 1
                || (ac0 < -(HFOV / 2)) // 2
                || (ac1 < -(HFOV / 2)) // 1
                || (ac1 > +(HFOV / 2)) // 2
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
                    //printf("SORTIE-DROITE \n");
                } 

                // if we define dx=x2-x1 and dy=y2-y1, 
                // then normals are (-dy, dx) and (dy, -dx).
                if(!(below_)) // doesnt face each other
                { 
                    left = (right);
                    right = intersect_segments(cp0, cp1, znl, zfl);
                }
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
            if((ac0 < ac1 && below_)
                || (ac1 < ac0 && (!below_))
            )
            {
               continue;
            }
            // wall attributes
            f32 
                z_floor = curr_sect->floor + (g_->p.height), //(curr_sect->floor) + g_->p.height / 2;
                z_ceiling = curr_sect->ceil;// - (g_->p.height);// (curr_sect->ceil) + g_->p.height / 2; 

            // impossible (floor above ceiling)
            if(z_floor >= z_ceiling){
                continue;
            }

            // ignore far -HFOV/2..+HFOV/2
            // check if angles are entirely far of bounds
            if( (ac0 < -(HFOV / 2.0f) && ac1 < -(HFOV / 2.0f))
                || (ac0 > +(HFOV / 2.0f) && ac1 > +(HFOV/ 2.0f)) 
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
            const int // (inverted wy_b's and wy_top's)
                   wy_b0 = (WINDOW_HEIGHT / 2) + (int) (z_floor * sy0),
                   wy_b1 = (WINDOW_HEIGHT / 2) + (int) (z_floor * sy1),
                   wy_t0 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * sy0),
                   wy_t1 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * sy1);

            // wall sns
            const bool s = (wx1 > wx0);
            const int 
                x1 = s ? (wx0) : (wx1),
                x2 = s ? (wx1) : (wx0);

             // wall is behind or degenerate
            if (x2 <= x1) 
                return; 

            // portal ?
            if(!l->_op_sect) // non-portal
            {
                // wall with verlines
                for(uint16_t i = x1 + 0; i < x2; i++)
                {
                    const int 
                        y_a = wy_b0 + (wy_b1 - wy_b0) 
                            * (i - x1) / (x2 - x1),
                        y_b = wy_t0 + (wy_t1 - wy_t0) 
                            * (i - x1) / (x2 - x1);
                    vert_line(g_, 
                        s ? (i) : (x2 + (x1 - i)), 
                        y_b + 1,
                        y_a,
                        l->_op_sect ? 0x0000AA : 0xBBBBBB
                    );
                }
            }else // portal
            {
                // top wall
                if(l->_op_sect->ceil < curr_sect->ceil)
                {

                }
                // bottom wall
                if(l->_op_sect->floor > curr_sect->floor)
                {
                    // wall y's
                    const int
                            wy_t0 = WINDOW_HEIGHT / 2 - 300,
                            wy_t1 = WINDOW_HEIGHT / 2 - 300;                    
                    // wall with verlines
                    for(uint16_t i = x1 + 0; i < x2; i++)
                    {
                        const int 
                            y_a = wy_b0 + (wy_b1 - wy_b0) 
                                * (i - x1) / (x2 - x1),
                            y_b = wy_t0 + (wy_t1 - wy_t0) 
                                * (i - x1) / (x2 - x1);
                        vert_line(g_, 
                            s ? (i) : (x2 + (x1 - i)), 
                            y_b + 1,
                            y_a,
                            0x0000AA                        
                        );
                    }
                }
            }
            // wall-outines
            // bottom-top-left-right
            d_line(g_, wx0, wy_b0, wx1, wy_b1, 0xFFFFFF);
            d_line(g_, wx0, wy_t0, wx1, wy_t1, 0xFFFFFF);
            d_line(g_, wx0, wy_b0, wx0, wy_t0, 0xFFFFFF);
            d_line(g_, wx1, wy_b1, wx1, wy_t1, 0xFFFFFF); 

            // A-B rects
            //d_rect(g_, wx0 - 2, wy_t1 - 25, 4, 4, 0x00FFFF);
            d_rect(g_, wx1 - 2, wy_t1 - 13, 4, 4, 0x00FF00);

            // todo:
            // texture-mapping
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
    for(uint16_t i = 0; i < WINDOW_WIDTH -1; i += MAP_TILE)
    {
        d_line(g_, i, 0, i, WINDOW_HEIGHT, 0x005500);
        for(uint16_t j = 0; j < WINDOW_HEIGHT; j += MAP_TILE)
        {
            d_line(g_, i, j, WINDOW_WIDTH, j, 0x005500);
        }
    }
    // player square and line 
    for(uint16_t i = 0; i < FOV; i ++)
    {
        d_line(g_, g_->p.pos.x + 2, g_->p.pos.y - 2,  
            g_->p.pos.x + (200.0f * cos(DEG2RAD((float)((g_->p.rotation - (FOV / 2)) + i)))),
            g_->p.pos.y + (200.0f * sin(DEG2RAD((float)((g_->p.rotation - (FOV/2)) + i)))),
            0x333333
        );        
    }
    d_line(g_, g_->p.pos.x, g_->p.pos.y, 
       g_->p.pos.x + (200.0f * cos(DEG2RAD(g_->p.rotation))),
       g_->p.pos.y + (200.0f * sin(DEG2RAD(g_->p.rotation))),
       0xFFFF00
    );   
    d_rect(g_, g_->p.pos.x, g_->p.pos.y, 5, 5, 0xFFFF00); 
    // [World-view]
    // sectors
    if(g_->scene._sectors_count > 1)
    {
        for(int zz = 0; zz < 2; zz++)
        {
            for(uint16_t i = 0; i < g_->scene._walls_ix; i++)
            {
                wall *l = &(g_->scene._walls[i]);
                /* if(g_->scene._sectors[i].n_vertices <= 1)
                    continue;
                for(uint32_t j = 0; j < g_->scene._sectors[i].n_vertices ; j ++)
                { */ 
                    const v2
                        *a = l->a,//g_->scene._sectors[i].vertices[j],
                        *b = l->b;// g_->scene._sectors[i].vertices[
                          //  (j == g_->scene._sectors[i].n_vertices - 1) ? (0) : (j + 1)
                        //];
                    if(zz == 1 && ( l->_sector/* &g_->scene._sectors[i] */ == g_->p._sect))
                    {
                        d_line(g_, (int)(a->x), (int)(a->y), (int)(b->x), (int)(b->y),
                            (l->_op_sect) ? 0x0000FF : 0xFFFF00);
                    }else if (zz == 0)
                    {
                        d_line(g_, (int)(a->x), (int)(a->y), (int)(b->x), (int)(b->y),
                            (l->_op_sect) ? 0x888888 : 0xFFFFFF);
                    }
                /* } */
            }
        }
    }

    // vertices
    if(g_->scene._verts_count > 1)
    {
        for(uint16_t i = 0; i < g_->scene._verts_count; i ++)
        {
            const v2 a = g_->scene._verts[i];
            const int T = 4;
            int color = 0x00FF00;
            /* for(uint32_t i = 0; i < g_->p._sect->n_vertices; i ++)
                if(g_->p._sect->vertices[i] == &g_->scene._verts[i])
                    color = 0xFFFF00;
            */
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
                g_->scene._sectors[k].indx = (k); // <- lowky useful

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
                                // g_->scene._sectors[k].portals[ix] = NULL;
                                g_->scene._sectors[k].portals[ix]->indx = (-1);
                            }else// 'number [0-9]'
                            {
                                // g_->scene._sectors[k].portals[ix] = &(g_->scene._sectors[ix]);
                                (g_->scene._sectors[k].portals[ix])->indx = (a);
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
            continue;
        printf("\033[35msector[%d]  %.0f %.0f    ", i, g_->scene._sectors[i].floor, g_->scene._sectors[i].ceil);
        g_->scene._sectors[i].n_walls = 0;
        // each vertices
        // continue ;
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
            int d = 0;
            while(d < g_->scene._verts_count && (&g_->scene._verts[d] != (v2 *)(a)))
                d++;
            printf("%d ", d);    
            
            const sector
                *s = &(g_->scene._sectors[i]);
            if (g_->scene._walls == NULL)
            {
                g_->scene._walls = malloc(sizeof(wall) * 1);
            }else
            {
                g_->scene._walls = (wall *) realloc(
                        g_->scene._walls,
                        sizeof(wall) * (g_->scene._walls_ix + 1)
                );
            }
            if( !g_->scene._walls ){
                fprintf(stderr, "realloc failed\n");
                return ;
            }
            // craft new wall !
            wall *l = &(g_->scene._walls[g_->scene._walls_ix]);
            // wall verts [A]-[B]
            l->a = (a);
            l->b = (b);
            // wall sector
            l->_sector = (sector *)(s);
            l->_op_sect = NULL;
            // wall portal-sector
            if(g_->scene._sectors[i].portals[j]->indx >= 0
                && g_->scene._sectors[i].portals[j]->indx < g_->scene._sectors_count)
            {
                const int ix = g_->scene._sectors[i].portals[j]->indx;
                l->_op_sect = (sector *)((&g_->scene._sectors[ix]));
            }
             
            if (g_->scene._sectors[i].n_walls >= 256) {
                fprintf(stderr, "Too many walls in sector %d\n", i);
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
                printf("%d ",(g_->scene._sectors[i].portals[j])->indx);
        }
        printf("\033[0m\n");
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
    (*g_)->p.pos.x = WINDOW_WIDTH / 3;
    (*g_)->p.pos.y = WINDOW_HEIGHT / 3;
    (*g_)->p.rotation = 0;
    (*g_)->p.height = 7.0f;
    (*g_)->p.y_pos = 0.0f;
    (*g_)->render_mode = false;
    (*g_)->scene._walls_ix = 0;
    (*g_)->scene._walls = NULL;
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
        render_doomnukem(g_);
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
        player_sector_pos(g_);
        update(g_);
    }
    destroy_doom(g_);
    return (EXIT_SUCCESS); 
}
