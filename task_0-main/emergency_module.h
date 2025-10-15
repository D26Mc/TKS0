#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "../emergency_module.h" // Include only the public interface

// === BASIC TESTS ===

// Test initialization of a single emergency node
void test_basic_init(void) {
    EmergencyNode_class_init(); // Initialize the module
    EmergencyNode_t node;
    assert(EmergencyNode_init(&node) == 0);
    assert(node.emergency_counter == 0);

    for (int i = 0; i < NUM_EMERGENCY_BUFFER; ++i) {
        assert(node.emergency_buffer[i] == 0); // Buffer should start empty
    }
}

// Test raising and solving emergencies
void test_raise_and_solve(void) {
    EmergencyNode_class_init();
    EmergencyNode_t node;
    EmergencyNode_init(&node);

    assert(EmergencyNode_raise(&node, 3) == 0); // Raise emergency ID 3
    assert(node.emergency_counter == 1);
    assert(EmergencyNode_is_emergency_state(&node)); // Check if emergency is active

    assert(EmergencyNode_solve(&node, 3) == 0); // Solve the emergency
    assert(node.emergency_counter == 0);
    assert(EmergencyNode_is_emergency_state(&node) == 0); // No emergency now
}

// Test handling of invalid emergency IDs
void test_out_of_range(void) {
    EmergencyNode_t node;
    EmergencyNode_init(&node);

    assert(EmergencyNode_raise(&node, 100) == -1); // Out of range
    assert(EmergencyNode_solve(&node, 100) == -1); // Out of range
}

// === SIMPLE MULTITHREAD TEST ===
#define NUM_THREADS 4
#define RAISES_PER_THREAD 1000

void* raise_thread(void* arg) {
    EmergencyNode_t* node = (EmergencyNode_t*)arg;
    for (int i = 0; i < RAISES_PER_THREAD; ++i) {
        EmergencyNode_raise(node, i % (NUM_EMERGENCY_BUFFER * 8));
    }
    return NULL;
}

void test_multithread_same_node(void) {
    EmergencyNode_class_init();
    EmergencyNode_t node;
    EmergencyNode_init(&node);

    pthread_t threads[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, raise_thread, &node);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Multithread test finished. Emergencies in node: %u\n", node.emergency_counter);
}

// === MAIN FUNCTION ===
int main(void) {
    printf("Running test_basic_init...\n");
    test_basic_init();
    printf("Passed test_basic_init!\n\n");

    printf("Running test_raise_and_solve...\n");
    test_raise_and_solve();
    printf("Passed test_raise_and_solve!\n\n");

    printf("Running test_out_of_range...\n");
    test_out_of_range();
    printf("Passed test_out_of_range!\n\n");

    printf("Running multithread test...\n");
    test_multithread_same_node();
    printf("Multithread test completed!\n\n");

    printf("All tests completed successfully!\n");
    return 0;
}
