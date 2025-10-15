#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "../emergency_module.h"
#include "../emergency_module.c" // (direct include for testing internals)

// === BASIC TESTS ===

void test_basic_init(void) {
    // Test that a node initializes correctly.
    EmergencyNode_class_init(); // Initialize the global module
    EmergencyNode_t node;
    assert(EmergencyNode_init(&node) == 0); // Node init should succeed
    assert(node.emergency_counter == 0);    // Counter must start at 0
    for (int i = 0; i < NUM_EMERGENCY_BUFFER; ++i)
        assert(node.emergency_buffer[i] == 0); // All buffer bytes cleared
}

void test_raise_and_solve(void) {
    // Test that raising and solving an emergency updates counters properly.
    EmergencyNode_class_init();
    EmergencyNode_t node;
    EmergencyNode_init(&node);

    assert(EmergencyNode_raise(&node, 3) == 0); // Raise emergency ID 3
    assert(node.emergency_counter == 1);        // One active emergency
    assert(EmergencyNode_is_emergency_state(&node)); // System reports emergency

    assert(EmergencyNode_solve(&node, 3) == 0); // Solve it
    assert(node.emergency_counter == 0);        // Back to normal
    assert(EmergencyNode_is_emergency_state(&node) == 0); // No emergency now
}

void test_out_of_range(void) {
    // Test handling of invalid emergency IDs (out of buffer range).
    EmergencyNode_t node;
    EmergencyNode_init(&node);
    assert(EmergencyNode_raise(&node, 100) == -1); // Out of range → error
    assert(EmergencyNode_solve(&node, 100) == -1); // Out of range → error
}

void test_destroy(void) {
    // Test that destroy resets the node and updates global state.
    EmergencyNode_t node;
    EmergencyNode_init(&node);
    EmergencyNode_raise(&node, 1); // Simulate active emergency
    EmergencyNode_destroy(&node);  // Should clear it
    assert(node.emergency_counter == 0); // Node cleared
}

// === MULTITHREADING TESTS ===

#define NUM_THREADS 16
#define RAISES_PER_THREAD 100000

// Function executed by each thread: repeatedly raises emergencies
void* raise_thread(void* arg) {
    EmergencyNode_t* node = (EmergencyNode_t*)arg;
    for (int i = 0; i < RAISES_PER_THREAD; ++i) {
        EmergencyNode_raise(node, i % (NUM_EMERGENCY_BUFFER * 8));
        // Print progress occasionally
        if (i % 20000 == 0) {
            printf("Thread %lu raised emergencies up to iteration %d\n",
                   (unsigned long)pthread_self(), i);
            fflush(stdout);
        }
    }
    printf("Thread %lu finished raising emergencies.\n", (unsigned long)pthread_self());
    fflush(stdout);
    return NULL;
}

void test_multithread_same_node(void) {
    // Stress test: many threads operate on the same EmergencyNode_t
    // Used to detect race conditions (non-thread-safe accesses)
    printf("Starting aggressive multithread test with %d threads, each raising %d emergencies.\n",
           NUM_THREADS, RAISES_PER_THREAD);
    fflush(stdout);

    EmergencyNode_class_init();
    EmergencyNode_t node;
    EmergencyNode_init(&node);

    pthread_t threads[NUM_THREADS];

    // Create all threads
    printf("Creating threads...\n");
    fflush(stdout);
    for (int i = 0; i < NUM_THREADS; ++i) {
        int ret = pthread_create(&threads[i], NULL, raise_thread, &node);
        if (ret != 0)
            printf("Failed to create thread %d\n", i);
        else
            printf("Thread %d created successfully.\n", i);
        fflush(stdout);
    }

    // Wait for all threads to complete
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

    // Check if race condition affected the counter
    if (node.emergency_counter != NUM_EMERGENCY_BUFFER * 8) {
        printf("Race condition detected! Counter is incorrect.\n");
    } else {
        printf("Counter appears correct (but this may be coincidental).\n");
    }
    printf("Test complete.\n");
    fflush(stdout);

    // Assert ensures race conditions fail the test visibly
    assert(node.emergency_counter == NUM_EMERGENCY_BUFFER * 8);
}

// === MAIN FUNCTION ===

int main(void) {
    // Run each test in sequence with clear output
    printf("Running test_basic_init...\n");
    fflush(stdout);
    test_basic_init();
    printf("test_basic_init passed.\n\n");

    printf("Running test_raise_and_solve...\n");
    fflush(stdout);
    test_raise_and_solve();
    printf("test_raise_and_solve passed.\n\n");

    printf("Running test_out_of_range...\n");
    fflush(stdout);
    test_out_of_range();
    printf("test_out_of_range passed.\n\n");

    printf("Running test_multithread_same_node...\n");
    fflush(stdout);
    test_multithread_same_node();
    printf("test_multithread_same_node passed.\n\n");

    printf("Running test_destroy...\n");
    fflush(stdout);
    test_destroy();
    printf("test_destroy passed.\n\n");

    printf("All tests completed successfully!\n");
    fflush(stdout);
    return 0;
}
