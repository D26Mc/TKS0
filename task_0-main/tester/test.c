#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "../emergency_module.h"
#include "../emergency_module.c"

void test_basic_init(void) {
    EmergencyNode_class_init();
    EmergencyNode_t node;
    assert(EmergencyNode_init(&node) == 0);
    assert(node.emergency_counter == 0);
    for (int i = 0; i < NUM_EMERGENCY_BUFFER; ++i)
        assert(node.emergency_buffer[i] == 0);
}

void test_raise_and_solve(void) {
    EmergencyNode_class_init();
    EmergencyNode_t node;
    EmergencyNode_init(&node);
    assert(EmergencyNode_raise(&node, 3) == 0);
    assert(node.emergency_counter == 1);
    assert(EmergencyNode_is_emergency_state(&node));
    assert(EmergencyNode_solve(&node, 3) == 0);
    assert(node.emergency_counter == 0);
    assert(EmergencyNode_is_emergency_state(&node) == 0);
}

void test_out_of_range(void) {
    EmergencyNode_t node;
    EmergencyNode_init(&node);
    assert(EmergencyNode_raise(&node, 100) == -1);
    assert(EmergencyNode_solve(&node, 100) == -1);
}

void test_destroy(void) {
    EmergencyNode_t node;
    EmergencyNode_init(&node);
    EmergencyNode_raise(&node, 1);
    EmergencyNode_destroy(&node);
    assert(node.emergency_counter == 0);
}

#define NUM_THREADS 16
#define RAISES_PER_THREAD 100000

void* raise_thread(void* arg) {
    EmergencyNode_t* node = (EmergencyNode_t*)arg;
    for (int i = 0; i < RAISES_PER_THREAD; ++i) {
        EmergencyNode_raise(node, i % (NUM_EMERGENCY_BUFFER * 8));
        if (i % 20000 == 0) {
            printf("Thread %lu raised emergencies up to iteration %d\n", (unsigned long)pthread_self(), i);
            fflush(stdout);
        }
    }
    printf("Thread %lu finished raising emergencies.\n", (unsigned long)pthread_self());
    fflush(stdout);
    return NULL;
}

void test_multithread_same_node(void) {
    printf("Starting aggressive multithread test with %d threads, each raising %d emergencies.\n", NUM_THREADS, RAISES_PER_THREAD);
    fflush(stdout);
    EmergencyNode_class_init();

    EmergencyNode_t node;
    EmergencyNode_init(&node);

    pthread_t threads[NUM_THREADS];

    printf("Creating threads...\n");
    fflush(stdout);
    for (int i = 0; i < NUM_THREADS; ++i) {
        int ret = pthread_create(&threads[i], NULL, raise_thread, &node);
        if (ret != 0) {
            printf("Failed to create thread %d\n", i);
            fflush(stdout);
        } else {
            printf("Thread %d created successfully.\n", i);
            fflush(stdout);
        }
    }

    printf("Waiting for threads to finish...\n");
    fflush(stdout);
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
        printf("Thread %d has finished.\n", i);
        fflush(stdout);
    }

    printf("All threads finished.\n");
    printf("Emergencies in node after aggressive multithread test: %u\n", node.emergency_counter);
    printf("Expected maximum (buffer capacity): %d\n", NUM_EMERGENCY_BUFFER * 8);
    if (node.emergency_counter != NUM_EMERGENCY_BUFFER * 8) {
        printf("Race condition detected! Counter is incorrect.\n");
    } else {
        printf("Counter appears correct (but this may be coincidental).\n");
    }
    printf("Test complete.\n");
    fflush(stdout);

    assert(node.emergency_counter == NUM_EMERGENCY_BUFFER * 8);
}

int main(void) {
    printf("Running test_basic_init...\n");
    fflush(stdout);
    test_basic_init();
    printf("test_basic_init passed.\n\n");
    fflush(stdout);

    printf("Running test_raise_and_solve...\n");
    fflush(stdout);
    test_raise_and_solve();
    printf("test_raise_and_solve passed.\n\n");
    fflush(stdout);

    printf("Running test_out_of_range...\n");
    fflush(stdout);
    test_out_of_range();
    printf("test_out_of_range passed.\n\n");
    fflush(stdout);

    printf("Running test_multithread_same_node...\n");
    fflush(stdout);
    test_multithread_same_node();
    printf("test_multithread_same_node passed.\n\n");
    fflush(stdout);

    printf("Running test_destroy...\n");
    fflush(stdout);
    test_destroy();
    printf("test_destroy passed.\n\n");
    fflush(stdout);

    printf("All tests completed successfully!\n");
    fflush(stdout);
    return 0;
}
