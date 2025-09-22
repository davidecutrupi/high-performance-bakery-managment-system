# Industrial Bakery Simulator 🍰

![Language](https://img.shields.io/badge/Language-C-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

A comprehensive C-based simulation of an industrial bakery's order and inventory management system. This project was developed as a final assignment for an Algorithms and Data Structures course, focusing on efficiency, memory management, and the implementation of complex logic using appropriate data structures.

---

## 📋 Overview

The program simulates the complete workflow of a high-tech industrial bakery. It operates on a **discrete-time** model, where each command processed marks the passage of one time unit. The simulation manages recipes, ingredients, warehouse inventory with expiring batches, customer orders, and a delivery courier with specific loading logistics.

---

## ✨ Features

* **Recipe Management**: Dynamically add and remove recipes from a recipe book.
* **Warehouse System**: Manages ingredients stored in batches. Each batch has a specific quantity and an expiration date.
* **Smart Ingredient Usage**: When preparing an order, the system automatically uses ingredient batches with the **closest expiration date first** (FIFO logic based on expiration).
* **Order Handling**:
    * Processes incoming customer orders.
    * If ingredients are insufficient, the order is placed in a **pending queue**.
    * Pending orders are automatically processed chronologically as soon as new supplies arrive.
* **Courier Logistics**:
    * A courier arrives periodically to pick up ready orders.
    * Orders are selected for shipping chronologically until the courier's remaining capacity is exceeded.
    * The selected orders are then loaded onto the truck in **descending order of weight**. Orders with the same weight are loaded chronologically.

---

## 🛠️ Data Structures & Implementation Details

The core of this project is built upon efficient data structures chosen to optimize performance for each specific task.

* ### Hash Tables
    The **Warehouse (`Magazzino`)** and the **Recipe Book (`Ricettario`)** are implemented as hash tables. This provides an average time complexity of $O(1)$ for searching, inserting, and deleting ingredients and recipes. Collisions are handled using the **chaining** method. A custom hash function is used for key distribution.

* ### Linked Lists & Queues
    * **Warehouse Inventory**: Each ingredient in the warehouse hash table points to a **sorted linked list** of its batches (`NodoSottolistaLotti`), ordered by expiration date. This ensures that the batch expiring soonest is always at the head of the list, making the FIFO logic highly efficient. An optimization pointer (`last_inserted`) is used to speed up insertions into this sorted list.
    * **Order Queues**: The simulation maintains two distinct queues (`CodaOrdini`) to manage the order lifecycle:
        1.  **Pending Orders (`ordini_attesa`)**: A standard FIFO queue for orders that could not be fulfilled due to ingredient shortages.
        2.  **Ready Orders (`ordini_pronti`)**: A queue holding prepared orders waiting for the courier. Orders are added here once prepared.

* ### Optimized I/O
    Input is read character by character using `getchar_unlocked()`, which is a non-locking, buffered input function. This significantly speeds up the parsing of commands compared to standard `scanf`.

---

## 🚀 How to Compile and Run

### Compilation
The program is written in standard C and can be compiled using `gcc` or any other C compiler. For best performance, it's recommended to compile with optimizations enabled.

```bash
# Compile the source file
gcc -o bakery_simulator "progetto api.c" -O2 -w
```

### Execution
The program reads the simulation configuration and commands from standard input. You can use a file for input via redirection.

```bash
# Run the compiled program with an input file
./bakery_simulator < input.txt
```

### Input File Format
1.  The **first line** of the input file must contain two integers: the courier's periodicity `n` and its capacity `C` (in grams).
2.  **Subsequent lines** contain the commands to be executed by the simulation.

---

## 💻 Commands

The simulation accepts the following commands:

### `aggiungi_ricetta`
Adds a new recipe to the recipe book. If a recipe with the same name already exists, the command is ignored.

* **Syntax**: `aggiungi_ricetta <recipe_name> <ingredient_1> <qty_1> <ingredient_2> <qty_2> ...`
* **Example**: `aggiungi_ricetta apple_pie flour 200 apples 500 sugar 150`

### `rimuovi_ricetta`
Removes a recipe from the recipe book. This fails if the recipe does not exist or if there are pending or ready-to-ship orders for it.

* **Syntax**: `rimuovi_ricetta <recipe_name>`
* **Example**: `rimuovi_ricetta apple_pie`

### `rifornimento`
Adds new batches of ingredients to the warehouse. Multiple ingredients can be supplied in a single command.

* **Syntax**: `rifornimento <ingredient_1> <qty_1> <exp_1> <ingredient_2> <qty_2> <exp_2> ...`
    * `exp` is the time instant at which the batch expires.
* **Example**: `rifornimento flour 1000 220 sugar 200 150`

### `order`
Places an order for a specified number of items of a given recipe. If the recipe doesn't exist, the order is rejected.

* **Syntax**: `ordine <recipe_name> <item_count>`
* **Example**: `ordine torta_paradiso 36`

---

## 📜 Example

Based on the provided specification, here is an example of the program's execution flow.

**Input (`input.txt`)**:
```
5 325
aggiungi_ricetta torta farina 50 uova 10 zucchero 20
aggiungi_ricetta ciambella farina 20 uova 5 burro 2
aggiungi_ricetta profiterole farina 10 uova 2 latte 3 zucchero 3 cioccolato 4
rimuovi_ricetta sfogliatella
rifornimento farina 100 10 uova 100 10 zucchero 100 10 burro 100 10 latte 100 10 cioccolato 100 10
ordine ciambella 6
ordine profiterole 3
rimuovi_ricetta profiterole
aggiungi_ricetta pane_dolce farina 1 zucchero 1 uova 1
ordine ciambella 3
ordine torta 1
rifornimento farina 100 15 farina 50 13 uova 45 20 zucchero 20 20 burro 15 20
rifornimento farina 100 15 uova 25 15 latte 5 15 cioccolato 5 15 zucchero 7 15
ordine torta 1
ordine profiterole 1
```

**Output**:
```
aggiunta
aggiunta
aggiunta
non presente
rifornito
camioncino vuoto
accettato
accettato
ordini in sospeso
aggiunta
accettato
9 ciambella 3
6 profiterole 3
accettato
rifornito
rifornito
accettato
accettato
5 ciambella 6
10 torta 1
13 torta 1
```

---

## 📄 License

This project is licensed under the MIT License. See the `LICENSE` file for more details.
