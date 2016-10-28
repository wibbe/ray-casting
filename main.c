#include "punity.c"

#define COLOR_BLACK  (1)
#define COLOR_WHITE  (2)
#define COLOR_GRAY   (3)
#define COLOR_RED    (4)
#define COLOR_BLUE   (5)
#define COLOR_GREEN  (6)
#define COLOR_YELLOW (7)



typedef struct {
   int floor_height;
   int ceiling_height;
   int wall_start;
   int wall_count;
} Sector;

typedef struct {
   Vec2 pos;
   int next_sector;
   int color;
} Wall;




/*
Level layout
  
   2---3
   |   |
0--1   4--5
|         |
|    x    |
|         |
|         |
7---------6
*/

#define SECTOR_COUNT 1
#define WALL_COUNT 8

static Sector SECTORS[SECTOR_COUNT] = {
   {0.0f, 0.0f, 0, WALL_COUNT}
};

static Wall WALLS[WALL_COUNT] = {
   {{ -4,  2},  -1, COLOR_RED},
   {{ -1,  2},  -1, COLOR_GREEN},
   {{ -1,  4},  -1, COLOR_BLUE},
   {{  1,  4},  -1, COLOR_GREEN},
   {{  1,  2},  -1, COLOR_RED},
   {{  4,  2},  -1, COLOR_YELLOW},
   {{  4, -3},  -1, COLOR_BLUE},
   {{ -4, -3},  -1, COLOR_YELLOW}
};


Vec2 player_pos = {0.0, 0.0};
Vec2 player_dir = {0.0, 1.0};
Vec2 camera_plane = {0.66, 0.0};

static Font font;
static Bitmap chess;

typedef struct {
   float ray_dist;
   float line_dist;
} Intersection;

// Line ray intersection from these articles
// https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d
// http://stackoverflow.com/questions/14307158/how-do-you-check-for-intersection-between-a-line-segment-and-a-line-ray-emanatin
Intersection line_ray_intersect(Vec2 ray_start, Vec2 ray_dir, Vec2 p1, Vec2 p2)
{
   Vec2 v1 = vec2_sub(ray_start, p1);
   Vec2 v2 = vec2_sub(p2, p1);
   Vec2 v3 = vec2_perp(ray_dir);

   float dot = vec2_dot(v2, v3);

   // Are the ray and line parallel?
   //if (abs(dot) < 0.00000001)
   //   return -1.0f;

   float t1 = vec2_perp_dot(v2, v1) / dot;
   float t2 = vec2_dot(v1, v3) / dot;

   Intersection result = {-1.0f, -1.0f};

   if (t1 >= 0.0f && t2 >= 0.0f && t2 <= 1.0f)
   {
      result.ray_dist = t1;
      result.line_dist = t2;
   }

   return result;
}

float wall_dists[PUN_CANVAS_WIDTH] = {0};
int wall_sample = 0;

void raycast(int x, Vec2 ray_pos, Vec2 ray_dir, int start_sector)
{
   float wall_dist = 99999.9f;
   float wall_interpolation = 0.0f;
   int closest_wall = -1;

   Sector * sector = &SECTORS[start_sector];

   for (int wall = 0; wall < sector->wall_count; ++wall)
   {
      int w1 = sector->wall_start + wall;
      int w2 = sector->wall_start + ((wall + 1) % sector->wall_count);

      int inside = vec2_perp_dot(vec2_sub(ray_pos, WALLS[w1].pos), vec2_sub(WALLS[w2].pos, WALLS[w1].pos)) > 0 ? 1 : 0;
      if (inside)
      {
         Intersection intersection = line_ray_intersect(ray_pos, ray_dir, WALLS[w1].pos, WALLS[w2].pos);

         float dist = vec2_dot(player_dir, vec2_mul(ray_dir, intersection.ray_dist));
         
         if (dist > 0.0f && dist < wall_dist)
         {
            closest_wall = wall;
            wall_dist = dist;
            wall_interpolation = intersection.line_dist;
         }
      }
   }

   wall_dists[x] = wall_dist;

   if (closest_wall >= 0)
   {
      Wall * wall = &WALLS[closest_wall];

      int height = (1.0f / wall_dist) * PUN_CANVAS_HEIGHT * 2;
      //int height = maximum(real_height, PUN_CANVAS_HEIGHT);

      int half_height = height / 2;

      int start = (PUN_CANVAS_HEIGHT / 2) - half_height;

      for (int y = 0; y < height; ++y)
      {
         int tex_x = (int)(wall_interpolation * chess.width);
         int tex_y = (int)((y / (float)height) * (float)chess.height);

         int py = start + y;
         if (py < 0  || py > PUN_CANVAS_HEIGHT)
            continue;

         int idx = x + (py * PUN_CANVAS_WIDTH);

         pixel_draw(x, py, chess.pixels[tex_x + (tex_y * chess.width)]);
      }
   }
}

Vec2 to_view_space(Vec2 pos)
{
   pos = vec2_sub(pos, player_pos);

   float inv_det = 1.0f / (camera_plane.x * player_dir.y - player_dir.x * camera_plane.y); //required for correct matrix multiplication

   float transform_x = inv_det * (player_dir.y * pos.x - player_dir.x * pos.y);
   float transform_y = inv_det * (-camera_plane.y * pos.x + camera_plane.x * pos.y); //this is actually the depth inside the screen, that what Z is in 3D

   return vec2_make(transform_x, transform_y);
}

Vec2 rotate(Vec2 dir, float angle)
{
   // Rotation matrix
   // [ cos(a), -sin(a) ] * [ dir.x ]
   // [ sin(a),  cos(a) ]   [ dir.y ]

   float s = sin(angle);
   float c = cos(angle);

   float x = (dir.x * c) - (dir.y * s);
   float y = (dir.x * s) + (dir.y * c);

   return vec2_make(x, y);
}


void draw_minimap()
{
   Rect r = rect_make(0, PUN_CANVAS_HEIGHT - 100, 100, PUN_CANVAS_HEIGHT);

   clip_set(r);
   rect_draw(r, COLOR_GRAY);

   for (int i = 0; i < WALL_COUNT; ++i)
   {
      Wall * w = &WALLS[i];
      Vec2 pos = to_view_space(w->pos);

      int x = 50 + (pos.x * 5);
      int y = PUN_CANVAS_HEIGHT - 50 - (pos.y * 5);

      if (rect_contains_point(&r, x, y))
         pixel_draw(x, y, w->color);
   }

   CORE->canvas.bitmap->pixels[50 + (PUN_CANVAS_HEIGHT - 50) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[50 + (PUN_CANVAS_HEIGHT - 51) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[50 + (PUN_CANVAS_HEIGHT - 52) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[50 + (PUN_CANVAS_HEIGHT - 53) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[50 + (PUN_CANVAS_HEIGHT - 54) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   
   CORE->canvas.bitmap->pixels[49 + (PUN_CANVAS_HEIGHT - 53) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[51 + (PUN_CANVAS_HEIGHT - 53) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[52 + (PUN_CANVAS_HEIGHT - 52) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
   CORE->canvas.bitmap->pixels[48 + (PUN_CANVAS_HEIGHT - 52) * PUN_CANVAS_WIDTH] = COLOR_BLACK;
}

void init()
{
   CORE->canvas.palette.colors[0] = color_make(0x00, 0x00, 0x00, 0x00);
   CORE->canvas.palette.colors[COLOR_BLACK] = color_make(0x00, 0x00, 0x00, 0xff);
   CORE->canvas.palette.colors[COLOR_WHITE] = color_make(0xff, 0xff, 0xff, 0xff);
   CORE->canvas.palette.colors[COLOR_GRAY] = color_make(0xaa, 0xaa, 0xaa, 0xff);
   CORE->canvas.palette.colors[COLOR_RED] = color_make(0xff, 0x00, 0x00, 0xff);
   CORE->canvas.palette.colors[COLOR_GREEN] = color_make(0x00, 0xff, 0x00, 0xff);
   CORE->canvas.palette.colors[COLOR_BLUE] = color_make(0x00, 0x00, 0xff, 0xff);
   CORE->canvas.palette.colors[COLOR_YELLOW] = color_make(0xff, 0xff, 0x00, 0xff);

   CORE->canvas.palette.colors_count = 8;
   canvas_clear(COLOR_BLACK);


   bitmap_load_resource(&chess, "chess.png");
   bitmap_load_resource(&font.bitmap, "font.png");

   font.char_width = 4;
   font.char_height = 7;
   CORE->canvas.font = &font;
}

void step()
{
   if (key_pressed(KEY_ESCAPE))
      CORE->running = 0;

   if (key_down(KEY_W))
      player_pos = vec2_add(player_pos, vec2_mul(player_dir, 0.08f));

   if (key_down(KEY_S))
      player_pos = vec2_add(player_pos, vec2_mul(player_dir, -0.08f));

   if (key_down(KEY_A))
      player_pos = vec2_add(player_pos, vec2_mul(vec2_perp(player_dir), 0.06f));

   if (key_down(KEY_D))
      player_pos = vec2_add(player_pos, vec2_mul(vec2_perp(player_dir), -0.06f));

      
   player_dir = rotate(player_dir, (CORE->mouse_dx * 0.005f));
   camera_plane = vec2_mul(vec2_perp(player_dir), -0.66f);

   clip_reset();
   canvas_clear(COLOR_BLACK);


   for (int x = 0; x < PUN_CANVAS_WIDTH; ++x)
   {
      float camera_x = 2.0 * (x / (float)PUN_CANVAS_WIDTH) - 1.0;

      Vec2 ray_pos = player_pos;
      Vec2 ray_dir = vec2_normalize(vec2_add(player_dir, vec2_mul(camera_plane, camera_x)));

      raycast(x, ray_pos, ray_dir, 0);
   }

   //printf("x: %d d: %f\n", wall_sample, wall_dists[wall_sample]);


   static char buf[256];
   sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
   //sprintf(buf, "%0.2f %0.2f (%0.2f %0.2f)", player_pos.x, player_pos.y, player_dir.x, player_dir.y); 
   text_draw(buf, 0, 0, COLOR_WHITE);

   draw_minimap();
}