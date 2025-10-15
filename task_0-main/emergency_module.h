#ifndef __EMERGENCY_MODULE__ //header guard: evita che il file venga incluso più volte dallo stesso sorgente 
#define __EMERGENCY_MODULE__

#include <stdint.h>

#define NUM_EMERGENCY_BUFFER 8 //Definisce la dimensione dell’array emergency_buffer.
// ogni nodo può avere 64 emergenze

// rappresenta un nodo di emergenza indipendente, che può essere gestito separatamente dagli altri nodi.
typedef struct { 
  uint8_t emergency_buffer[NUM_EMERGENCY_BUFFER]; //bitmap di 64 bit
  uint32_t emergency_counter; //contatore delle emergenze attive in questo nodo.
}EmergencyNode_t; //nodo di emergenza indipendente

int8_t EmergencyNode_class_init(void); 
//Inizializza le variabili globali del modulo 

int8_t
EmergencyNode_init(EmergencyNode_t* const restrict)__attribute__((__nonnull__(1)));
//Azzera tutti i bit del buffer e il contatore locale. Nodo pronto a ricevere emergenze.

int8_t
EmergencyNode_raise(EmergencyNode_t* const restrict, const uint8_t exeception)__attribute__((__nonnull__(1)));
//Imposta il bit corrispondente nell’emergency_buffer. Aggiorna il contatore locale e, se necessario, quello globale. Accende il LED globale.

int8_t
EmergencyNode_solve(EmergencyNode_t* const restrict, const uint8_t exeception)__attribute__((__nonnull__(1)));
//Resetta il bit corrispondente nel buffer. Decrementa il contatore locale.

int8_t
EmergencyNode_is_emergency_state(const EmergencyNode_t* const restrict) __attribute__((__nonnull__(1)));
//verifica se ha emergenze attive

int8_t
EmergencyNode_destroy(EmergencyNode_t* const restrict)__attribute__((__nonnull__(1)));
//elimina il nodo

#endif // !__EMERGENCY_MODULE__
