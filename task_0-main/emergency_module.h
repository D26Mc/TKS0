#ifndef __EMERGENCY_MODULE__ //header guard: prevents this file from being included multiple times in the same source
#define __EMERGENCY_MODULE__

#include <stdint.h>

#define NUM_EMERGENCY_BUFFER 8 //Defines the size of the emergency_buffer array.
// each node can have 64 emergencies

// represents an independent emergency node, which can be managed separately from other nodes.
typedef struct { 
  uint8_t emergency_buffer[NUM_EMERGENCY_BUFFER]; //64-bit bitmap
  uint32_t emergency_counter; //counter of active emergencies in this node.
}EmergencyNode_t; //independent emergency node

int8_t EmergencyNode_class_init(void); 
//Initializes the module’s global variables

int8_t
EmergencyNode_init(EmergencyNode_t* const restrict)__attribute__((__nonnull__(1)));
//Clears all bits in the buffer and the local counter. Node ready to receive emergencies.

int8_t
EmergencyNode_raise(EmergencyNode_t* const restrict, const uint8_t exeception)__attribute__((__nonnull__(1)));
//Sets the corresponding bit in the emergency_buffer. Updates the local counter and, if needed, the global one. Turns on the global LED.

int8_t
EmergencyNode_solve(EmergencyNode_t* const restrict, const uint8_t exeception)__attribute__((__nonnull__(1)));
//Resets the corresponding bit in the buffer. Decrements the local counter.

int8_t
EmergencyNode_is_emergency_state(const EmergencyNode_t* const restrict) __attribute__((__nonnull__(1)));
//Checks whether there are active emergencies

int8_t
EmergencyNode_destroy(EmergencyNode_t* const restrict)__attribute__((__nonnull__(1)));
//Deletes the node

#endif // !__EMERGENCY_MODULE__
