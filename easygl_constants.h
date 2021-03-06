#ifndef EASYGL_CONSTANTS_H
#define EASYGL_CONSTANTS_H

enum color_types {WHITE, BLACK, BLUE, DARKGREY, LIGHTGREY, GREEN, YELLOW,
CYAN, RED, DARKGREEN, MAGENTA, NUM_COLOR};

enum line_types {SOLID, DASHED};

#define MAXPTS 100    /* Maximum number of points drawable by fillpoly */

typedef struct {
    float x; 
    float y;
} t_point; /* Used in calls to fillpoly */

#endif // EASYGL_CONSTANTS_H
