#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "graphics.h"
#include "common.h"

#define DEBUG
#define SUCCESS 0
#define ERROR -1

void button_press (float x, float y, int flags);
void drawscreen();
void proceed_button_func(void (*drawscreen_ptr) (void));
void mouse_move (float x, float y);
void key_press (int i);
void init_grid();
void delay();
int parse_file(char *file);
void run_lee_moore_algo();

int num_rows = 0;
int num_columns = 0;

typedef struct CELL {
    float x1;       // x-coordinate of the cell's top left corner
    float y1;       // y-coordinate of the cell's top left corner
    float x2;       // x-coordinate of the cell's bottom right corner
    float y2;       // y-coordinate of the cell's bottom right corner
    char text[5];   // text of the cell
    float text_x;   // x-coordinate of the text
    float text_y;   // y-coordinate of the text

    bool is_obstruction;    // true if this cell is an obstruction for wiring
    bool is_source;         // true if it's a source
    bool is_sink;           // true if it's a sink
    bool is_routed;         // true if we've tried routing this cell
    bool is_wire;           // true if the cell is a wire
    int wire_num;           // wire number (i.e. the net number)
    int value;              // value of the lee-moore algo
} CELL;

CELL **grid;

int cur_src_col = -1;
int cur_src_row = -1;
int cur_wire_num = -1;

typedef struct LOCATION {
    int col;
    int row;

    LOCATION *next;
    LOCATION *prev;
};
void add_to_list(LOCATION **head, LOCATION * g);
LOCATION *pop_from_list(LOCATION **head);
void remove_from_list(LOCATION **head, LOCATION *remove);

LOCATION *expansion_list = NULL;
bool target_found = false;



void clean_up(void) {
    // Clean up dynamically allocated grid
    for (int col = 0; col < num_columns; col++) {
        free(grid[col]);
    }
    free(grid);
}

void delay (void) {

    int i, j, k, sum;

    sum = 0;
    for (i=0;i<100;i++) 
        for (j=0;j<i;j++)
            for (k=0;k<30;k++) 
                sum = sum + i+j-k; 
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Need input file\n");
        exit(1);
    }

    char *file = argv[1];
    printf("Input file: %s\n", file);

    // initialize display with WHITE background, and define a clean_up function
    init_graphics("Some Example Graphics", WHITE, clean_up);
    init_world(0.,0.,1000.,1000.);

    parse_file(file);

    create_button("Window", "Proceed", proceed_button_func);
    drawscreen();
    event_loop(button_press, mouse_move, key_press, drawscreen);
    return 0;
}

void init_grid() {
    t_report report;
    report_structure(&report);
#ifdef DEBUG
    printf("width: %d, height: %d\n", report.top_width, report.top_height);
#endif

    float height = report.top_height;
    float width = report.top_width;
    float cell_height = height / num_rows;
    float cell_width = width / num_columns;

    // Allocate memory for the grid
    grid = (CELL **)my_malloc(num_columns * sizeof(CELL *));
    for (int col = 0; col < num_columns; col++) {
        grid[col] = (CELL *)my_malloc(num_rows * sizeof(CELL));
    }

    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
            grid[col][row].x1 = cell_width * col;
            grid[col][row].y1 = cell_height * row;
            grid[col][row].x2 = cell_width + cell_width * col;
            grid[col][row].y2 = cell_height + cell_height * row;
            grid[col][row].text_x = grid[col][row].x2 - cell_width / 2.;
            grid[col][row].text_y = grid[col][row].y2 - cell_height / 2.;
            grid[col][row].is_obstruction = false;
            grid[col][row].is_source = false;
            grid[col][row].is_sink = false;
            grid[col][row].is_routed = false;
            grid[col][row].is_wire = false;
            grid[col][row].wire_num = -1;
            grid[col][row].value = -1;
#ifdef DEBUG
            printf("grid[%d][%d] = (%f, %f) (%f, %f) (%f, %f)\n", col, row, grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2, grid[col][row].text_x, grid[col][row].text_y);
#endif
        }
    }
}

void draw_grid() {
    // Draw grid
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
            char text[10] = "";
            if (grid[col][row].is_obstruction) {
                // Draw obstruction
                setcolor(BLUE);
                fillrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                setcolor(BLACK);
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
            } else if (grid[col][row].wire_num != -1) {
                // Draw source and sinks
                setcolor(GREEN + grid[col][row].wire_num);
                fillrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                setcolor(BLACK);
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                if (grid[col][row].value != -1) {
                    sprintf(text, "%d", grid[col][row].value);
                } else {
                    sprintf(text, "%d (%s)", grid[col][row].wire_num, grid[col][row].is_source ? "src" : "snk");
                }
                drawtext(grid[col][row].text_x, grid[col][row].text_y, text, 150.);
            } else if (grid[col][row].value != -1) {
                // Draw expansion list
                setcolor(BLACK);
                sprintf(text, "%d", grid[col][row].value);
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                drawtext(grid[col][row].text_x, grid[col][row].text_y, text, 150.);
            } else {
                setcolor(BLACK);
                sprintf(text, "(%d, %d)", col, row);
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                drawtext(grid[col][row].text_x, grid[col][row].text_y, text, 150.);
            }
        }
    }
}

#define GRID_SIZE 0
#define NUM_OBSTRUCTED_CELLS 1

int parse_file(char *file) {
    FILE *fp;
    int ret = ERROR;

    if (file != NULL) {
        fp = fopen(file, "r");
        if (fp == NULL) {
            printf("Failed to open file: %s\n", file);
        } else {
            char *line;
            size_t len = 0;
            ssize_t read;

            int line_num = 0;
            int num_obstructed_cells = 0;
            int num_wires_to_route = 0;

            while ((read = getline(&line, &len, fp)) != -1) {
#ifdef DEBUG
                printf("parse_file[%d]: %s", line_num, line);
#endif
                switch (line_num) {
                    case GRID_SIZE:
                    {
                        const char delim[2] = " ";
                        char *token;

                        token = strtok(line, delim);
                        num_columns = atoi(token);
                        token = strtok(NULL, delim);
                        num_rows = atoi(token);
                        printf("num_rows: %d num_columns: %d\n", num_rows, num_columns);
                        init_grid();
                    }
                    break;
                    case NUM_OBSTRUCTED_CELLS:
                    {
                        num_obstructed_cells = atoi(line);
                        printf("num_obstructed_cells: %d\n", num_obstructed_cells);

                        while (line_num - NUM_OBSTRUCTED_CELLS < num_obstructed_cells && (read = getline(&line, &len, fp)) != -1) {
                            const char delim[2] = " ";
                            char *token;
                            token = strtok(line, delim);
                            int col = atoi(token);
                            token = strtok(NULL, delim);
                            int row = atoi(token);
                            printf("(%d, %d) is obstruction\n", col, row);
                            grid[col][row].is_obstruction = true;
                            line_num++;
                        }
                    }
                    break;
                    default: // NUM_WIRES_TO_ROUTE
                    {
                        num_wires_to_route = atoi(line);
                        int cur_wire = 0;
                        printf("num_wires_to_route: %d\n", num_wires_to_route);

                        while (cur_wire < num_wires_to_route && (read = getline(&line, &len, fp)) != -1) {
                            const char delim[2] = " ";
                            char *token;
                            token = strtok(line, delim);
                            int num_pins = atoi(token);
                            int idx = 0;
                            printf("Number of pins: %d\n", num_pins);

                            while (num_pins > 0) {
                                token = strtok(NULL, delim);
                                int col = atoi(token);
                                token = strtok(NULL, delim);
                                int row = atoi(token);

                                // First one is a source; rest are sinks
                                if (idx == 0) {
                                    grid[col][row].is_source = true;
                                    grid[col][row].wire_num = cur_wire;
                                    printf("(%d, %d) is a source\n", col, row);
                                    idx++;
                                } else {
                                    grid[col][row].is_sink = true;
                                    grid[col][row].wire_num = cur_wire;
                                    printf("(%d, %d) is a sink\n", col, row);
                                }

                                num_pins--;
                            }
                            cur_wire++;
                        }
                    }
                    break;
                }
                line_num++;
            }

            fclose(fp);
            if (line) {
                free(line);
            }
        }
    } else {
        printf("Invalid file!");
    }
    return ret;
}

void drawscreen() {
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

void button_press(float x, float y, int flags) {
    printf("User clicked a button at coordinates (%f, %f)\n", x, y);
}


void proceed_button_func(void (*drawscreen_ptr) (void)) {
    while (!target_found) {
    // Run the algorithm
    run_lee_moore_algo();
    drawscreen();
    delay();
    }
}

bool find_new_source() {
    bool found = false;
    for (int col = 1; col < num_columns; col++) {
        for (int row = 1; row < num_rows; row++) {
            // Find a new source to route
            if (grid[col][row].is_source && !grid[col][row].is_routed) {
                cur_src_col = col;
                cur_src_row = row;
                cur_wire_num = grid[col][row].wire_num;
                found = true;
                grid[col][row].is_routed = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }

    printf("New current source: (%d, %d) [%d]\n", cur_src_col, cur_src_row, cur_wire_num);
    return found;
}

LOCATION *find_smallest_value() {
    LOCATION *cur = expansion_list;
    LOCATION *smallest = NULL;
    int smallest_val = INT_MAX;

    while (cur != NULL) {
        if (grid[cur->col][cur->row].value < smallest_val) {
            smallest_val = grid[cur->col][cur->row].value;
            smallest = cur;
        }
        cur = cur->next;
    }

    if (smallest != NULL) {
        printf("Smallest cell in expansion_list: (%d, %d)\n", smallest->col, smallest->row);
        // Remove the smallest one from the expansion list
        remove_from_list(&expansion_list, smallest);
    } else {
        printf("ERROR: Cannot find the smallest cell in expansion_list\n");
    }

    return smallest;
}

void remove_from_list(LOCATION **head, LOCATION *remove) {
    LOCATION *cur = *head;

    while (cur != NULL) {
        if (cur->col == remove->col && cur->row == remove->row) {
            // Found the item in the list
            if (cur == *head) {
                // First item in the list
                *head = cur->next;
                if (cur->next != NULL) {
                    (*head)->prev = NULL;
                }
                cur->next = NULL;
                cur->prev = NULL;
            } else if (cur->next == NULL) {
                // Last item in the list
                cur->prev->next = NULL;
                cur->next = NULL;
                cur->prev = NULL;
            } else {
                // Middle of the list
                cur->next->prev = cur->prev;
                cur->prev->next = cur->next;
                cur->next = NULL;
                cur->prev = NULL;
            }
            break;
        }
        cur = cur->next;
    }
}

/**
 * See if coordinates (c, r) are valid. Only valid if c and r are both greater
 * than 0 and less than the bounds of the grid.
 */
bool is_valid_coordinates(int col, int row) {
    return (col >= 0 && row >=0 && col < num_columns && row < num_rows);
}

bool is_valid_neighbor(int col, int row) {
    return (is_valid_coordinates(col, row) &&
        !grid[col][row].is_obstruction &&
        grid[col][row].value == -1 &&
        !grid[col][row].is_wire);
}

LOCATION *make_location(int col, int row) {
    LOCATION *g = (LOCATION *)malloc(sizeof(LOCATION));
    g->col = col;
    g->row = row;
    g->next = NULL;
    g->prev = NULL;

    return g;
}

LOCATION *find_all_neighbors(int col, int row) {
    // Neighbors is deemed as the one on top, below, left, and right of (col, row)
    LOCATION *neighbors = NULL;
    int c, r;

#ifdef DEBUG
    printf("Find all neighbors for (%d, %d)\n", col, row);
#endif

    // Top
    c = col;
    r = row - 1;
    if (is_valid_neighbor(c, r)) {
        LOCATION *g = make_location(c, r);
        neighbors = g;
#ifdef DEBUG
        printf("Found top neighbor (%d, %d)\n", c, r);
#endif
    }

    // Bottom
    c = col;
    r = row + 1;
    if (is_valid_neighbor(c, r)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found bottom neighbor (%d, %d)\n", c, r);
#endif
    }

    // Left
    c = col - 1;
    r = row;
    if (is_valid_neighbor(c, r)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found left neighbor (%d, %d)\n", c, r);
#endif
    }

    // Right
    c = col + 1;
    r = row;
    if (is_valid_neighbor(c, r)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found right neighbor (%d, %d)\n", c, r);
#endif
    }
    return neighbors;
}

void run_lee_moore_algo() {
#ifdef DEBUG
    printf("Running lee-moore algo\n");
#endif

    // Really, if any is -1, they should all be -1s...
    if (cur_src_col == -1 || cur_src_row == -1 || cur_wire_num == -1) {
        if (!find_new_source()) {
            printf("Attempted all sources!\n");
            return;
        }
    }

    if (grid[cur_src_col][cur_src_row].value == -1) {
        // First step
        grid[cur_src_col][cur_src_row].value = 1;
        LOCATION *g = make_location(cur_src_col, cur_src_row);
        expansion_list = g;
        printf("Labeled source (%d, %d) as first step!\n", cur_src_col, cur_src_row);
        return;
    } else if (expansion_list != NULL) {
        int col, row;
        // Find the cell in the expansion list with the smallest value
        LOCATION *g = find_smallest_value();
        if (g != NULL) {
            if (grid[g->col][g->row].is_sink && grid[g->col][g->row].wire_num == grid[cur_src_col][cur_src_row].wire_num) {
                target_found = true;
                printf("Found the target (%d, %d)\n", g->col, g->row);
                // g is the target, done!
                return;
            }

            LOCATION *neighbors = find_all_neighbors(g->col, g->row);
            LOCATION *cur = NULL;
            while ((cur = pop_from_list(&neighbors)) != NULL) {
                col = cur->col;
                row = cur->row;
                // if neighbor is unlabelled
                if (grid[col][row].value == -1) {
                    // label it with the label of g + 1
                    grid[col][row].value = grid[g->col][g->row].value + 1;

                    printf("expansion_list: %p\n", expansion_list);

                    // add neighbor to expansion list
                    add_to_list(&expansion_list, cur);
                }
            }
        } else {
            printf("ERROR: cannot find smallest value...\n");
            return;
        }
    } else if (target_found == false) {
        // Loop has terminated (i.e. couldn't hit a target), then fail
        printf("WARNING: Failed to route src (%d, %d) on net %d\n", cur_src_col, cur_src_row, grid[cur_src_col][cur_src_row].wire_num);
        return;
    } else {
        // Traceback begins
        printf("Beginning traceback of src (%d, %d) on net %d\n", cur_src_col, cur_src_row, grid[cur_src_col][cur_src_row].wire_num);
    }
}

LOCATION *pop_from_list(LOCATION **head) {
    LOCATION *tmp = NULL;
    LOCATION *g = NULL;

    if (*head != NULL) {
        tmp = (*head)->next;
        if (tmp != NULL) {
            tmp->prev = NULL;
        }

        g = *head;
        g->next = NULL;
        g->prev = NULL;

        *head = tmp;
    }
#ifdef DEBUG
    if (g != NULL) {
        printf("Popped (%d, %d)\n", g->col, g->row);
    } else {
        printf("Didn't pop anything...\n");
    }
#endif

    return g;
}

void add_to_list(LOCATION **head, LOCATION * g) {
    LOCATION *cur = *head;

#ifdef DEBUG
    printf("Adding (%d, %d) to list\n", g->col, g->row);
#endif

    if (*head == NULL) {
        *head = g;
        return;
    }

    // Go to the end of the list
    while (cur->next != NULL) {
        cur = cur->next;
    }

    cur->next = g;
    g->prev = cur;
    g->next = NULL;
}

void mouse_move(float x, float y) {
    // function to handle mouse move event, the current mouse position in the current world coordinate
    // as defined as MAX_X and MAX_Y in init_world is returned
}

void key_press(int i) {
    // function to handle keyboard press event, the ASCII character is returned
}
