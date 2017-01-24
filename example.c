#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "graphics.h"
#include "common.h"

//#define DEBUG
#define SUCCESS 0
#define ERROR -1

void button_press (float x, float y, int flags);
void drawscreen();
void proceed_button_func(void (*drawscreen_ptr) (void));
void proceed_fast_button_func(void (*drawscreen_ptr) (void));
void mouse_move (float x, float y);
void key_press (int i);
void init_grid();
int parse_file(char *file);
void run_lee_moore_algo();

bool done = false;
int num_rows = 0;
int num_columns = 0;
int num_sources = 0;
int num_sinks = 0;
int num_successful_sinks = 0;
int num_failed_sinks = 0;

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
    bool is_routed;         // true if we've tried routing this net
    bool is_wire;           // true if the cell is a wire
    int wire_num;           // wire number (i.e. the net number)
    int value;              // value of the lee-moore algo
} CELL;

CELL **grid;

int cur_src_col = -1;
int cur_src_row = -1;
int cur_sink_col = -1;
int cur_sink_row = -1;
int cur_trace_col = -1;
int cur_trace_row = -1;
int cur_wire_num = -1;

typedef struct LOCATION {
    int col;
    int row;

    LOCATION *next;
    LOCATION *prev;
} LOCATION;
void add_to_list(LOCATION **head, LOCATION * g);
LOCATION *pop_from_list(LOCATION **head);
void remove_from_list(LOCATION **head, LOCATION *remove);
LOCATION *make_location(int col, int row);
void find_all_sources();

#define MAX_NUM_RETRIES 25

LOCATION *expansion_list = NULL;
bool sink_found = false;
bool multiple_sink = false;
int num_retries = 0;
LOCATION *all_sources = NULL;
LOCATION *failed_sources_for_multisink = NULL;

typedef enum STATE {
    IDLE,
    EXPANSION,
    TRACEBACK
} STATE;

STATE cur_state;


void delay(void) {

    int i, j, k, sum;

    sum = 0;
    for (i=0;i<100;i++)
        for (j=0;j<i;j++)
            for (k=0;k<30;k++)
                sum = sum + i+j-k;
}


void clean_up(void) {
    // Clean up dynamically allocated grid
    for (int col = 0; col < num_columns; col++) {
        free(grid[col]);
    }
    free(grid);
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

    cur_state = IDLE;
    parse_file(file);

    find_all_sources();

    create_button("Window", "Go 1 Step", proceed_button_func);
    create_button("Window", "Go 1 State", proceed_fast_button_func);
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
                setcolor(DARKGREY + grid[col][row].wire_num);
                fillrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                setcolor(BLACK);
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
                if (grid[col][row].is_wire && !(grid[col][row].is_source || grid[col][row].is_sink)) {
                    sprintf(text, "w");
                } else if (grid[col][row].value != -1) {
                    sprintf(text, "%d", grid[col][row].value);
                } else {
                    sprintf(text, "%d_%s", grid[col][row].wire_num, grid[col][row].is_source ? "sc" : "sk");
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
                drawrect(grid[col][row].x1, grid[col][row].y1, grid[col][row].x2, grid[col][row].y2);
#ifdef DEBUG
                sprintf(text, "(%d, %d)", col, row);
                drawtext(grid[col][row].text_x, grid[col][row].text_y, text, 150.);
#endif
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
                                    num_sources++;
                                    idx++;
                                } else {
                                    grid[col][row].is_sink = true;
                                    grid[col][row].wire_num = cur_wire;
                                    printf("(%d, %d) is a sink\n", col, row);
                                    num_sinks++;
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

    printf("There are %d sources\n", num_sources);
    return ret;
}

void drawscreen() {
    clearscreen();  /* Should be first line of all drawscreens */
    draw_grid();
}

void button_press(float x, float y, int flags) {
    printf("User clicked a button at coordinates (%f, %f)\n", x, y);
}

void proceed_button_func(void (*drawscreen_ptr) (void)) {
    if (!done) {
        run_lee_moore_algo();
        drawscreen();
    } else {
        printf("Nothing else to do!\n");
    }
}

void proceed_fast_button_func(void (*drawscreen_ptr) (void)) {
    STATE state = cur_state;
    while (state == cur_state && !done) {
        run_lee_moore_algo();
        drawscreen();
        delay();
    }

    if (done) 
        printf("Nothing else to do!\n");
}


void find_all_sources() {
    printf("Finding all sources\n");
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
            if (grid[col][row].is_source) {
                LOCATION *loc = make_location(col, row);
                add_to_list(&all_sources, loc);
            }
        }
    }
}

bool find_new_source() {
    /*
    bool found = false;
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
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
    */
    bool found = false;
    LOCATION *loc = pop_from_list(&all_sources);
    if (loc != NULL) {
        int col = loc->col;
        int row = loc->row;
        if (!grid[col][row].is_routed) {
            cur_src_col = col;
            cur_src_row = row;
            cur_wire_num = grid[cur_src_col][cur_src_row].wire_num;
            grid[cur_src_col][cur_src_row].is_routed = true;
            found = true;
            printf("New current source: (%d, %d) [%d]\n", cur_src_col, cur_src_row, cur_wire_num);
        } else {
            printf("WARNING: Cannot find new source! (%d, %d) already routed!?\n", col, row);
        }
        free(loc);
    } else {
        done = true;
        printf("No more sources to route!\n");
    }
    return found;
}

void print_cell(int col, int row) {
    printf("(%d, %d): is_obstruction: %s, is_source: %s, is_sink: %s, is_routed: %s, is_wire: %s, wire_num: %d, value: %d\n",
        col, row,
        grid[col][row].is_obstruction ? "true" : "false",
        grid[col][row].is_source ? "true" : "false",
        grid[col][row].is_sink ? "true" : "false",
        grid[col][row].is_routed ? "true" : "false",
        grid[col][row].is_wire ? "true" : "false",
        grid[col][row].wire_num,
        grid[col][row].value);
}

#define ABS(x) (((x) < 0) ? -(x) : (x))

bool list_contains(LOCATION *list, int col, int row) {
    bool contains = false;
    LOCATION *cur = list;
    while (cur != NULL) {
        if (cur->col == col && cur->row == row) {
            printf("List contains: (%d, %d)\n", col, row);
            contains = true;
            break;
        }
        cur = cur->next;
    }
    return contains;
}

bool find_new_source_for_sink(int sink_col, int sink_row) {
    bool found = false;

    // The idea is to find the part of the wire that is closest to the sink
    int wire_num = grid[sink_col][sink_row].wire_num;
    int smallest_diff = INT_MAX;
    int closest_col = -1;
    int closest_row = -1;

    if (wire_num != -1) {
        for (int col = 0; col < num_columns; col++) {
            for (int row = 0; row < num_rows; row++) {
                if (grid[col][row].wire_num == wire_num &&
                    grid[col][row].is_wire &&
                    !(list_contains(failed_sources_for_multisink, col, row)) &&
                    !(grid[col][row].is_sink || grid[col][row].is_source)) {

                    // Found the correct wire. Now see if it's close to the sink
                    int diff = ABS(col - sink_col) + ABS(row - sink_row);
                    if (diff < smallest_diff) {
                        smallest_diff = diff;
                        closest_col = col;
                        closest_row = row;
                    }
                }
            }
        }
    }

    if (closest_col != -1 && closest_row != -1) {
        cur_src_col = closest_col;
        cur_src_row = closest_row;
        found = true;

        printf("Found (%d, %d) for sink (%d, %d)\n", cur_src_col, cur_src_row, sink_col, sink_row);
    } else {
        printf("Couldn't find anything...\n");
    }

    return found;
}

bool find_new_sink(int src_col, int src_row) {
    bool found = false;
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
#ifdef DEBUG
            if (grid[col][row].is_sink) {
                print_cell(col, row);
            }
#endif
            // Find a sink for the source (src_col, src_row)
            if (grid[col][row].is_sink &&
                !grid[col][row].is_routed &&
                !grid[col][row].is_wire &&
                grid[src_col][src_row].wire_num == grid[col][row].wire_num) {
                cur_sink_col = col;
                cur_sink_row = row;
                found = true;
                break;
            }
        }
        if (found) {
            printf("New current sink: (%d, %d) [%d]\n", cur_sink_col, cur_sink_row, cur_wire_num);
            break;
        }
    }

    printf("Found new sink? %d\n", found);

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

bool is_valid_neighbor(int col, int row, bool trace_back, int wire_num) {
    bool valid = false;
    if (trace_back) {
        valid = (is_valid_coordinates(col, row) && !grid[col][row].is_obstruction && grid[col][row].value != -1);
        if (valid && grid[col][row].is_wire && wire_num != -1) {
            // Only way it's valid is if the wire is the same wire_num
            if (!(grid[col][row].wire_num == wire_num)) {
                valid = false;
            }
        }
    } else {
        valid = (is_valid_coordinates(col, row) && !grid[col][row].is_obstruction && grid[col][row].value == -1 && !grid[col][row].is_wire);
        if (valid && (grid[col][row].is_sink || grid[col][row].is_source)) {
            if (!(grid[col][row].wire_num == wire_num)) {
                valid = false;
            }
        }
    }
    return valid;
}


void clear_failed_list() {
    if (failed_sources_for_multisink != NULL) {
        LOCATION *cur = failed_sources_for_multisink;
        while (cur != NULL) {
            LOCATION *next = cur->next;
            free(cur);
            cur = next;
        }

        failed_sources_for_multisink = NULL;
    }
}

void clear_expansion_list() {
    if (expansion_list != NULL) {
        LOCATION *cur = expansion_list;
        while (cur != NULL) {
            LOCATION *next = cur->next;
            free(cur);
            cur = next;
        }

        expansion_list = NULL;
    }
}

void reset_current() {
    cur_src_col = -1;
    cur_src_row = -1;
    cur_sink_col = -1;
    cur_sink_row = -1;
    cur_trace_col = -1;
    cur_trace_row = -1;
    cur_wire_num = -1;
    cur_state = IDLE;

    sink_found = false;
    multiple_sink = false;
    num_retries = 0;
    clear_expansion_list();
    clear_failed_list();

}

void reset_all() {
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
            grid[col][row].is_routed = false;
            grid[col][row].is_wire = false;
            if (!(grid[col][row].is_source || grid[col][row].is_sink)) {
                grid[col][row].wire_num = -1;
            }
            grid[col][row].value = -1;
        }
    }

    cur_src_col = -1;
    cur_src_row = -1;
    cur_sink_col = -1;
    cur_sink_row = -1;
    cur_trace_col = -1;
    cur_trace_row = -1;
    cur_wire_num = -1;
    cur_state = IDLE;

    sink_found = false;
    multiple_sink = false;

    clear_expansion_list();
    clear_failed_list();
}

void reset_grid() {
    for (int col = 0; col < num_columns; col++) {
        for (int row = 0; row < num_rows; row++) {
            grid[col][row].value = -1;
        }
    }
}

LOCATION *make_location(int col, int row) {
    LOCATION *g = (LOCATION *)malloc(sizeof(LOCATION));
    g->col = col;
    g->row = row;
    g->next = NULL;
    g->prev = NULL;

    return g;
}

typedef enum DIRECTION {
    TOP,
    LEFT,
    RIGHT,
    BOTTOM
} DIRECTION;

LOCATION *create_neighbors(int col, int row, bool trace_back, int wire_num, DIRECTION d) {
    int c, r;
    char *text = "";
    LOCATION *g = NULL;

    switch (d) {
        case TOP:
            c = col;
            r = row - 1;
            text = "TOP";
            break;
        case BOTTOM:
            c = col;
            r = row + 1;
            text = "BOTTOM";
            break;
        case LEFT:
            c = col - 1;
            r = row;
            text = "LEFT";
            break;
        case RIGHT:
            c = col + 1;
            r = row;
            text = "RIGHT";
            break;
    }
    printf("Creating neighbors for (%d, %d) direction %s\n", col, row, text);


    if (is_valid_neighbor(c, r, trace_back, wire_num)) {
        g = make_location(c, r);
        printf("Found %s neighbor (%d, %d)\n", text, c, r);
    }

    return g;
}

LOCATION *find_all_neighbors(int col, int row, bool trace_back, int wire_num) {
    // Neighbors is deemed as the one on top, below, left, and right of (col, row)
    LOCATION *neighbors = NULL;
    printf("Find all neighbors for (%d, %d)\n", col, row);

    for (int i = 0; i < 4; i++) {
        DIRECTION d = (DIRECTION)((num_retries + i) % 4);
        LOCATION *g = create_neighbors(col, row, trace_back, wire_num, d);
        if (g != NULL) {
            add_to_list(&neighbors, g);
        }
    }

/*
    // Top
    c = col;
    r = row - 1;
    if (is_valid_neighbor(c, r, trace_back, wire_num)) {
        LOCATION *g = make_location(c, r);
        neighbors = g;
#ifdef DEBUG
        printf("Found top neighbor (%d, %d)\n", c, r);
#endif
    }

    // Bottom
    c = col;
    r = row + 1;
    if (is_valid_neighbor(c, r, trace_back, wire_num)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found bottom neighbor (%d, %d)\n", c, r);
#endif
    }

    // Left
    c = col - 1;
    r = row;
    if (is_valid_neighbor(c, r, trace_back, wire_num)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found left neighbor (%d, %d)\n", c, r);
#endif
    }

    // Right
    c = col + 1;
    r = row;
    if (is_valid_neighbor(c, r, trace_back, wire_num)) {
        LOCATION *g = make_location(c, r);
        add_to_list(&neighbors, g);
#ifdef DEBUG
        printf("Found right neighbor (%d, %d)\n", c, r);
#endif
    }
*/
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

        if (!find_new_sink(cur_src_col, cur_src_row)) {
            printf("ERROR: Cannot find sink for (%d, %d) [%d]\n", cur_src_col, cur_src_row, cur_wire_num);
        }
    }

    if (grid[cur_src_col][cur_src_row].value == -1) {
        // First step
        grid[cur_src_col][cur_src_row].value = 1;
        LOCATION *g = make_location(cur_src_col, cur_src_row);
        expansion_list = g;
        printf("Labeled source (%d, %d) as first step!\n", cur_src_col, cur_src_row);
        cur_state = EXPANSION;
        return;
    } else if (expansion_list != NULL && !sink_found) {
        int col, row;
        // Find the cell in the expansion list with the smallest value
        LOCATION *g = find_smallest_value();
        if (g != NULL) {
            // Check to see if g is the sink. If so, then we're done
            if (g->col == cur_sink_col && g->row == cur_sink_row) {
                sink_found = true;
                printf("Found the sink (%d, %d)\n", g->col, g->row);
                free(g);
                return;
            }

            LOCATION *neighbors = find_all_neighbors(g->col, g->row, false, cur_wire_num);
            LOCATION *cur = NULL;
            while ((cur = pop_from_list(&neighbors)) != NULL) {
                col = cur->col;
                row = cur->row;
                // if neighbor is unlabelled
                if (grid[col][row].value == -1) {
                    // label it with the label of g + 1
                    grid[col][row].value = grid[g->col][g->row].value + 1;

                    // Check to see if we have expanded to sink. If so, then we're done
                    if (col == cur_sink_col && row == cur_sink_row) {
                        sink_found = true;
                        printf("Found the sink (%d, %d)\n", g->col, g->row);
                        free(g);
                        return;
                    }

                    // add neighbor to expansion list
                    add_to_list(&expansion_list, cur);
                }
            }

            // Don't need the LOCATION anymore. Cleanup
            free(g);
        } else {
            printf("ERROR: cannot find smallest value...\n");
            return;
        }
    } else if (sink_found == false) {
        // Loop has terminated (i.e. couldn't hit a sink), then fail
        printf("WARNING: Failed to route src (%d, %d) on net %d\n", cur_src_col, cur_src_row, grid[cur_src_col][cur_src_row].wire_num);
        printf("Number of retries: %d\n", num_retries);
        if (num_retries < MAX_NUM_RETRIES) {
            if (multiple_sink) {
                printf("multiple sink\n");
                // Try another "source"
                reset_grid();
                sink_found = false;
                clear_expansion_list();
                cur_state = IDLE;
                cur_trace_col = -1;
                cur_trace_row = -1;
                num_retries++;

                LOCATION *failed = make_location(cur_src_col, cur_src_row);
                add_to_list(&failed_sources_for_multisink, failed);

                // Need to find a new cur_src_col and new cur_src_row to route to the new sink
                find_new_source_for_sink(cur_sink_col, cur_sink_row);
            } else {
                printf("Current state: %d\n", cur_state);
                if (cur_state == EXPANSION) {
                    printf("Failed during expansion; Ripping up all previous nets\n");
                    // rip-up all
                    // reset_all();
                } else if (cur_state == TRACEBACK) {
                    printf("Failed during traceback\n");
                }
                num_retries++;
            }
        } else {
            printf("ERROR: Reached number of retries! Giving up\n");
            num_failed_sinks++;
            num_retries = 0;
            cur_state = IDLE;
            reset_grid();
            reset_current();
        }
        return;
    } else {
        // Traceback
        printf("Traceback of source (%d, %d) on net %d\n", cur_src_col, cur_src_row, grid[cur_src_col][cur_src_row].wire_num);

        cur_state = TRACEBACK;

        // Start at sink
        if (cur_trace_col == -1 || cur_trace_row == -1) {
            cur_trace_col = cur_sink_col;
            cur_trace_row = cur_sink_row;

            grid[cur_trace_col][cur_trace_row].is_wire = true;
            grid[cur_trace_col][cur_trace_row].is_routed = true;
            return;
        }

        if (cur_trace_col == cur_src_col && cur_trace_row == cur_src_row) {
            printf("Successfully finished traceback of (%d, %d) on net %d\n", cur_src_col, cur_src_row, grid[cur_src_col][cur_src_row].wire_num);
            grid[cur_trace_col][cur_trace_row].is_wire = true;

            num_successful_sinks++;

            // Check to see if there are more sinks for this net
            if (find_new_sink(cur_src_col, cur_src_row)) {
                multiple_sink = true;
                // More work to do! We need to route to the new sink
                printf("There is more work to be done! Found new sink for this source\n");
                reset_grid();
                sink_found = false;
                clear_expansion_list();
                clear_failed_list();
                cur_state = IDLE;
                cur_trace_col = -1;
                cur_trace_row = -1;

                // Need to find a new cur_src_col and new cur_src_row to route to the new sink
                find_new_source_for_sink(cur_sink_col, cur_sink_row);
            } else {
                printf("We are done! Finished routing all sinks for source (%d, %d)\n", cur_src_col, cur_src_row);
                printf("Number of sources: %d; Number of sinks: %d; Number of successful sinks: %d Number of failed sinks: %d\n",
                    num_sources, num_sinks, num_successful_sinks, num_failed_sinks);
                // We are done, reset counters and states
                reset_grid();
                reset_current();
            }
            return;
        }

        int cur_value = grid[cur_trace_col][cur_trace_row].value;

        LOCATION *neighbors = find_all_neighbors(cur_trace_col, cur_trace_row, true, cur_wire_num);
        LOCATION *cur = NULL;
        int col = -1;
        int row = -1;

        while ((cur = pop_from_list(&neighbors)) != NULL) {
            col = cur->col;
            row = cur->row;

            if (grid[col][row].value == cur_value - 1) {
#ifdef DEBUG
                printf("Found next cell to wire: (%d, %d)\n", col, row);
#endif
                break;
            }
        }

        if (col == -1 || row == -1) {
            printf("ERROR: Could not find the next cell to route!\n");
            cur_state = IDLE;
            return;
        }

        grid[col][row].is_wire = true;
        grid[col][row].wire_num = grid[cur_src_col][cur_src_row].wire_num;
        cur_trace_col = col;
        cur_trace_row = row;
        printf("Current trace (%d, %d)\n", cur_trace_col, cur_trace_row);
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

    printf("Adding (%d, %d) to list\n", g->col, g->row);

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
