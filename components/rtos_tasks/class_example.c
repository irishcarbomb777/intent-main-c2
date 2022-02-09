#include <stdio.h>
#include <math.h>
#include <string.h>
#include <uchar.h>

/**
 * Define Shape class
 */
typedef struct Shape Shape;
struct Shape {
    /**
     * Variables header...
     */
    double width, height, currentArea;

    /**
     * Functions header...
     */
    double (*area)(Shape *shape);
};

/**
 * Functions
 */
double calc(Shape *shape) {
        return shape->width * shape->height;
}

/**
 * Constructor
 */
Shape _Shape(int width, int height) {
    Shape s;

    s.width = width;
    s.height = height;
    s.currentArea = calc(&s);
    s.area = calc;

    return s;
}

/********************************************/

int main() {
    Shape s1 = _Shape(5, 6);
    s1.width = 5.35;
    s1.height = 12.5462;

    printf("Hello World\n\n");

    printf("User.width = %f\n", s1.width);
    printf("User.height = %f\n", s1.height);
    printf("User.area = %f\n\n", s1.area(&s1));
    printf("User.currentArea = %f\n", s1.currentArea);
    printf("Made with \xe2\x99\xa5 \n");

    return 0;
};
