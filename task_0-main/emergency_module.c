#include "./emergency_module.h"
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

//private
//LED GLOBALE (uno solo)
typedef atomic_ushort gpio; //Questa riga crea un alias di tipo 
//rappresenta una variabile atomica basata su un unsigned short.
gpio emergency_led; //1 = acceso, 0 = spento.




static struct{
  atomic_flag lock; //serve come meccanismo di sincronizzazione minimale, chiamato anche spinlock. 
  // può essere 0=libero, 1=occupato
  uint8_t excepion_counter; //conta quante entità (nodi) hanno emergenze attive
  uint8_t init_done:1; //flag 1 bit che segnala se il modulo è stato inizializzato.
}EXCEPTION_COUNTER; //stato globale del modulo




//IMPOSTSA ACCESO IL LED
static inline void _hw_raise_emergency(void) //sincronizzare l’accensione del led con le modifiche globali.
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //controlla se il thread è occupato, in caso lo occupa settandolo a 1
  atomic_store(&emergency_led, 1); //Imposta il led globale a 1 (accesо)
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //libera il thread settandolo a 0
}

//INCREMENTA IL COUNTER EMERGENZE
static void _increase_global_emergency_counter(void) 
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //controlla se il thread è occupato, in caso lo occupa settandolo a 1
  EXCEPTION_COUNTER.excepion_counter++; // incrementa excepion_counter (numero di nodi con almeno 1 emergenza)
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //libera il thread settandolo a 0
}

static void _solved_module_exception_state(void)__attribute__((__unused__));

//DECREMENTA IL COUNTER EMERGENZE
static void _solved_module_exception_state(void)
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //controlla se il thread è occupato, in caso lo occupa settandolo a 1
  EXCEPTION_COUNTER.excepion_counter--; //decrementa excepion_counter.
  if (!EXCEPTION_COUNTER.excepion_counter) //Se il contatore globale diventa 0 → spegne il led (emergency_led = 0).
  {
    atomic_store(&emergency_led, 0); //spegne il led globale = 0
  }
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //libera il thread settandolo a 0
}

static uint8_t read_globla_emergency_couner(void)__attribute__((__unused__));

//LEGGE in modo protetto IL VALORE DEL CONTATORE E LO RESTITUISCE
static uint8_t read_globla_emergency_couner(void) 
{
  while (atomic_flag_test_and_set(&EXCEPTION_COUNTER.lock)); //controlla se il thread è occupato, in caso lo occupa settandolo a 1
  const uint8_t res= EXCEPTION_COUNTER.excepion_counter; 
  atomic_flag_clear(&EXCEPTION_COUNTER.lock); //libera il thread settandolo a 0

  return res;
}







//public
// INIZIALIZZA
int8_t EmergencyNode_class_init(void)
{
  if (EXCEPTION_COUNTER.init_done) //Se init_done è già 1 → ritorna -1 (già inizializzato).
  {
    return -1;
  }
  atomic_store(&emergency_led, 0); //spegne il led
  EXCEPTION_COUNTER.init_done=1; //setta init_done = 1
  EXCEPTION_COUNTER.excepion_counter=0; //azzera il contatore globale

  return 0;
}
// non inizializza EXCEPTION_COUNTER.lock (atomic_flag). 
// In più non è thread-safe se due thread chiamano questa funzione contemporaneamente: 
// due chiamate concorrenti possono passare il check init_done e sovrascrivere lo stato.

//AZZERA LA STRUTTURA EmergencyNode_t passata (buffer e contatore locale).
int8_t EmergencyNode_init(EmergencyNode_t* const restrict p_self) //p_self puntatore al nodo iniziale
{
  memset(p_self, 0, sizeof(*p_self)); //riempie un’area di memoria con uno specifico valore.
  return 0;
}

// AZIONA EMERGENZA
int8_t EmergencyNode_raise(EmergencyNode_t* const restrict p_self, const uint8_t exeception)
{ //Calcola quale byte e quale bit della bitmap corrispondono al exeception
  const uint8_t exception_byte = exeception/8;  //in quale dei 8 byte si trova il bit
  const uint8_t exception_bit = exeception % 8; //in quale dei 8 byte si trova il bit

  if (exeception >= NUM_EMERGENCY_BUFFER*8) //Se exeception fuori range → -1.
  {
    return -1;
  }

  const uint8_t old_emergency_bit = (p_self->emergency_buffer[exception_byte] >> exception_bit) & 0x01; //Legge il bit old; poi imposta il bit nella bitmap
  p_self->emergency_buffer[exception_byte] |= 1 << exception_bit; //	Operazione bitwise OR per accendere il bit corrispondente all’emergenza.

  if (!old_emergency_bit) //Se il bit non era già settato:
  {
    if(!p_self->emergency_counter) { //il nodo non aveva altre emergenze
      _increase_global_emergency_counter(); //per aumentare il contatore globale 
    }
    p_self->emergency_counter++; 
  }

  _hw_raise_emergency(); //accende il led (con lock)

  return 0;
}

// DISATTIVA EMERGENZA
int8_t EmergencyNode_solve(EmergencyNode_t* const restrict p_self, const uint8_t exeception)
{
  const uint8_t exception_byte = exeception/8;
  const uint8_t exception_bit = exeception % 8;

  if (exeception >= NUM_EMERGENCY_BUFFER * 8)
  { //se l'eccezione è fuori dal buffer --> -1
    return -1;
  }

  if (p_self->emergency_buffer[exception_byte] &  (1 << exception_bit)) 
  { //Controlla se il bit è settato; se sì: lo resetta
    p_self->emergency_buffer[exception_byte] ^= (1 << exception_bit); // risolve l'em
    p_self->emergency_counter--; //decrementa il contatore locale,
    if (!p_self->emergency_counter)
    { //se non ha più emergenze
      _solved_module_exception_state();
    }
  }

  return 0;
}

//VERIFICA SE CI SONO EMERGENZE
int8_t EmergencyNode_is_emergency_state(const EmergencyNode_t* const restrict p_self)
{ //Ritorna non-zero se questo nodo ha emergenze o se il contatore globale > 0 (il che significa che qualche nodo ha emergenza).
  return p_self->emergency_counter || read_globla_emergency_couner();
  // 0 → nessuna emergenza nel nodo e nessun nodo ha emergenze
	// 1 (o non-zero) → almeno un’emergenza attiva, locale o in un altro nodo
}

//ELIMINA NODO
int8_t EmergencyNode_destroy(EmergencyNode_t* const restrict p_self)
{ //Se il nodo ha ancora emergenze attive
  if (p_self->emergency_counter)
  {
    _solved_module_exception_state(); //per decrementare il contatore globale
  }

  memset(p_self, 0, sizeof(*p_self)); //azzera la struttura
  return 0;
}


/*
Non protetto (rischio di race):
	•	accesso e modifica a strutture per nodo (p_self->emergency_buffer e p_self->emergency_counter) non sono protetti: quindi se due thread diversi manipolano lo stesso EmergencyNode_t contemporaneamente, si generano race condition (letture/scritture simultanee).
	•	EmergencyNode_class_init non è atomicamente protetta: due thread che la chiamano insieme possono creare stato inconsistente.
	•	EXCEPTION_COUNTER.lock potrebbe non essere inizializzato correttamente: se non inizializzato, atomic_flag_test_and_set ha comportamento indefinito.

Conclusione: il modulo è parzialmente thread-safe — la parte globale è protetta (se il lock è inizializzato), ma le strutture locali non lo sono. Per essere completamente thread-safe bisogna:
	•	Proteggere anche le operazioni su EmergencyNode_t (per esempio con mutex per nodo, o usare atomic per emergency_counter e operazioni di bit manipolazione atomiche).
	•	Garantire l’inizializzazione corretta del atomic_flag.
*/