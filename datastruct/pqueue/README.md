# pqueue.c

[Priority queue](https://en.wikipedia.org/wiki/Priority_queue) for C.

## Example

Create a queue of ints.

```c

int compare(const void *a, const void *b, void *udata) {
    if (*(int*)a < *(int*)b) return -1;
    if (*(int*)a > *(int*)b) return 1;
    return 0;
}

void main() {
    int items[] = {9, 5, 1, 3, 4, 2, 6, 8, 9, 2, 1};
    struct pqueue *queue = pqueue_new(sizeof(int), compare, NULL);

    for (int i = 0; i < sizeof(items)/sizeof(int); i++) {
        pqueue_push(queue, &items[i]);
    }

    const void *item = pqueue_pop(queue);
    while (item) {
        printf("%d\n", *(int*)item);
        item = pqueue_pop(queue);
    }
}

// OUTPUT:
// 1
// 1
// 2
// 2
// 3
// 4
// 5
// 6
// 8
// 9
// 9
```
