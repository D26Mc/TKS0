#include "./emergency_module.h"
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

//private
//GLOBAL LED (only one)
typedef atomic_ushort gpio; //creates a type alias 
//represents an atomic variable based on an unsigned short.
gpio emergency_led; //1 = on, 0 = off.




static struct{
  atomic_flag lock; //serves as a minimal synchronization mechanism.
  //0=free, 1=busy
  uint8_t excepion_counter; //counts how many entities (nodes) have active emergencies
  uint8_t init_done:1; //1-bit flag that indicates if the module has been initialized.
}EXCEPTION_COUNTER; //global module state




//TURNS ON THE LED
static inline void _hw_raise_emergency(void) //synchronize LED activation with global changes.
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //checks if the thread is busy; if not, occupies it by setting it to 1
  atomic_store(&emergency_led, 1); //Sets the global LED to 1 (on)
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //frees the thread by setting it to 0
}

//INCREASES THE EMERGENCY COUNTER
static void _increase_global_emergency_counter(void) 
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //checks if the thread is busy; if not, occupies it by setting it to 1
  EXCEPTION_COUNTER.excepion_counter++; //increments excepion_counter (number of nodes with at least one emergency)
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //frees the thread by setting it to 0
}

static void _solved_module_exception_state(void)__attribute__((__unused__));

//DECREASES THE EMERGENCY COUNTER
static void _solved_module_exception_state(void)
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //checks if the thread is busy; if not, occupies it by setting it to 1
  EXCEPTION_COUNTER.excepion_counter--; //decrements excepion_counter.
  if (!EXCEPTION_COUNTER.excepion_counter) //If the global counter becomes 0 → turns off the LED (emergency_led = 0).
  {
    atomic_store(&emergency_led, 0); //turns off the global LED = 0
  }
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //frees the thread by setting it to 0
}

static uint8_t read_globla_emergency_couner(void)__attribute__((__unused__));

//READS THE COUNTER IN A PROTECTED WAY AND RETURNS ITS VALUE
static uint8_t read_globla_emergency_couner(void) 
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //checks if the thread is busy; if not, occupies it by setting it to 1
  const uint8_t res= EXCEPTION_COUNTER.excepion_counter; 
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //frees the thread by setting it to 0

  return res;
}







//public
// INITIALIZES
int8_t EmergencyNode_class_init(void)
{
  if (EXCEPTION_COUNTER.init_done) //If init_done is already 1 → return -1 (already initialized).
  {
    return -1;
  }
  atomic_store(&emergency_led, 0); //turns off the LED
  EXCEPTION_COUNTER.init_done=1; //sets init_done = 1
  EXCEPTION_COUNTER.excepion_counter=0; //resets the global counter

  return 0;
}
// does not initialize EXCEPTION_COUNTER.lock (atomic_flag). 
// Also not thread-safe if two threads call this function at the same time: 
// two concurrent calls may pass the init_done check and overwrite the state.

//RESETS THE PASSED EmergencyNode_t STRUCTURE (buffer and local counter).
int8_t EmergencyNode_init(EmergencyNode_t* const restrict p_self) //p_self pointer to the initial node
{
  memset(p_self, 0, sizeof(*p_self)); //fills a memory area with a specific value.
  return 0;
}

// RAISES AN EMERGENCY
int8_t EmergencyNode_raise(EmergencyNode_t* const restrict p_self, const uint8_t exeception)
{ //Calculates which byte and which bit in the bitmap correspond to the exeception
  const uint8_t exception_byte = exeception/8;  //in which of the 8 bytes the bit is
  const uint8_t exception_bit = exeception % 8; //in which bit position within the byte

  if (exeception >= NUM_EMERGENCY_BUFFER*8) //If exeception is out of range → -1.
  {
    return -1;
  }

  const uint8_t old_emergency_bit = (p_self->emergency_buffer[exception_byte] >> exception_bit) & 0x01; //Reads the old bit; then sets the new bit in the bitmap
  p_self->emergency_buffer[exception_byte] |= 1 << exception_bit; //	Bitwise OR operation to turn on the bit corresponding to the emergency.

  if (!old_emergency_bit) //If the bit was not already set:
  {
    if(!p_self->emergency_counter) { //the node had no other emergencies
      _increase_global_emergency_counter(); //to increase the global counter 
    }
    p_self->emergency_counter++; 
  }

  _hw_raise_emergency(); //turns on the LED (with lock)

  return 0;
}

// SOLVES AN EMERGENCY
int8_t EmergencyNode_solve(EmergencyNode_t* const restrict p_self, const uint8_t exeception)
{
  const uint8_t exception_byte = exeception/8;
  const uint8_t exception_bit = exeception % 8;

  if (exeception >= NUM_EMERGENCY_BUFFER * 8)
  { //if the exception is outside the buffer --> -1
    return -1;
  }

  if (p_self->emergency_buffer[exception_byte] &  (1 << exception_bit)) 
  { //Checks if the bit is set; if yes: clears it
    p_self->emergency_buffer[exception_byte] ^= (1 << exception_bit); //resolves the emergency
    p_self->emergency_counter--; //decrements the local counter,
    if (!p_self->emergency_counter)
    { //if it has no more active emergencies
      _solved_module_exception_state();
    }
  }

  return 0;
}

//CHECKS IF THERE ARE ACTIVE EMERGENCIES
int8_t EmergencyNode_is_emergency_state(const EmergencyNode_t* const restrict p_self)
{ //Returns non-zero if this node has emergencies or if the global counter > 0 (which means some node has an active emergency).
  return p_self->emergency_counter || read_globla_emergency_couner();
  // 0 → no emergency in this node and no node has active emergencies
	// 1 (or non-zero) → at least one active emergency, local or in another node
}

//DESTROYS NODE
int8_t EmergencyNode_destroy(EmergencyNode_t* const restrict p_self)
{ //If the node still has active emergencies
  if (p_self->emergency_counter)
  {
    _solved_module_exception_state(); //to decrement the global counter
  }

  memset(p_self, 0, sizeof(*p_self)); //resets the structure
  return 0;
}


/*
Not protected (race risk):
	•	Access and modification to per-node structures (p_self->emergency_buffer and p_self->emergency_counter) are not protected: therefore, if two different threads manipulate the same EmergencyNode_t simultaneously, race conditions occur (simultaneous reads/writes).
	•	EmergencyNode_class_init is not atomically protected: two threads calling it together can create inconsistent state.
	•	EXCEPTION_COUNTER.lock might not be properly initialized: if uninitialized, atomic_flag_test_and_set has undefined behavior.

*/
