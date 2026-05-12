#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

struct pas_record_1 {
    int y;
};

struct pas_record_2 {
    int x;
    struct pas_record_1 inner;
};

struct pas_record_2 point;

int main() {
    point.x = 3;
    point.inner.y = point.x + 4;
    printf("%d", point.inner.y);
    return 0;
}
