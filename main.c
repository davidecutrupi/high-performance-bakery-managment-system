#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define SEED 0x17648694
#define TABLE_SIZE_MAGAZZINO 2048
#define TABLE_SIZE_RICETTARIO 1024

typedef enum { RED, BLACK } Color;
typedef enum { false, true } Bool;


typedef struct Command {
  char *word;
  unsigned char end;
} Command;



typedef struct NodoSottolistaLotti {
  int data_scadenza;
  int qta;
  struct NodoSottolistaLotti *next;
} NodoSottolistaLotti;

typedef struct NodoListaLotti{
  struct NodoListaLotti *next;
  char *name;
  NodoSottolistaLotti *lotti;
  NodoSottolistaLotti *last_inserted;
  int disponibile;
} NodoListaLotti;
typedef NodoListaLotti * ListaLotti;

typedef struct Magazzino {
  ListaLotti table[TABLE_SIZE_MAGAZZINO];
} Magazzino; // Hash Table Magazzino



typedef struct NodoListaIngredienti {
  struct NodoListaIngredienti *next;
  NodoListaLotti *ingrediente;
  int qta;
} NodoListaIngredienti;
typedef NodoListaIngredienti * ListaIngredienti;

typedef struct NodoListaRicette {
  struct NodoListaRicette *next;
  char *name;
  ListaIngredienti ingredienti;
} NodoListaRicette;
typedef NodoListaRicette * ListaRicette;

typedef struct Ricettario {
  ListaRicette table[TABLE_SIZE_RICETTARIO]; 
} Ricettario; // Hash table ricettario



typedef struct NodoListaOrdini {
  NodoListaRicette *ricetta;
  NodoListaIngredienti *ingrediente_mancante;
  int n_orders;
  int time;
  int weight;
  struct NodoListaOrdini *next;
} NodoListaOrdini;
typedef NodoListaOrdini * ListaOrdini;

typedef struct CodaOrdini {
  ListaOrdini head, tail;
} CodaOrdini;



CodaOrdini* createCodaOrdine() {
  CodaOrdini *ordini = (CodaOrdini*) malloc(sizeof(CodaOrdini));
  ordini->head = ordini->tail = NULL;
  return ordini;
}

void enqueueOrdine(CodaOrdini *Q, NodoListaRicette *ricetta, NodoListaIngredienti *ingrediente_mancante, int time, int n_orders, int weight) {
  NodoListaOrdini *ordine = (NodoListaOrdini*) malloc(sizeof(NodoListaOrdini));
  ordine->next = NULL;
  ordine->ricetta = ricetta;
  ordine->time = time;
  ordine->n_orders = n_orders;
  ordine->weight = weight;
  ordine->ingrediente_mancante = ingrediente_mancante;
  if (Q->head == NULL) Q->head = ordine;
  if (Q->tail != NULL) Q->tail->next = ordine;
  Q->tail = ordine;
}

NodoListaOrdini* dequeueOrdine(CodaOrdini *Q) {
  if (Q->head == NULL) return NULL;
  NodoListaOrdini *ordine = Q->head;
  Q->head = Q->head->next;
  if (Q->head == NULL) Q->tail = NULL;
  return ordine;
}

uint32_t hash(const void* key) {
  uint32_t h = SEED;
  h ^= 2166136261UL;
  const uint8_t* data = (const uint8_t*)key;
  for(int i = 0; data[i] != '\0'; i++) {
    h ^= data[i];
    h *= 16777619;
  }
  return h;
}

Magazzino* createMagazzino() {
  Magazzino *magazzino = (Magazzino*) malloc(sizeof(Magazzino));
  for(int i=0; i<TABLE_SIZE_MAGAZZINO; i++) magazzino->table[i] = NULL;
  return magazzino;
}

NodoListaIngredienti* create_ingrediente_node(Magazzino *magazzino, char* name, int qta, NodoListaIngredienti *next) {
  NodoListaIngredienti *ingredient = (NodoListaIngredienti*) malloc(sizeof(NodoListaIngredienti));
  ingredient->qta = qta;
  ingredient->next = next;

  // Cerco l'ingrediente nel magazzino (per linkarlo al ricettario)
  unsigned long index = hash(name) % TABLE_SIZE_MAGAZZINO;
  NodoListaLotti *list = magazzino->table[index];  
  while (list != NULL) {
    if (!strcmp(list->name, name)) break;
    list = list->next;
  }
  
  if (list != NULL) {
    free(name); // Libero la stringa con il nome dell'ingrediente visto che esiste già
    ingredient->ingrediente = list;
  }
  else { // Non esiste l'ingrediente nel magazzino, lo creo ma senza heap
    list = (ListaLotti) malloc(sizeof(NodoListaLotti));
    list->name = name;
    list->next = magazzino->table[index];
    list->disponibile = 0;
    list->lotti = NULL;
    list->last_inserted = NULL;
    magazzino->table[index] = list;
    ingredient->ingrediente = list;
  }

  return ingredient;
}

NodoListaRicette* create_ricetta_node(char *name, NodoListaRicette *next) {
  NodoListaRicette *ricetta = (NodoListaRicette*) malloc(sizeof(NodoListaRicette));
  ricetta->name = name;
  ricetta->next = next;
  ricetta->ingredienti = NULL;
  return ricetta;
}

NodoListaRicette* check_ricetta(char *name, Ricettario *ricettario, unsigned long index) {
  NodoListaRicette *list = ricettario->table[index]; // Cerco la ricetta nel ricettario
  while (list != NULL) {
    if (!strcmp(list->name, name)) return list;
    list = list->next;
  }
  return list;
}

Ricettario* createRicettario() {
  Ricettario *ricettario = (Ricettario*) malloc(sizeof(Ricettario));
  for(int i=0; i<TABLE_SIZE_RICETTARIO; i++) ricettario->table[i] = NULL;
  return ricettario;
}

void remove_lotti_scaduti(NodoListaLotti *list, int time) {
  while (list->lotti != NULL && list->lotti->data_scadenza <= time) {
    list->disponibile -= list->lotti->qta;
    NodoSottolistaLotti *tmp = list->lotti;
    list->lotti = list->lotti->next;
    if (list->last_inserted == tmp) list->last_inserted = list->lotti; // Aggiorno last_updated
    free(tmp);
  }
}

void insertLotto(Magazzino *magazzino, int time, char *nome, int scad, int qta) {
  unsigned long index = hash(nome) % TABLE_SIZE_MAGAZZINO; // Hash function
  ListaLotti list = magazzino->table[index];

  while(list != NULL) { // Cerco l'ingrediente nella lista dei lotti
    if (!strcmp(list->name, nome)) break;
    list = list->next;
  }

  if (list == NULL) { // Ingrediente non trovato, lo aggiungo alla lista
    list = (ListaLotti) malloc(sizeof(NodoListaLotti));
    list->name = nome;
    list->next = magazzino->table[index];
    list->lotti = NULL;
    magazzino->table[index] = list;
  }
  else free(nome); // Se l'elemento nella lista esiste già, faccio la free del nome dell'ingrediente letto

  // Faccio un check sui lotti scaduti
  remove_lotti_scaduti(list, time);

  // Aggiorno il contatore della qta disponbile
  list->disponibile += qta;

  // Creo il nuovo lotto nella sottolista e lo metto in ordine di scadenza
  NodoSottolistaLotti *new_lotto = (NodoSottolistaLotti*) malloc(sizeof(NodoSottolistaLotti));
  new_lotto->data_scadenza = scad;
  new_lotto->qta = qta;

  if (list->lotti == NULL || scad < list->lotti->data_scadenza) { // Se la lista è vuota oppure va messo all'inizio
    new_lotto->next = list->lotti;
    list->lotti = new_lotto;
    list->last_inserted = new_lotto; // Aggiorno last_updated
  }
  else {
    NodoSottolistaLotti *curr = (list->last_inserted != NULL && list->last_inserted->data_scadenza <= scad) ? list->last_inserted : list->lotti; // Scelgo se partire a scorrere dall'inizio o da last_inserted
    while (curr->next != NULL && curr->next->data_scadenza < scad) { curr = curr->next; }
    if (curr->next != NULL && curr->next->data_scadenza == scad) { // Già c'è un lotto con la stessa data di scadenza, incremento quella quantità
      curr->next->qta += qta;
      free(new_lotto);
    }
    else {
      new_lotto->next = curr->next;
      curr->next = new_lotto;
    }
    list->last_inserted = curr->next; // Aggiorno last_updated
  }
}

void remove_lotti(int time, NodoListaLotti *ingrediente, int necessario) {
  // Elimino i lotti fino a che non ho terminato il necessario
  while (necessario > 0) {
    if (ingrediente->lotti->data_scadenza <= time) { // Lotto scaduto
      ingrediente->disponibile -= ingrediente->lotti->qta;
      NodoSottolistaLotti *tmp = ingrediente->lotti;
      ingrediente->lotti = ingrediente->lotti->next;
      free(tmp);
      if (ingrediente->last_inserted == tmp) ingrediente->last_inserted = ingrediente->lotti; // Aggiorno last_updated
      continue;
    } 
    if (ingrediente->lotti->qta > necessario) { ingrediente->lotti->qta -= necessario; ingrediente->disponibile -= necessario; break; } // Se avanza qualcosa di un lotto non lo rimuovo ma decremento la qta
    necessario -= ingrediente->lotti->qta;
    ingrediente->disponibile -= ingrediente->lotti->qta;
    NodoSottolistaLotti *tmp = ingrediente->lotti;
    ingrediente->lotti = ingrediente->lotti->next;
    if (ingrediente->last_inserted == tmp) ingrediente->last_inserted = ingrediente->lotti; // Aggiorno last_updated
    free(tmp);
  }
}

Bool check_availability(Magazzino *magazzino, int time, NodoListaLotti *ingredient, int necessario) {
  remove_lotti_scaduti(ingredient, time); // Mi assicuro che non ci siano lotti scaduti
  if (ingredient->disponibile >= necessario) return true;
  return false;
}

char* read_word(char *word) {

  int capacity = 20;
  int size = 0;
  if (word == NULL) word = (char*) malloc(capacity * sizeof(char));
  unsigned char c;

  while((c = getchar_unlocked()) != 255) {
    if (isspace(c) && size > 0) { break; } // Fine parola
    else if (isspace(c)) { continue; } // Salta spazi vuoti
    word[size++] = c;
  }

  if (size == 0) { 
    free(word);
    return NULL;
  }
  word[size] = '\0';
  return word;
}

Command read_number(char *prev_number) {
  Command command;
  command.end = '0';
  int capacity = 10;
  int size = 0;
  if (prev_number == NULL) command.word = (char*) malloc(capacity * sizeof(char));
  else command.word = prev_number;
  unsigned char c;

  while((c = getchar_unlocked()) != 255) {
    if (c == '\n') command.end = '1';
    if (isspace(c) && size > 0) { break; } // Fine parola
    else if (isspace(c)) { continue; } // Salta spazi vuoti

    command.word[size++] = c;
  }

  if (size == 0) { 
    free(command.word);
    command.word = NULL;
    return command;
  }
  command.word[size] = '\0';
  return command;
}

void aggiungi_ricetta(Ricettario *ricettario, Magazzino *magazzino) {
  Command command;
  command.word = NULL;
  char *name = read_word(NULL); // Leggo nome ricetta
  unsigned long index = hash(name) % TABLE_SIZE_RICETTARIO;

  NodoListaRicette *ricetta = check_ricetta(name, ricettario, index); // Controllo se esiste già
  if (ricetta != NULL) {
    // Leggo a vuoto per finire la stringa
    unsigned char c;
    while((c = getchar_unlocked()) != '\n') {}
    free(name);
    printf("ignorato\n");
    return;
  }

  ricetta = create_ricetta_node(name, ricettario->table[index]); // Creo la ricetta e la aggiungo alla lista dell'hash table
  ricettario->table[index] = ricetta;

  do {
    char *ingredient_name = read_word(NULL); // Leggo nome ingrediente
    command = read_number(command.word); // Leggo qta ingrediente
    int ingredient_qta = atoi(command.word);

    NodoListaIngredienti *ingredient = create_ingrediente_node(magazzino, ingredient_name, ingredient_qta, ricetta->ingredienti); // Creo ingrediente e lo aggiungo alla lista degli ingredienti della ricetta
    ricetta->ingredienti = ingredient;
  } while (command.end == '0');
  free(command.word);
  printf("aggiunta\n");
}

void rimuovi_ricetta(Ricettario *ricettario, CodaOrdini *ordini_pronti, CodaOrdini *ordini_attesa) {
  char *name = read_word(NULL); // Leggo nome ricetta
  unsigned long index = hash(name) % TABLE_SIZE_RICETTARIO;

  NodoListaRicette *prev = NULL, *list = ricettario->table[index]; // Cerco la ricetta nel ricettario
  while (list != NULL) {
    if (!strcmp(list->name, name)) break;
    prev = list;
    list = list->next;
  }
  if (list == NULL) { printf("non presente\n"); return; } // Ricetta non trovata
  free(name);

  // Controllo se ci sono ordini non ancora spediti con di quella ricetta
  NodoListaOrdini *ordine = ordini_attesa->head;
  while(ordine != NULL) {
    if (ordine->ricetta == list) { printf("ordini in sospeso\n"); return; }
    ordine = ordine->next;
  }
  ordine = ordini_pronti->head;
  while(ordine != NULL) {
    if (ordine->ricetta == list) { printf("ordini in sospeso\n"); return; }
    ordine = ordine->next;
  }

  NodoListaIngredienti *next_node, *curr_node = list->ingredienti; // Dealloco tutti gli ingredienti e i loro nodi
  while (curr_node != NULL) {
    next_node = curr_node->next;
    free(curr_node);
    curr_node = next_node;
  }

  free(list->name); // Dealloco la ricetta
  if (prev != NULL) prev->next = list->next;
  else ricettario->table[index] = list->next;
  free(list);
  printf("rimossa\n");
}

void rifornimento(int time, Ricettario *ricettario, Magazzino *magazzino, CodaOrdini *ordini_pronti, CodaOrdini *ordini_attesa) {
  Command command;
  command.word = NULL;

  do {
    char *name = read_word(NULL); // Leggo nome - non faccio la free perché mi serve memorizzare la stringa
    command = read_number(command.word); // Leggo qta
    int qta = atoi(command.word);
    command = read_number(command.word); // Leggo scadenza
    insertLotto(magazzino, time, name, atoi(command.word), qta);
  } while (command.end == '0');
  free(command.word);
  printf("rifornito\n");

  // Controllo se posso fare qualche ordine in attesa
  NodoListaOrdini *prev = NULL, *ordine = ordini_attesa->head, *next;
  while(ordine != NULL) { // Scorro gli ordini in attesa 

    if (ordine->ingrediente_mancante->ingrediente->disponibile < ordine->ingrediente_mancante->qta * ordine->n_orders) goto CIAO;

    int weight = 0;
    NodoListaIngredienti *ingredient = ordine->ricetta->ingredienti;
    while (ingredient != NULL) {
      if (!check_availability(magazzino, time, ingredient->ingrediente, ingredient->qta * ordine->n_orders)) { // Non ci sono ancora abbastanza ingredienti, controllo l'ordine successivo
        ordine->ingrediente_mancante = ingredient;
        break;
      }
      weight += ingredient->qta * ordine->n_orders;
      ingredient = ingredient->next;
    }
    
    if (ingredient == NULL) { // L'ordine può essere completato
      ingredient = ordine->ricetta->ingredienti;
      while (ingredient != NULL) {
        remove_lotti(time, ingredient->ingrediente, ingredient->qta * ordine->n_orders); 
        ingredient = ingredient->next;
      }

      ordine->weight = weight; // Aggiorno il peso dell'ordine
      next = ordine->next; // Mi salvo il successivo ordine in attesa

      // Rimuovo l'ordine tra quelli in attesa
      if (prev != NULL) {
        if (ordine->next == NULL) ordini_attesa->tail = prev;
        prev->next = ordine->next;
      }
      else dequeueOrdine(ordini_attesa);
      
      // Aggiungo l'ordine tra quelli pronti in ordine di time
      if (ordini_pronti->head == NULL || ordini_pronti->head->time > ordine->time) { // Se la lista è vuota oppure va messo all'inizio
        ordine->next = ordini_pronti->head;
        ordini_pronti->head = ordine;
        if (ordini_pronti->tail == NULL) ordini_pronti->tail = ordine;
      }
      else {
        NodoListaOrdini *curr = ordini_pronti->head;
        while (curr->next != NULL && curr->next->time < ordine->time) curr = curr->next;
        ordine->next = curr->next;
        curr->next = ordine;
        if (ordine->next == NULL) ordini_pronti->tail = ordine;
      }

      ordine = next;
    }
    else {
      CIAO:
      prev = ordine;
      ordine = ordine->next;
    }
  }
}

void ordine(int time, Ricettario *ricettario, Magazzino *magazzino, CodaOrdini *ordini_pronti, CodaOrdini *ordini_attesa) {
  char* read = read_word(NULL); // Leggo il nome della ricetta
  NodoListaRicette *ricetta = check_ricetta(read, ricettario, hash(read) % TABLE_SIZE_RICETTARIO); // Cerco se esiste la ricetta

  read = read_number(read).word;
  int number_orders = atoi(read);
  free(read);
  
  if (ricetta == NULL) { printf("rifiutato\n"); return; }
  printf("accettato\n");

  // Faccio il check degli ingredienti
  int weight = 0;
  NodoListaIngredienti *ingredient = ricetta->ingredienti;
  while (ingredient != NULL) {
    if (!check_availability(magazzino, time, ingredient->ingrediente, ingredient->qta * number_orders)) { // Lo aggiungo tra quelli in attesa
      enqueueOrdine(ordini_attesa, ricetta, ingredient, time, number_orders, 0);
      return;
    }
    weight += ingredient->qta * number_orders;
    ingredient = ingredient->next;
  }

  // Se ho abbastanza ingredienti, elimino i lotti dal magazzino
  ingredient = ricetta->ingredienti;
  while (ingredient != NULL) {
    remove_lotti(time, ingredient->ingrediente, ingredient->qta * number_orders); 
    ingredient = ingredient->next;
  }

  enqueueOrdine(ordini_pronti, ricetta, NULL, time, number_orders, weight); // Aggiungo l'ordine tra quelli pronti
}

void corriere(int CAPIENZA_CORRIERE, CodaOrdini *ordini_pronti, CodaOrdini *ordini_attesa) {
  NodoListaOrdini *ordini_spediti = NULL; // Lista degli ordini nel campioncino
  while(ordini_pronti->head != NULL) {

    if (ordini_pronti->head->weight > CAPIENZA_CORRIERE) break; // Non c'è più spazio, mi fermo

    // Tolgo l'ordine dalla coda di quelli pronti e lo metto in quelli spediti
    NodoListaOrdini *ordine = dequeueOrdine(ordini_pronti); 
    if (ordini_spediti == NULL || ordini_spediti->weight < ordine->weight) { // Se la lista è vuota oppure va messo all'inizio
      ordine->next = ordini_spediti;
      ordini_spediti = ordine;
    }
    else {
      NodoListaOrdini *curr = ordini_spediti;
      while (curr->next != NULL && curr->next->weight >= ordine->weight) curr = curr->next;
      ordine->next = curr->next;
      curr->next = ordine;
    }
    
    CAPIENZA_CORRIERE -= ordine->weight; 
  }

  if (ordini_spediti == NULL) { printf("camioncino vuoto\n"); return; } // Se il camioncino è vuoto

  while (ordini_spediti != NULL) { // Stampo gli ordini dal più pesante e li dealloco
    NodoListaOrdini *ordine = ordini_spediti;
    printf("%d %s %d\n", ordine->time, ordine->ricetta->name, ordine->n_orders);
    ordini_spediti = ordine->next;
    free(ordine); // Non faccio la free di ordine->name perchè è il nome della ricetta e deve rimanere salvato
  }
}

int main() {

  int PER_CORRIERE, CAPIENZA_CORRIERE, time = 0;
  Magazzino *magazzino = createMagazzino();
  Ricettario *ricettario = createRicettario();
  CodaOrdini *ordini_pronti = createCodaOrdine();
  CodaOrdini *ordini_attesa = createCodaOrdine();

  char *command_type = NULL;

  if (!scanf("%d %d", &PER_CORRIERE, &CAPIENZA_CORRIERE)) exit(1);

  while ((command_type = read_word(command_type)) != NULL) {

    if (time != 0 && time % PER_CORRIERE == 0) corriere(CAPIENZA_CORRIERE, ordini_pronti, ordini_attesa); // Controllo se deve passare il camioncino

    if (!strcmp(command_type, "aggiungi_ricetta")) aggiungi_ricetta(ricettario, magazzino);
    else if (!strcmp(command_type, "rimuovi_ricetta")) rimuovi_ricetta(ricettario, ordini_pronti, ordini_attesa);
    else if (!strcmp(command_type, "rifornimento")) rifornimento(time, ricettario, magazzino, ordini_pronti, ordini_attesa);
    else if (!strcmp(command_type, "ordine")) ordine(time, ricettario, magazzino, ordini_pronti, ordini_attesa);

    time++; // Incremento il tempo
  }
  free(command_type);

  if (time != 0 && time % PER_CORRIERE == 0) corriere(CAPIENZA_CORRIERE, ordini_pronti, ordini_attesa); // Controllo se deve passare il camioncino alla fine

  return 0;
}