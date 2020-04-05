//
// Created by diego on 19/3/20.
//

#include <assert.h>
#include "../queue.h"

int main() {
    queue *q = queue_create(20);

    assert(queue_isempty(q) == 1);

    for (int i = 0; i < 20; i++) {
        queue_add(q, i);
    }

    for (int i = 0; i < 20; i++) {
        assert(queue_pop(q) == i);
    }

    assert(queue_isempty(q) == 1);

    queue_free(q);
}