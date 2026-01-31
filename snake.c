#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "keyboard.h"

#define GAME_WIDTH 40
#define GAME_HEIGHT 15
#define MAX_SNAKE 200

typedef struct {
    uint16_t x;
    uint16_t y;
} Point;

typedef struct {
    Point body[MAX_SNAKE];
    size_t length;
    int dx;
    int dy;
    int next_dx;
    int next_dy;
} Snake;

static Snake g_snake;
static Point g_food;
static uint32_t g_score = 0;
static uint32_t g_ticks = 0;
static int g_game_over = 0;

static void draw_border(void)
{
    console_set_cursor(2, 0);
    console_write("Game area:");
    
    // Top border
    console_set_cursor(3, 0);
    console_putc('+');
    for (int i = 0; i < GAME_WIDTH; i++) {
        console_putc('-');
    }
    console_putc('+');
    
    // Side borders
    for (int y = 0; y < GAME_HEIGHT; y++) {
        console_set_cursor(4 + y, 0);
        console_putc('|');
        console_set_cursor(4 + y, GAME_WIDTH + 1);
        console_putc('|');
    }
    
    // Bottom border
    console_set_cursor(4 + GAME_HEIGHT, 0);
    console_putc('+');
    for (int i = 0; i < GAME_WIDTH; i++) {
        console_putc('-');
    }
    console_putc('+');
}

static void draw_snake(void)
{
    for (size_t i = 0; i < g_snake.length; i++) {
        char ch = (i == 0) ? 'H' : 'o';
        console_set_cursor(4 + g_snake.body[i].y, 1 + g_snake.body[i].x);
        console_putc(ch);
    }
}

static void draw_food(void)
{
    console_set_cursor(4 + g_food.y, 1 + g_food.x);
    console_putc('*');
}

static void draw_ui(void)
{
    console_set_cursor(4 + GAME_HEIGHT + 2, 0);
    console_write("Score: ");
    
    // Simple number to string conversion
    uint32_t score = g_score;
    int digits[10];
    int digit_count = 0;
    
    if (score == 0) {
        console_putc('0');
    } else {
        while (score > 0) {
            digits[digit_count++] = score % 10;
            score /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            console_putc('0' + digits[i]);
        }
    }
    
    console_write("  Length: ");
    int length = g_snake.length;
    int digits2[10];
    int digit_count2 = 0;
    
    while (length > 0) {
        digits2[digit_count2++] = length % 10;
        length /= 10;
    }
    for (int i = digit_count2 - 1; i >= 0; i--) {
        console_putc('0' + digits2[i]);
    }
}

static void clear_game_area(void)
{
    for (int y = 0; y < GAME_HEIGHT; y++) {
        console_clear_line(4 + y);
    }
}

static int check_collision(uint16_t x, uint16_t y)
{
    // Check wall collision
    if (x >= GAME_WIDTH || y >= GAME_HEIGHT) {
        return 1;
    }
    
    // Check self collision
    for (size_t i = 0; i < g_snake.length; i++) {
        if (g_snake.body[i].x == x && g_snake.body[i].y == y) {
            return 1;
        }
    }
    
    return 0;
}

static void spawn_food(void)
{
    // Simple pseudo-random food spawning
    static uint32_t seed = 12345;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    
    g_food.x = (seed / 65536) % GAME_WIDTH;
    g_food.y = ((seed / 65536) / GAME_WIDTH) % GAME_HEIGHT;
    
    // Make sure food doesn't spawn on snake
    for (size_t i = 0; i < g_snake.length; i++) {
        if (g_snake.body[i].x == g_food.x && g_snake.body[i].y == g_food.y) {
            spawn_food();
            return;
        }
    }
}

static void update_game(void)
{
    g_ticks++;
    
    // Update direction based on queued input
    if (g_snake.next_dx != 0 || g_snake.next_dy != 0) {
        // Prevent reversing into itself
        if (!(g_snake.next_dx == -g_snake.dx && g_snake.next_dy == -g_snake.dy)) {
            g_snake.dx = g_snake.next_dx;
            g_snake.dy = g_snake.next_dy;
            g_snake.next_dx = 0;
            g_snake.next_dy = 0;
        }
    }
    
    // Move snake - slower movement
    if (g_ticks % 50 == 0) {  // Much slower - move every 50 ticks instead of 5
        uint16_t new_x = g_snake.body[0].x + g_snake.dx;
        uint16_t new_y = g_snake.body[0].y + g_snake.dy;
        
        if (check_collision(new_x, new_y)) {
            g_game_over = 1;
            return;
        }
        
        // Check if eating food
        int ate_food = 0;
        if (new_x == g_food.x && new_y == g_food.y) {
            g_score++;
            ate_food = 1;
            spawn_food();
        }
        
        // Shift body (remove tail if not eating)
        if (!ate_food && g_snake.length > 0) {
            for (size_t i = g_snake.length - 1; i > 0; i--) {
                g_snake.body[i] = g_snake.body[i - 1];
            }
        } else if (ate_food && g_snake.length < MAX_SNAKE) {
            for (size_t i = g_snake.length; i > 0; i--) {
                g_snake.body[i] = g_snake.body[i - 1];
            }
            g_snake.length++;
        }
        
        g_snake.body[0].x = new_x;
        g_snake.body[0].y = new_y;
    }
}

static void handle_input(void)
{
    if (!keyboard_has_data()) {
        return;
    }
    
    int key = keyboard_read_key();
    
    if (key == KEY_UP && g_snake.dy == 0) {
        g_snake.next_dx = 0;
        g_snake.next_dy = -1;
    } else if (key == KEY_DOWN && g_snake.dy == 0) {
        g_snake.next_dx = 0;
        g_snake.next_dy = 1;
    } else if (key == KEY_LEFT && g_snake.dx == 0) {
        g_snake.next_dx = -1;
        g_snake.next_dy = 0;
    } else if (key == KEY_RIGHT && g_snake.dx == 0) {
        g_snake.next_dx = 1;
        g_snake.next_dy = 0;
    } else if (key == 'q' || key == 'Q') {
        g_game_over = 2;  // Quit signal
    }
}

void snake_game_run(void)
{
    console_clear();
    
    // Initialize snake in the middle
    g_snake.length = 3;
    g_snake.body[0].x = GAME_WIDTH / 2;
    g_snake.body[0].y = GAME_HEIGHT / 2;
    g_snake.body[1].x = GAME_WIDTH / 2 - 1;
    g_snake.body[1].y = GAME_HEIGHT / 2;
    g_snake.body[2].x = GAME_WIDTH / 2 - 2;
    g_snake.body[2].y = GAME_HEIGHT / 2;
    
    g_snake.dx = 1;
    g_snake.dy = 0;
    g_snake.next_dx = 0;
    g_snake.next_dy = 0;
    
    g_score = 0;
    g_game_over = 0;
    g_ticks = 0;
    
    spawn_food();
    
    console_write("=== SNAKE GAME ===\n");
    console_write("Arrow keys to move, Q to quit\n\n");
    
    draw_border();
    draw_snake();
    draw_food();
    draw_ui();
    
    // Game loop
    while (!g_game_over) {
        handle_input();
        update_game();
        
        // Redraw every frame
        clear_game_area();
        draw_border();
        draw_snake();
        draw_food();
        draw_ui();
        
        // Long delay (busy wait) - makes game playable speed
        for (volatile int i = 0; i < 2000000; i++);
    }
    
    if (g_game_over == 1) {
        console_set_cursor(4 + GAME_HEIGHT + 4, 0);
        console_write("GAME OVER! Final Score: ");
        
        uint32_t score = g_score;
        int digits[10];
        int digit_count = 0;
        
        if (score == 0) {
            console_putc('0');
        } else {
            while (score > 0) {
                digits[digit_count++] = score % 10;
                score /= 10;
            }
            for (int i = digit_count - 1; i >= 0; i--) {
                console_putc('0' + digits[i]);
            }
        }
        console_putc('\n');
    }
}
