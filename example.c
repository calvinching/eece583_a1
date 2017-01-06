#include <stdio.h>
#include <stdlib.h>
#include "graphics.h"
#include "common.h"

void delay (void);
void button_press (float x, float y, int flags);
void drawscreen (void);
void new_button_func (void (*drawscreen_ptr) (void));
void mouse_move (float x, float y);
void key_press (int i);
void init_grid();

void clean_up(void) {
    // function to do "clean up" when the graphics window is closed unexpectedly.
}

typedef struct CELL {
    float x1;
    float x2;
    float y1;
    float y2;
    char text[5];
    float text_x;
    float text_y;
};

CELL **grid;

int num_rows = 10;
int num_columns = 5;

int main() {

    int i;
    
    // initialize display with WHITE background, and define a clean_up function
    init_graphics("Some Example Graphics", WHITE, clean_up);
    init_world(0.,0.,1000.,1000.);
    init_grid();
    printf("creating button\n");
    printf("grid: %f\n", grid[0][0].x1);
    create_button("Window", "New Button", new_button_func);
    printf("finished creating button\n");
    drawscreen(); 
    event_loop(button_press, mouse_move, key_press, drawscreen);
    return 0;
}

void init_grid() {
    float x1 = 0;
    float y1 = 0;
    float x2 = 0;
    float y2 = 0;

    t_report report;
    report_structure(&report);
    printf("width: %d, height: %d\n", report.top_width, report.top_height);

    float height = report.top_height;
    float width = report.top_width;
    float cell_height = height / num_rows;
    float cell_width = width / num_columns;

    // Allocate memory for the grid
    grid = (CELL **)my_malloc(num_rows * sizeof(CELL *));
    for (int i = 0; i < num_rows; i++) {
        grid[i] = (CELL *)my_malloc(num_columns * sizeof(CELL));
    }

    printf("cell height: %f, cell width: %f\n", cell_height, cell_width);

    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_columns; j++) {
            grid[i][j].x1 = cell_width * j;
            grid[i][j].y1 = cell_height * i;
            grid[i][j].x2 = cell_width + cell_width * j;
            grid[i][j].y2 = cell_height + cell_height * i;
            grid[i][j].text_x = grid[i][j].x2 - cell_width / 2.;
            grid[i][j].text_y = grid[i][j].y2 - cell_height / 2.;
            printf("grid[%d][%d] = (%f, %f) (%f, %f) (%f, %f)\n", i, j, grid[i][j].x1, grid[i][j].y1, grid[i][j].x2, grid[i][j].y2, grid[i][j].text_x, grid[i][j].text_y);
        }
    }
}

void draw_grid() {
    printf("draw_grid\n");
    // Draw grid
    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_columns; j++) {
            char text[10] = "";
            sprintf(text, "(%d, %d)", i, j);
            drawrect(grid[i][j].x1, grid[i][j].y1, grid[i][j].x2, grid[i][j].y2); 
            drawtext(grid[i][j].text_x, grid[i][j].text_y, text, 150.);
        }
    }
}

void drawscreen (void) {
    printf("draw_screen\n");
    /**
     * redrawing routine for still pictures.  Graphics package calls
     * this routine to do redrawing after the user changes the window
     * in any way.
     */
    t_point polypts[3] = {{500.,400.},{450.,480.},{550.,480.}};
    t_point polypts2[4] = {{700.,400.},{650.,480.},{750.,480.}, {800.,400.}};

    clearscreen();  /* Should be first line of all drawscreens */

    draw_grid();
    /*
    t_report report;
    report_structure(&report);
    printf("width: %d, height: %d\n", report.top_width, report.top_height);
    float height = report.top_height;
    float width = report.top_width;
    float x1 = 0;
    float y1 = 0;
    float x2 = 0;
    float y2 = 0;
    int current_row = 0;
    int current_col = 0;
    float height_per_row = height / num_rows;
    float width_per_col = width / num_columns;

    // Draw the grid. First draw the rows
    while (current_row < num_rows) {
        x1 = 0;
        y1 += height_per_row;
        x2 = width;
        y2 = y1;
        printf("Drawing (%f, %f) to (%f, %f)\n", x1, y1, x2, y2);
        drawline(x1, y1, x2, y2);
        current_row++;
    }

    // Reset counters;
    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;

    // Draw the columns
    while (current_col < num_columns) {
        x1 += width_per_col;
        y1 = 0;
        x2 = x1;
        y2 = height;
        drawline(x1, y1, x2, y2);
        current_col++;
    }
    */
/*
    setfontsize (10);
    setlinestyle (SOLID);
    setlinewidth (1);
    setcolor (BLACK);

    drawtext (0.,55.,"Colors",150.);
    setcolor (LIGHTGREY);
    fillrect (150.,30.,200.,80.);
    setcolor (DARKGREY);
    fillrect (200.,30.,250.,80.);
    setcolor (WHITE);
    fillrect (250.,30.,300.,80.);
    setcolor (BLACK);
    fillrect (300.,30.,350.,80.);
    setcolor (BLUE);
    fillrect (350.,30.,400.,80.);
    setcolor (GREEN);
    fillrect (400.,30.,450.,80.);
    setcolor (YELLOW);
    fillrect (450.,30.,500.,80.);
    setcolor (CYAN);
    fillrect (500.,30.,550.,80.);
    setcolor (RED);
    fillrect (550.,30.,600.,80.);
    setcolor (DARKGREEN);
    fillrect (600.,30.,650.,80.);
    setcolor (MAGENTA);
    fillrect (650.,30.,700.,80.);

 setcolor (WHITE);
 drawtext (400.,55.,"fillrect",150.);
 setcolor (BLACK);
 drawtext (500.,150.,"drawline",150.);
 setlinestyle (SOLID);
 drawline (440.,120.,440.,200.);
 setlinestyle (DASHED);
 drawline (560.,120.,560.,200.);
 setcolor (BLUE);
 drawtext (190.,300.,"drawarc",150.);
 drawarc (190.,300.,50.,0.,270.);
 drawarc (300.,300.,50.,0.,-180.);
 fillarc (410.,300.,50.,90., -90.);
 fillarc (520.,300.,50.,0.,360.);
 setcolor (BLACK);
 drawtext (520.,300.,"fillarc",150.);
 setcolor (BLUE);
 fillarc (630.,300.,50.,90.,180.);
 fillarc (740.,300.,50.,90.,270.);
 fillarc (850.,300.,50.,90.,30.);
 setcolor (RED);
 fillpoly(polypts,3);
 fillpoly(polypts2,4);
 setcolor (BLACK);
 drawtext (500.,450.,"fillpoly",150.);
 setcolor (DARKGREEN);
 drawtext (500.,610.,"drawrect",150.);
 drawrect (350.,550.,650.,670.); 

 setcolor (BLACK);
 setfontsize (8);
 drawtext (100.,770.,"8 Point Text",800.);
 setfontsize (12);
 drawtext (400.,770.,"12 Point Text",800.);
 setfontsize (15);
 drawtext (700.,770.,"18 Point Text",800.);
 setfontsize (24);
 drawtext (300.,830.,"24 Point Text",800.);
 setfontsize (32);
 drawtext (700.,830.,"32 Point Text",800.);
 setfontsize (10);

 setlinestyle (SOLID);
 drawtext (200.,900.,"Thin line (width 1)",800.);
 setlinewidth (1);
 drawline (100.,920.,300.,920.);
 drawtext (500.,900.,"Width 3 Line",800.);
 setlinewidth (3);
 drawline (400.,920.,600.,920.);
 drawtext (800.,900.,"Width 6 Line",800.);
 setlinewidth (6);
 drawline (700.,920.,900.,920.);
 */
}


void delay (void) {

/* A simple delay routine for animation. */

 int i, j, k, sum;

 sum = 0;
 for (i=0;i<100;i++) 
    for (j=0;j<i;j++)
       for (k=0;k<30;k++) 
          sum = sum + i+j-k; 
}

void button_press (float x, float y, int flags) {

/* Called whenever event_loop gets a button press in the graphics *
 * area.  Allows the user to do whatever he/she wants with button *
 * clicks.                                                        */

 printf("User clicked a button at coordinates (%f, %f)", x, y);
#ifdef WIN32
 if (flags & MOUSECLK_CONTROL)
     printf(", CTRL pressed");
 if (flags & MOUSECLK_SHIFT)
     printf(", SHIFT pressed");
#endif
 printf("\n");
}


void new_button_func (void (*drawscreen_ptr) (void)) {

 printf ("You pressed the new button!\n");
 setcolor (MAGENTA);
 setfontsize (12);
 drawtext (500., 500., "You pressed the new button!", 10000.);
}

void mouse_move (float x, float y) {
    // function to handle mouse move event, the current mouse position in the current world coordinate
    // as defined as MAX_X and MAX_Y in init_world is returned
}

void key_press (int i) {
    // function to handle keyboard press event, the ASCII character is returned
}
