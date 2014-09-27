#include <stdint.h>

#include "FastLED.h"

#define __always_inline __attribute__((always_inline))


#define PIXEL_WIDTH     (20)
#define PIXEL_HEIGHT    (16)
#define BOARD_WIDTH     (128)
#define BOARD_HEIGHT    (128)
#define TIMEOUT_STALL   (4*1000)
#define TIMEOUT_PATTERN (40*1000)


typedef struct color {
    uint8_t r,g,b;
} t_color;

typedef struct pattern {
    uint8_t range;
    uint8_t threshold;
    uint8_t nbr_states;
    unsigned char type;
} t_pattern;

const struct pattern patterns[] = {
    {2, 5, 3,  'N'},
    {2, 3, 3,  'N'},
    {1, 1, 12, 'N'},
    {1, 3, 3,  'M'},
    {1, 2, 5,  'M'}
};
const unsigned int nbr_patterns = sizeof(patterns)/sizeof(patterns[0]);


const struct pattern *g_pattern;
unsigned long g_timestamp;
unsigned long g_timestamp_stalled;
uint8_t buffer_0[BOARD_HEIGHT][BOARD_WIDTH];
uint8_t buffer_1[BOARD_HEIGHT][BOARD_WIDTH];
uint8_t (*matrix)[BOARD_WIDTH] = buffer_0;
uint8_t (*matrix_nxt)[BOARD_WIDTH] = buffer_1;
uint8_t (*matrix_tmp)[BOARD_WIDTH];

CRGB leds[PIXEL_HEIGHT][PIXEL_WIDTH];
t_color *g_colorspace = NULL;
bool g_changed = false;


void rand_pattern(void)
{
    Serial.print("rand_pattern\n");
    uint8_t idx = random(nbr_patterns);
    g_pattern = &(patterns[idx]);
}


void rand_matrix(const struct pattern *p, uint8_t (*m)[BOARD_WIDTH])
{
    Serial.print("rand_matrix\n");
    for (int y = 0; y < BOARD_HEIGHT; ++y)  {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            m[y][x] = random(p->nbr_states);
        }
    }
}


void rand_colorspace(const struct pattern *p)
{
    Serial.print("rand_colorspace\n");
    if (g_colorspace != NULL) free(g_colorspace);
    g_colorspace = (t_color*) malloc(p->nbr_states*sizeof(t_color));
    if (g_colorspace == NULL) return;
    uint8_t  start_colors[3] = {random(255), random(255), random(255)};
    uint8_t color_spacing = 255 / p->nbr_states;
    for (int i = 0; i < p->nbr_states; i++) {
        g_colorspace[i].r = (start_colors[0] + i*color_spacing) % 256;
        g_colorspace[i].g = (start_colors[1] + i*color_spacing) % 256;
        g_colorspace[i].b = (start_colors[2] + i*color_spacing) % 256;  
    }
}

void initialize_board(void)
{
    rand_pattern();
    rand_matrix(g_pattern, matrix);
    rand_colorspace(g_pattern);
    g_timestamp = millis();
}

void setup()
{
    Serial.begin(9600);
    Serial.print("Setup\n");
    randomSeed(analogRead(0) ^ analogRead(10) ^ analogRead(20) ^ analogRead(30));
    LEDS.addLeds<LPD8806, 0,  1 >(leds[0],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 2,  3 >(leds[1],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 4,  5 >(leds[2],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 6,  7 >(leds[3],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 8,  9 >(leds[4],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 10, 11>(leds[5],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 12, 13>(leds[6],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 14, 15>(leds[7],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 16, 17>(leds[8],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 18, 19>(leds[9],  PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 20, 21>(leds[10], PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 22, 23>(leds[11], PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 24, 25>(leds[12], PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 26, 27>(leds[13], PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 28, 29>(leds[14], PIXEL_WIDTH);
    LEDS.addLeds<LPD8806, 30, 31>(leds[15], PIXEL_WIDTH);

    // Clear pixels to reduce power consumption and allow reprogramming
    clear_pixels();
    delay(1000);
    
    initialize_board();
}


void loop()
{
Serial.print("loop()\n");
    unsigned long perf_timer = millis();
    write_pixels();
Serial.print("write_pixels\n");
    g_changed = false;
    process_next_gen(g_pattern);
    
    if (g_changed) {
        g_timestamp_stalled = millis();
    }

Serial.print("process_next_gen\n");

    // Handle stalls
    if (!g_changed &&
        (millis() - g_timestamp_stalled) > TIMEOUT_STALL)
    {
        rand_matrix(g_pattern, matrix_nxt);
        // TODO: Instead of randomizing matrix.
        //       Try switching to another pattern with the same number of states
        // print "Generation stalled.. randomizing board"
    }
Serial.print("stalls handled\n");

    // Old Pattern reset
    if ((millis() - g_timestamp) > TIMEOUT_PATTERN)
    {
        rand_matrix(g_pattern, matrix_nxt);
        g_timestamp = millis();
        initialize_board();
        // print "Pattern is getting boring.. randomizing board"
    }
Serial.print("old patterns reset\n");

Serial.print("time since timestamp: ");
Serial.println(millis() - g_timestamp);
Serial.print("time in loop: ");
Serial.println(millis() - perf_timer);
    matrix_tmp = matrix;
    matrix = matrix_nxt;
    matrix_nxt = matrix_tmp;
//    delay(25);
}


__always_inline void write_pixels(void)
{
    for (int y = 0; y < PIXEL_HEIGHT; ++y)  {
        for (int x = 0; x < PIXEL_WIDTH; ++x) {
            leds[y][x] = CHSV(c.r, 255, 255);
            t_color c = g_colorspace[matrix[y][x]];
        }
    }
    LEDS.show();
}


void clear_pixels(void)
{
    for (int y = 0; y < PIXEL_HEIGHT; ++y)  {
        for (int x = 0; x < PIXEL_WIDTH; ++x) {
            leds[y][x] = 0;
        }
    }
    LEDS.show();
}


__always_inline void process_next_gen(const struct pattern *p)
{
//    Serial.print("process_next_gen\n");
    switch (p->type) {
    case 'N': {
        for (unsigned int y = 0; y < BOARD_HEIGHT; ++y)  {
            for (unsigned int x = 0; x < BOARD_WIDTH; ++x) {
                process_neumann(p, x, y);
            }
        }
    }
    break;
    case 'M': {
        for (unsigned int y = 0; y < BOARD_HEIGHT; ++y)  {
            for (unsigned int x = 0; x < BOARD_WIDTH; ++x) {
                process_moore(p, x, y);
            }
        }
    }
    break;
    }
}


__always_inline void process_moore(const struct pattern *p, unsigned int x, unsigned int y)
{
    unsigned int n_neigh = 0;
    unsigned int n_searched = 0;
    for (int ii = -(p->range); ii < (p->range+1); ++ii) {
        unsigned int y_i = (x + ii) % BOARD_HEIGHT;

        for (int i = -(p->range); i < (p->range+1); ++i) {
            unsigned int x_i = (x + i) % BOARD_WIDTH;

            if (x != x_i || y != y_i) {
                n_searched += 1;
                if (((matrix[y][x] + 1) % p->nbr_states) == matrix[y_i][x_i]) {
                    n_neigh += 1;
                }
            }
        }
    }
//#    print "n_neigh=%d  searched=%d  range=%d  threshold=%d" % (n_neigh, n_searched, rang, thresh)
/*
Serial.print("matrix[");
Serial.print(y);
Serial.print("][");
Serial.print(x);
Serial.print("]\n");
*/
    if (n_neigh >= p->threshold) {
        matrix_nxt[y][x] = (matrix[y][x] + 1) % p->nbr_states;
        g_changed = true;
    } else {
        matrix_nxt[y][x] = matrix[y][x];
    }
}


__always_inline void process_neumann(const struct pattern *p, unsigned int x, unsigned int y)
{
    unsigned int n_neigh = 0;
    unsigned int n_searched = 0;
    unsigned int nxt_state = (matrix[y][x] + 1) % p->nbr_states;
    for (int r = 1; r < (p->range + 1); r++) {
        for (int i = 0; i < (r + 1); i++) {
            n_searched += 2;
            if  (nxt_state == matrix[(y - (r - i)) % BOARD_HEIGHT][(x + i) % BOARD_WIDTH]) {
                n_neigh += 1;
            }
            if  (nxt_state == matrix[(y + (r - i)) % BOARD_HEIGHT][(x - i) % BOARD_WIDTH]) {
                n_neigh += 1;
            }
        }
        for (int i = 0; i < r; i++) {
            n_searched += 2;
            if  (nxt_state == matrix[(y - (r - i)) % BOARD_HEIGHT][(x - i) % BOARD_WIDTH]) {
                n_neigh += 1;
            }
            if  (nxt_state == matrix[(y + (r - i)) % BOARD_HEIGHT][(x + i) % BOARD_WIDTH]) {
                n_neigh += 1;
            }
        }
    }

    if (n_neigh >= p->threshold) {
        matrix_nxt[y][x] = (matrix[y][x] + 1) % p->nbr_states;
        g_changed = true;
    } else {
        matrix_nxt[y][x] = matrix[y][x];
    }
}
