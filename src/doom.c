#include "doom.h"

// rotate vector v by angle a (in radians)
static inline v2 rotate_v2(v2 v, f32 a)
{
    return (v2){
        (v.x * cos(a)) - (v.y * sin(a)),
        (v.x * sin(a)) + (v.y * cos(a))
    };
}
static void transform_world(game_state *g_)
{
    return;
    for(uint32_t i = 0; i < g_->scene._walls_ix; i ++)
    {     
        wall *l = &(g_->scene._walls[i]);

        (l->p_a).x = (l->a).x - g_->p.pos.x;
        (l->p_b).x = (l->b).x - g_->p.pos.x;
        (l->p_a).y = (l->a).y - g_->p.pos.y;
        (l->p_b).y = (l->b).y - g_->p.pos.y;
        rotate_v2( (l->p_a), DEG2RAD(g_->p.rotation));
        rotate_v2((l->p_b), DEG2RAD(g_->p.rotation));
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
                // transform_world(g_);
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

static inline void set_pixel_color(game_state *g_, int x, int y, int c)
{
    // prevents segfaults
    if(x < 0 || y < 0 || x > WINDOW_WIDTH || y > WINDOW_HEIGHT)
        return;
    int ix = (WINDOW_WIDTH * y) + x;
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
    for(uint16_t i = 0; i < width; i ++ )
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
*/
static inline v2 intersect_segments(v2 a0, v2 a1, v2 b0, v2 b1) {
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y))
            - ((a0.y - a1.y) * (b0.x - b1.x));

    if (fabsf(d) < 0.000001f) { return (v2) { NAN, NAN }; }

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

static f32 cross_product(v2 A, v2 B, v2 P) {
    float dx = B.x - A.x;  // x-component of the wall direction
    float dy = B.y - A.y;  // y-component of the wall direction
    float px = P.x - A.x;  // x-component of the player direction
    float py = P.y - A.y;  // y-component of the player direction
    return dx * py - dy * px;
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
                continue;
            }
        
            // angle-clip against view frustum
            f32
                ac0 = normalize_angle(-(atan2(cp0.y, cp0.x) - PI_2)),
                ac1 = normalize_angle(-(atan2(cp1.y, cp1.x) - PI_2));

            bool below_ = true;
            const v2 
                middle_wall = (v2){
                    (l->a.x + l->b.x) / 2.0f,
                    (l->a.y + l->b.y) / 2.0f
                },
                wall_normal = (v2){
                    (l->b.x - l->a.x),
                    (l->b.y - l->a.y)
                },
                p_to_wall = (v2){
                    g_->p.pos.x - l->a.x,
                    g_->p.pos.y - l->a.y
                };
            const f32
                cross = wall_normal.x * p_to_wall.y - wall_normal.y * p_to_wall.x;
            if(cross < 0.0f) // doesnt face each other
            {
                below_ = false;
            }
            if(cross == 0.0f)
            {
                continue;
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
                    /*
                    // left = (right);
                    if(ac0 < -(HFOV/2))
                        //printf("SORTIE-GAUCHE \n");
                    if(ac1 > +(HFOV/2))
                        //printf("SORTIE-DROITE \n");
                    */
                }else
                {
                    /*
                    if(ac0 > +(HFOV /2))
                        printf("SORTIE-GAUCHE \n");
                    if(ac1 < -(HFOV / 2))
                        printf("SORTIE-DROITE \n");
                    */
                } 
   
                // if we define dx=x2-x1 and dy=y2-y1, 
                // then the normals are (-dy, dx) and (dy, -dx).
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
            // todo: parse wad to get real values of wall floor and ceiling heights
            float z_floor = 12.0f;
            float z_ceiling = 0.0f;

            // ignore far -HFOV/2..+HFOV/2
            // check if angles are entirely far of bounds
            //printf("A:%d b:%d \n", (int)(radToDeg(ac0)), (int)(radToDeg(ac1)));
            if( (ac0 < -(HFOV / 2) && ac1 < -(HFOV / 2.0f))
                || (ac0 > +(HFOV / 2) && ac1 > +(HFOV/ 2.0f)) 
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
            d_line(g_, wx0, wy_b0, wx1, wy_b1, 0xFFFFFF);
            d_line(g_, wx0, wy_t1, wx1, wy_t1, 0xFFFFFF);
            d_line(g_, wx0, wy_b0, wx0, wy_t0, 0xFFFFFF);
            d_line(g_, wx1, wy_b1, wx1, wy_t1, 0xFFFFFF);

            // A-B rects
            d_rect(g_, wx0 - 2, wy_t1 - 25, 4, 4, 0x00FFFF);
            d_rect(g_, wx1 - 2, wy_t1 - 25, 4, 4, 0x00FF00);
         
            // wall-filled
            // with verlines
            bool s = (wx1 > wx0);
            const int 
                x1 = s ? (wx0) : (wx1),
                x2 = s ? (wx1) : (wx0);
            for(uint16_t i = x1 + 1; i < x2; i++)
            {
                const int 
                    y_a = wy_b0 + (wy_b1 - wy_b0) 
                        * (s ? (i - x1) : (i - x1)) / (x2 - x1),
                    y_b = wy_t0 + (wy_t1 - wy_t0) 
                        * (s ? (i - x1) : (i - x1)) / (x2 - x1);
                vert_line(g_, 
                    s ? (i) : (x2 + (x1 - i)), 
                    y_b + 1,
                    y_a, 
                l->color);
            } 

            // todo:
            // texture-mapping
        }
    }
    else
    {

      // player square and line 
        for(uint32_t i = 0; i < FOV; i ++)
        {
            d_line(g_, g_->p.pos.x + 2, g_->p.pos.y - 2,  
                g_->p.pos.x + (200.0f * cos(DEG2RAD((float)((g_->p.rotation - (FOV / 2)) + i)))),
                g_->p.pos.y + (200.0f * sin(DEG2RAD((float)((g_->p.rotation - (FOV/2)) + i)))),
                0xAAAAAA
            );        
        }
        d_line(g_, g_->p.pos.x, g_->p.pos.y, 
           g_->p.pos.x + (200.0f * cos(DEG2RAD(g_->p.rotation))),
           g_->p.pos.y + (200.0f * sin(DEG2RAD(g_->p.rotation))),
           0xFFFF00
        );   
        d_rect(g_, g_->p.pos.x, g_->p.pos.y, 5, 5, 0xFFFF00); 
        // world-view
        // walls
        for(uint32_t i = 0; i < g_->scene._walls_ix; i ++)
        {
            d_line(g_, 
                g_->scene._walls[i].a.x, g_->scene._walls[i].a.y, 
                g_->scene._walls[i].b.x, g_->scene._walls[i].b.y,
                g_->scene._walls[i].color
            );      
            // a
            d_rect(g_, g_->scene._walls[i].a.x - 2,  g_->scene._walls[i].a.y - 2, 4, 4, 0x00FFFF); 
            // b
            d_rect(g_, g_->scene._walls[i].b.x - 2,  g_->scene._walls[i].b.y - 2, 4, 4, 0x00FF00); 
        }
        // vertices
        /*
        if(g_->scene._verts_count > 1){
            for(uint32_t i = 0; i < g_->scene._verts_count; i ++)
            {
                d_rect(g_, 
                       g_->scene._verts[i].x * 10, g_->scene._verts[i].y * 10, 
                       2, 2, 
                0xFF0000);
            }
        }*/
    }
}

static void add_wall(game_state *g_, int x1, int y1, int x2, int y2, int color)
{
    wall *l = &(g_->scene._walls[g_->scene._walls_ix]);

    if(x1 > WINDOW_WIDTH) x1 = (WINDOW_WIDTH - 1);
    if(x1 < 0) x1 = 0;
    if(x2 < 0) x2 = 0;
    if(x2 > WINDOW_HEIGHT) x2 = (WINDOW_HEIGHT - 1);
    l->a.x = x1;  
    l->a.y = y1;   

    l->b.x = x2;
    l->b.y = y2;
    l->color = color;

    g_->scene._walls_ix++;
}

// txt to data
static void parse_txt(game_state *g_, const char *f)
{
    const int read_len = 100;
    char bfr[read_len];
    if(!f || !g_)
        return ;
    int fd = open(f, O_RDONLY);

    printf("LOADING %s file \n", f);
    if(!fd)
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
        char line[33];  // Make sure the buffer is large enough for 32 characters + null terminator
        int read_n = 0, stored_n = 0;
        while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len)
        {
            line[read_n] = '\0';
            // vertices
            if(!strncmp(line, "vertex", 6))
            {
                stored_n += 8;
                int y = -1;
                while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len) 
                { 
                    line[read_n] = '\0';
                    if(y == -1){
                        // printf("+y: %s \n", line);
                        y = atoi(line); 
                    }else{
                        // printf("+x: %s %f%f\n", line, g_->scene._verts[0].x, g_->scene._verts[0].y);
                        const int x = atoi(line); 
                        const uint32_t c = g_->scene._verts_count;
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
            // sections
            else if(!strncmp(line, "sector", 5))
            {
                int c = 0, b = 0;  
                uint32_t vertices = 0;
                const int k = g_->scene._sectors_count;
                stored_n += 8;
                g_->scene._sectors = (sector *) realloc(g_->scene._sectors, sizeof(sector) * (k + 1));
                if(!g_->scene._sectors)
                    return ;
                g_->scene._sectors[k].n_neighbors = 0;
                g_->scene._sectors[k].neighbors = (sector **)calloc(sizeof(sector *), 1);
                g_->scene._sectors[k].vertices = (v2 *)calloc(sizeof(v2), 1);
                while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len) 
                { 
                    line[read_n] = '\0';
                    if(c == 0){
                        g_->scene._sectors[k].floor = (float)(atoi(line));
                    }
                    else if (c == 1){
                        g_->scene._sectors[k].ceil = (float)(atoi(line));
                    }else
                    {
                        
                        if(*(bfr + stored_n + (read_n)) == '\t')
                        {
                           b = 1; 
                        }
                        if(!b) // load vertices
                        {
                            const int 
                                v_ = g_->scene._sectors[k].n_vertices,
                                ix = atoi(line);
                            if(!(ix < 0 || ix > g_->scene._verts_count))
                            { 
                                const v2 vert = g_->scene._verts[ix];
                                g_->scene._sectors[k].vertices = (v2 *) realloc(
                                    g_->scene._sectors[k].vertices, sizeof(v2) * (v_ + 1)
                                );
                                g_->scene._sectors[k].vertices[v_] = (v2)(vert);
                                g_->scene._sectors[k].n_vertices = (v_ + 1);
                                vertices++;
                            }
                        }else{ // load neighbors
                            
                            const int a = atoi(line);
                            if(a > g_->scene._sectors_count || a < 0) //
                            {
                                
                                const int ix = g_->scene._sectors[k].n_neighbors;                               
                                g_->scene._sectors[k].neighbors = (sector **)(
                                    realloc(g_->scene._sectors[k].neighbors, sizeof(sector *) * ix + 1)
                                );
                                g_->scene._sectors[k].neighbors[ix] = &(g_->scene._sectors[ix]);
                                g_->scene._sectors[k].n_neighbors ++; 
                                
                            }else{

                            }
                            
                        }
                
                    }
                    if(*(bfr + stored_n + (read_n)) == '\n')
                    {
                        g_->scene._sectors[k].n_vertices = (vertices);
                        g_->scene._sectors_count++;
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
    printf("\033\[32m Successfully loaded map: %s   [%d vertices, %d sectors]...\n", f, g_->scene._verts_count, g_->scene._sectors_count);
}




static void scene(game_state *g_)
{ 
    g_->scene._walls = (wall *) malloc(100 * sizeof(wall));
    if (!g_->scene._walls)
    {

    }
    add_wall(g_, 600, 200, 600, 400, 0xBBBB00);
    add_wall(g_, 100, 100, 200, 500, 0xBB00BB);
    add_wall(g_, 200, 500, 600, 400, 0x00FF00);

    g_->scene._verts = (v2 *)malloc(sizeof(v2));
    g_->scene._verts[0] = (v2){
        12.0f,
        4.0f
    };
    g_->scene._sectors = (sector *) malloc(sizeof(sector));
    g_->scene._verts_count = 1;
    g_->scene._sectors_count = 1;
    parse_txt(g_, "maps/map.txt");
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
