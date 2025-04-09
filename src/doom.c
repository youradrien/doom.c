#include "doom.h"

// rotate vector v by angle a (in radians)
static inline v2 rotate_v2(v2 v, f32 a)
{
    return (v2){
        (v.x * cos(a)) - (v.y * sin(a)),
        (v.x * sin(a)) + (v.y * cos(a))
    };
}



static BSP_node *find_plyr_node(BSP_node* node, f32 px, f32 py)
{
    while (node->_front || node->_back)
    {
        float side = (node->_partition->b.x - node->_partition->a.x) * (py - node->_partition->a.y)
                        -
                     (node->_partition->b.y - node->_partition->a.y) * (px - node->_partition->a.x);

        node = (side >= 0) ? node->_front : node->_back;
    }
    return node;
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
                //
                if(g_->scene.bsp_head){
                    BSP_node *n = find_plyr_node(g_->scene.bsp_head, g_->p.pos.x, g_->p.pos.y);
                    g_->p._node = (n);
                }

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
    // segfaults
    if(x < 0 || y < 0 || x > WINDOW_WIDTH || y > WINDOW_HEIGHT)
        return;
    const int ix = (WINDOW_WIDTH * y) + x;
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
// 3. Wall-Cliping (test if lines intersects with player viewcone/viewport by using vector2 cross product)
// 4. Perspective transformation  (scaling lines according to their dst from player, for appearance of perspective (far-away objects are smaller).
// 5. [COMING SOON ? ] Z-buffering (track depth of pixel to avoid rendering useless surfaces)
static void render(game_state *g_)
{
    // transform world view to 1st-person view    
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
                */
            }else
            {
                /*
                if(ac0 > +(HFOV /2))
                    printf("SORTIE-GAUCHE \n");
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
        f32 z_floor = (0.0f) + (g_->p.height);
        f32 z_ceiling = 50.0f;
        if((l->_sector))
        {
            const f32 
                h = l->_sector->floor,
                p = l->_sector->ceil;
            z_floor = (g_->p.height / 1.2) - (h);
            z_ceiling = (p);
        }
        // impossible (floor above ceiling)
        if(z_floor >= z_ceiling){
            continue ;
        }

        // ignore far -HFOV/2..+HFOV/2
        // check if angles are entirely far of bounds
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
               wy_t0 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * sy0),
               wy_t1 = (WINDOW_HEIGHT / 2) - (int) (z_ceiling * sy1);

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
        
        // wall sns
        bool s = (wx1 > wx0);
        const int 
            x1 = s ? (wx0) : (wx1),
            x2 = s ? (wx1) : (wx0);

        // wall-outines
        // bottom-top-left-right
        d_line(g_, wx0, wy_b0, wx1, wy_b1, 0xFFFFFF);
        d_line(g_, wx0, wy_t0, wx1, wy_t1, 0xFFFFFF);
        d_line(g_, wx0, wy_b0, wx0, wy_t0, 0xFFFFFF);
        d_line(g_, wx1, wy_b1, wx1, wy_t1, 0xFFFFFF); 

        // A-B rects
        d_rect(g_, wx0 - 2, wy_t1 - 25, 4, 4, 0x00FFFF);
        d_rect(g_, wx1 - 2, wy_t1 - 25, 4, 4, 0x00FF00);

        // wall-filled
        // with verlines
        for(uint16_t i = x1 + 0; i < x2; i++)
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






static void add_wall(game_state *g_, int x1, int y1, int x2, int y2, sector *s, int color)
{
    wall *l = &(g_->scene._walls[g_->scene._walls_ix]);

    if(x1 > WINDOW_WIDTH) x1 = (WINDOW_WIDTH - 1);
    if(x1 < 0) x1 = 1;
    if(x2 < 0) x2 = 1;
    if(x2 > WINDOW_HEIGHT) x2 = (WINDOW_HEIGHT - 1);
    l->a.x = x1;  
    l->a.y = y1;   

    l->b.x = x2;
    l->b.y = y2;
    l->color = color;
    if(!(!s)){
        l->_sector = (s);
    }
    
    g_->scene._walls_ix++;
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
    for(uint32_t i = 0; i < WINDOW_WIDTH -1; i += MAP_TILE)
    {
        d_line(g_, i, 0, i, WINDOW_HEIGHT, 0x005500);
        for(uint32_t j = 0; j < WINDOW_HEIGHT; j += MAP_TILE)
        {
            d_line(g_, i, j, WINDOW_WIDTH, j, 0x005500);
        }
    }
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
    // [World-view]
    // sectors
    if(g_->scene._sectors_count > 1)
    {
        for(uint32_t i = 0; i < g_->scene._sectors_count; i++)
        {
            if(g_->scene._sectors[i].n_vertices <= 1)
                continue;
            for(uint32_t j = 0; j < g_->scene._sectors[i].n_vertices - 1; j ++)
            { 
                const v2
                    *a = g_->scene._sectors[i].vertices[j],
                    *b = g_->scene._sectors[i].vertices[j + 1];
                d_line(g_, (int)(a->x), (int)(a->y), (int)(b->x), (int)(b->y),
                        g_colors[i]);
            } 
        }
    }
    // vertices
    if(g_->scene._verts_count > 1)
    {
        for(uint32_t i = 0; i < g_->scene._verts_count; i ++)
        {
            const v2 a = g_->scene._verts[i];
            const int T = 4;
            d_line(g_, (int)(a.x - T), (int)(a.y - T), (int)(a.x + T), (int)(a.y - T), 0x00FF00);
            d_line(g_, (int)(a.x - T), (int)(a.y + T), (int)(a.x + T), (int)(a.y + T), 0x00FF00);
            d_line(g_, (int)(a.x - T), (int)(a.y - T), (int)(a.x - T), (int)(a.y + T), 0x00FF00);
            d_line(g_, (int)(a.x + T), (int)(a.y - T), (int)(a.x + T), (int)(a.y + T), 0x00FF00); 
        }
    }
    g_->scene._ibsp = 0;
    g_->scene._draw_bspleft = 0;
    g_->scene._draw_bspright = 0;
    if(g_->scene.bsp_head->_front 
        || g_->scene.bsp_head->_back)
    {
        draw_bsp(g_, g_->scene.bsp_head, WINDOW_WIDTH / 2, 450, 120, 0);
    }   
}




// Recursive function to print the BSP tree
static void printBSP(BSP_node *node, int depth)
{
    if (!node) return;

    for (int i = 0; i < depth; i++)
        printf("\033[34m│   ");
    if (node->_partition) {
        printf("├── \033[36m [NODE] Partition: [A](%.1f, %.1f) → [B](%.1f, %.1f) \n",
              (node->_partition)->a.x, (node->_partition)->a.y,
                (node->_partition)->b.x, (node->_partition)->b.y);
    } else {
        printf("\033[35m └── [LEAF] \033[0m \n");
        return;
    }

    // Recursively print front and back children
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
            // vertices
            if(!strncmp(line, "vertex", 6))
            {
                stored_n += 8;
                float y = -1.0f;
                while(sscanf(bfr + stored_n, "%32s%n", line, &read_n) && stored_n < read_len) 
                { 
                    line[read_n] = '\0';
                    if(y == -1){
                        y = atof(line); 
                    }else{
                        const float x = atof(line); 
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
            // SECTORS
            else if(!strncmp(line, "sector", 6))
            {
                int c = 0, b = 0;  
                const int k = g_->scene._sectors_count;
                stored_n += 8;
                g_->scene._sectors = (sector *) realloc(g_->scene._sectors, sizeof(sector) * (k + 1));
                if(!g_->scene._sectors)
                    return ;
                g_->scene._sectors[k].n_portals = 0;
                g_->scene._sectors[k].portals = (sector **)calloc(sizeof(sector *), 1);
                // g_->scene._sectors[k].vertices = (v2 **)calloc(sizeof(v2 *), 1);
                g_->scene._sectors[k].n_vertices = 0;
                while(sscanf(bfr + stored_n, "%50s%n", v_line, &read_n) && stored_n < read_len) 
                { 
                    v_line[read_n] = '\0';
                    if(c == 0){
                        g_->scene._sectors[k].floor = (float)(atoi(v_line));
                    }
                    else if (c == 1){
                        g_->scene._sectors[k].ceil = (float)(atoi(v_line));
                    }else
                    {
                        if(!b) // load vertices pointers
                        {
                            const int 
                                v_ = g_->scene._sectors[k].n_vertices,
                                ix = atoi(v_line);
                            if(!(ix < 0 || ix > g_->scene._verts_count))
                            { 
                                // const v2 *vert = &(g_->scene._verts[ix]);
                                /*
                                g_->scene._sectors[k].vertices = (v2 **) realloc(
                                    g_->scene._sectors[k].vertices, sizeof(v2 *) * (v_ + 1)
                                );
                                (g_->scene._sectors[k].vertices[v_]) = &(g_->scene._verts[ix]);                     
                                g_->scene._sectors[k].n_vertices = (v_ + 1);
                                */
                                g_->scene._sectors[k].vertices[v_] = (v2 *)(&(g_->scene._verts[ix]));
                                g_->scene._sectors[k].n_vertices = (v_ + 1);
                              
                            }
                        }else{ // load neighbors    
                            const int a = atoi(v_line);
                            if(!(a > g_->scene._sectors_count))
                            {
                                const int ix = g_->scene._sectors[k].n_portals;                               
                                g_->scene._sectors[k].portals = (sector **)(
                                    realloc(g_->scene._sectors[k].portals, sizeof(sector *) * ix + 1)
                                );
                                if(a < 0){
                                    g_->scene._sectors[k].portals[ix] = NULL;
                                }else
                                    g_->scene._sectors[k].portals[ix] = &(g_->scene._sectors[ix]);
                                g_->scene._sectors[k].n_portals ++; 
                            }
                        }
                        if(*(bfr + stored_n + (read_n)) == '\t')
                        {
                            b = 1; 
                        }
                    }
                    if(*(bfr + stored_n + (read_n)) == '\n')
                    { 
                        //g_->scene._sectors[k].n_vertices = (vertices);
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
    printf("\033\[32m Successfully loaded map: %s   [%d vertices, %d sectors]...\033\[33m \n", f, g_->scene._verts_count, g_->scene._sectors_count);
}





static void sector_to_walls(game_state *g_)
{
    int x_max = INT_MIN, y_max = INT_MIN;
    int x_min = INT_MAX, y_min = INT_MAX;
    for(uint32_t i = 0; i < g_->scene._verts_count; i++)
    {
        if(g_->scene._verts[i].x > x_max)
            x_max = g_->scene._verts[i].x;
        if(g_->scene._verts[i].x < x_min)
            x_min = g_->scene._verts[i].x;
        if(g_->scene._verts[i].y > y_max)
            y_max = g_->scene._verts[i].y;
        if(g_->scene._verts[i].y < y_min)
            y_min = g_->scene._verts[i].y;
    }
    printf("MAP SIZE:  x[%d, %d] \t y[%d, %d] \n", x_min, x_max, y_min, y_max);
    for(uint32_t i = 0; i < g_->scene._verts_count; i++)
    {
        g_->scene._verts[i].x *= MAP_TILE;
        g_->scene._verts[i].y *= MAP_TILE;
    }    for(uint32_t i = 0; i < g_->scene._sectors_count; i++)
    {
        if(g_->scene._sectors[i].n_vertices < 2)
            continue;
        for(uint32_t j = 0; j < g_->scene._sectors[i].n_vertices - 1; j++)
        {
            const v2 
                *a = g_->scene._sectors[i].vertices[j],
                *b = g_->scene._sectors[i].vertices[j + 1];
            const sector 
                *s = &(g_->scene._sectors[i]),
                *op_s = (g_->scene._sectors[i].portals[j]);

            wall *l = &(g_->scene._walls[g_->scene._walls_ix]);
            // [A]
            l->a.x = a->x;  l->a.y = a->y;   
            // [B]
            l->b.x = b->x;  l->b.y = b->y;

            l->color = g_colors[i];
            if(s){
                l->_sector = (sector *)(s);
            }else
                l->_sector = NULL;
            if(op_s)
                l->_op_sect = (sector *)(op_s);
            else
                l->_op_sect = NULL;
            g_->scene._walls_ix++;        
        }
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
    for(uint32_t i = 0; i < _region_len; i ++)
    {
        const wall *w = &(_region[i]); 
        if(w->_in_bsp
            || w == tracked_wall)
            continue;
        const v2 ac[2] = {
            w->a,
            w->b
        };
        // score format is : [linedefs in front | linedefs in back | splits]
        int scores[3] = {0, 0, 0};
        // re-loop each [linedefs/walls]
        for(uint32_t j = 0; j < _region_len; j++)
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
            for(uint32_t k = 0; k < 2; k++)
            {
                const f32 
                    eq = (ac[k].x - j_w->a.x) * (j_w->b.y - j_w->a.y)
                            - (ac[k].y - j_w->a.y) * (j_w->b.x - j_w->a.x);
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
        // linedef with the fewest splits and most balanced partitioning is chosen.
        if(score < s_score)
        {
            s_score = (score);
            selected_wall = (wall *)(w);
        } 
        // If multiple linedefs have the same split count, prefer the one closest to the center of the map.i
        if(score == s_score){ }
    }
    return (selected_wall);
}




static BSP_node *bsp_(game_state *g_, wall *_region, unsigned int _region_size)
{
    // let leafs exists
    //if(!_region || !_region_size)
    //    return (NULL);
    BSP_node* node = (BSP_node*)malloc(sizeof(BSP_node));

    node->_partition = NULL;
    node->_front = NULL;
    node->_back = NULL;
    if(_region_size < 1){
        return (node); 
    }
    // allocate new region
    wall 
        *front_region = (wall *)malloc(sizeof(wall) * (_region_size)),
        *back_region = (wall *)malloc(sizeof(wall) * (_region_size));

    wall *_W = &(_region[0]);
    // wall *_W = (heuristic_bsp(g_, , _region, _region_size));
    if(!front_region || !back_region)
        return (NULL);
    int
        front_len = 0,
        back_len = 0;
    // attribute [left/right] new splitted regions
    for(uint32_t i = 0; i < _region_size; i ++)
    {
        const wall * 
            l_def = &(_region[i]);
        if(l_def == _W) // ofc bro
            continue;
        wall *A = (_W);
        wall *B = (wall *) (l_def);
        // Compute direction vector of A
        v2 dir_A = { A->b.x - A->a.x, A->b.y - A->a.y };
        
        // Compute normal vector of A (perpendicular)
        v2 normal_A = { -dir_A.y, dir_A.x };
        
        // Compute vector from A.a to B.a
        v2 vec_AB = { B->a.x - A->a.x, B->a.y - A->a.y };
        
        // Compute dot product of vec_AB with normal_A
        float dot = vec_AB.x * normal_A.x + vec_AB.y * normal_A.y;
        
        // fill front_
        if(dot > 0.0f)
        {
            front_region[front_len] = *((wall *)(l_def));
            front_len++;
        }
        // fill back_
        if (dot < 0.0f) 
        {
            back_region[back_len] = *((wall *)(l_def));
            back_len++;
        }
        //handle colinears
        if ( dot == 0.0f)
        {
            if(i % 2 == 0)
            {
                back_region[back_len] = *((wall *)(l_def));
                back_len++;            
            }else{
                front_region[front_len] = *((wall *)(l_def));
                front_len++;
            }
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





static void scene(game_state *g_)
{ 
    g_->scene._walls = (wall *) malloc(200 * sizeof(wall));
    if (!g_->scene._walls)
        return;

    g_->scene._verts = (v2 *)malloc(sizeof(v2));
    if(!g_->scene._verts)
        return;
    g_->scene._verts[0] = (v2){
        0.0f,
        0.0f
    };
    g_->scene._sectors = (sector *) malloc(sizeof(sector));
    if(!g_->scene._sectors)
        return;
    g_->scene._verts_count = 1;
    g_->scene._sectors_count = 1;
    parse_txt(g_, "maps/bruh.txt"); 
    sector_to_walls(g_);
    // INIT SPLITTED REGIONS
    // ---------------------
    BSP_node* node = (BSP_node*)malloc(sizeof(BSP_node));
    wall *splitter = &(g_->scene._walls[g_->scene._walls_ix / 2]);
    // allocate new region
    wall 
        *front_region = (wall *)malloc(sizeof(wall) * (g_->scene._walls_ix)),
        *back_region = (wall *)malloc(sizeof(wall) * (g_->scene._walls_ix));
    if(!front_region || !back_region)
        return ;
    int
        front_len = 0,
        back_len = 0; 
    // initliaze whole left/right] new splitted regions
    for(uint32_t i = 0; i < g_->scene._walls_ix; i ++)
    {
        const wall * 
            l_def = &(g_->scene._walls[i]);
        if(l_def == splitter) // ofc bro
            continue;
        wall *A = (splitter);
        wall *B = (wall *) (l_def);
        // Compute direction vector of A
        const v2 
            dir_A = { A->b.x - A->a.x, A->b.y - A->a.y },
            normal_A = { -dir_A.y, dir_A.x },
            vec_AB = { B->a.x - A->a.x, B->a.y - A->a.y };
        
        // Compute dot product of vec_AB with normal_A
        float dot = vec_AB.x * normal_A.x + vec_AB.y * normal_A.y;
        
        // fill front_
        if(dot > 0.0f)
        {
            front_region[front_len] = *((wall *)(l_def));
            front_len++;
        }
        // fill back_
        if (dot < 0.0f) 
        {
            back_region[back_len] = *((wall *)(l_def));
            back_len++;
        }
        //handle colinears
        if ( dot == 0.0f)
        {
            if(i % 2 == 0)
            {
                back_region[back_len] = *((wall *)(l_def));
                back_len++;            
            }else{
                front_region[front_len] = *((wall *)(l_def));
                front_len++;
            }
        }
    }
    // start whole bsp tree
    g_->scene.bsp_head = (node);
    node->_partition = splitter;
    node->_back = bsp_(g_, back_region, back_len);
    node->_front = bsp_(g_, front_region, front_len);
    printBSP(g_->scene.bsp_head, 0);
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
    (*g_)->p.height = 20.0f;
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
    if(g_->render_mode)
    {    
        render(g_);
    }else
    {
        render_minimap(g_);
    }
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
