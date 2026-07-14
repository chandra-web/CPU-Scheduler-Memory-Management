#include "../Clock/headers.h"


struct QueueNode;
typedef struct QueueNode* QueueNodePtr;

typedef struct QueueNode {
    process* nodeProcess;
    QueueNodePtr next;
} QueueNode;


typedef struct Queue {
    QueueNode* front;
} Queue;


Queue* createQueue() {
    Queue* pq = (Queue*)malloc(sizeof(Queue));
    if (pq == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    pq->front = NULL;
    return pq;
}

int isEmpty (Queue* q) {
    if (q->front == NULL)
        return 1;
    return 0;
}

void normalQenqueue(Queue* pq, process* newprocess) {
    // Create a new QueueNode
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->nodeProcess = newprocess;

    if (pq->front == NULL) {
        newNode->next = pq->front;
        pq->front = newNode;
    } else {
        QueueNode* current = pq->front;
        while (current->next != NULL) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
}


process* dequeue(Queue* pq) {
    if (pq->front == NULL) {
       // printf("Queue is empty\n");
        return NULL;
    }
    QueueNode* temp = pq->front;
    pq->front = pq->front->next;
    process* dequeuedProcess = temp->nodeProcess;
    temp = NULL;
    free(temp);
    return dequeuedProcess;
}


process* peek(Queue* pq) {
    if (pq->front == NULL) {
      //  printf("Queue is empty\n");
        return NULL;
    }
    return pq->front->nodeProcess;
}


void display(Queue* q) {
    QueueNode* current = q->front;
    if (current == NULL) {
        printf("Queue is empty\n");
        return;
    }
    printf("\nQueue:\n");
    while (current != NULL) {
        printf("Process ID: %d\n", current->nodeProcess->id);
        current = current->next;
    }
}

// Function to free the memory allocated for the priority queue
void freeQueue(Queue* pq) {
    QueueNode* current = pq->front;
    QueueNode* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(pq);
}

int countQueue(Queue* queue) {
    int count = 0;
    QueueNode* current = queue->front;
    while (current!= NULL) {
        count++;
        current = current->next;
    }
    return count;
}