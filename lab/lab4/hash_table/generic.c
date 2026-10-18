#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../../lab3/vector/generic.h"
#include "generic.h"

// Создает вектор с фиксированным размером, все элементы инициализируются нулями
Vector *createVectorBySize(size_t elem_size, size_t size) {
    if (elem_size == 0) return NULL;

    Vector *vector = malloc(sizeof(Vector));
    if (vector == NULL) return NULL;

    vector->data = calloc(size, elem_size);
    if (vector->data == NULL) {
        free(vector);
        return NULL;
    }

    vector->elem_size = elem_size;
    vector->size = size;
    vector->capacity = size;

    return vector;
}

// Простая хеш-функция для целых чисел: остаток от деления на 10
int HashInt(const void *key) {
    return (*(const int*)key) % 10;
}

// Хеш-функция для строк
int HashString(const void *key) {
    const char *str = (const char*)key;
    unsigned long hash = 0;
    while (*str) {
        hash = hash * 31 + (unsigned char)(*str++);
    }
    return (int)hash;
}

// Вычисляет шаг для двойного хеширования 
int HashStep(const void *key, size_t key_size, int capacity) {
    if (capacity <= 1) return 1;
    
    unsigned long hash = 0;
    const unsigned char *data = (const unsigned char *)key;
    
    for (size_t i = 0; i < key_size; i++) {
        hash = hash * 31 + data[i];
    }
    
    int step = (int)(hash % (capacity - 1)) + 1;
    return step;
}

// Создает новую хеш-таблицу с заданными размерами ключа и значения
HashTable *createHashTable(size_t key_size, size_t val_size) {
    if (key_size == 0 || val_size == 0) return NULL;

    HashTable* table = malloc(sizeof(HashTable));
    if (table == NULL) return NULL;
    
    // Каждый слот: 1 байт (состояние) + ключ + значение
    table->values = createVectorBySize(1 + key_size + val_size, TABLE_MIN_SIZE);
    if (table->values == NULL) {
        free(table);
        return NULL;
    }

    table->key_size = key_size;
    table->val_size = val_size;
    table->size = 0;                    // Текущее количество элементов
    table->capacity = TABLE_MIN_SIZE;   // Начальная вместимость

    return table;
}

// Вставляет или обновляет элемент в хеш-таблице
void setItemHashTable(HashTable *table, void *key, void *data, HashFunc hash, CmpFunc cmp) {
    if (table == NULL || hash == NULL || cmp == NULL || key == NULL || data == NULL) {
        return;
    }

    // Проверяем необходимость перехеширования
    if ((float)table->size / table->capacity > 0.5) {
        rehashHashTable(table, hash, cmp);
    }

    const size_t index = hash(key) % table->capacity;
    int step = HashStep(key, table->key_size, (int)table->capacity);

    size_t counter = 0;
    // Поиск подходящего слота с использованием двойного хеширования
    while (counter < table->capacity) {
        size_t index_new = (index + counter * (size_t)step) % table->capacity;
        void *item = getVectorItem(table->values, index_new);

        if (item == NULL) {
            counter++;
            continue;
        }
        
        char status = *(char*)item;
        void *item_key = (char*)item + 1;
        void *item_value = (char*)item_key + table->key_size;

        // Нашли пустой или удаленный слот - вставляем новый элемент
        if (status == SLOT_EMPTY || status == SLOT_DELETED) {
            *(char*)item = SLOT_OCCUPIED;
            memcpy(item_key, key, table->key_size);
            memcpy(item_value, data, table->val_size);
            table->size++;
            break;
        }
        // Ключ уже существует - обновляем значение
        else if (status == SLOT_OCCUPIED && cmp(item_key, key) == 0) {
            memcpy(item_value, data, table->val_size);
            break;
        }

        counter++;
    }
}

// Перераспределяет элементы таблицы при увеличении вместимости
void rehashHashTable(HashTable *table, HashFunc hash, CmpFunc cmp) {
    if (table == NULL || hash == NULL || cmp == NULL) {
        return;
    }
    // Сохраняем старые данные
    size_t old_capacity = table->capacity;
    size_t old_key_size = table->key_size;
    size_t old_val_size = table->val_size;
    Vector *old_vector = table->values;
    
    // Увеличиваем вместимость в 2 раза
    size_t new_capacity = old_capacity * 2;
    Vector *new_vector = createVectorBySize(1 + table->key_size + table->val_size, new_capacity);
    if (!new_vector) return;

    // Устанавливаем новую структуру таблицы
    table->values = new_vector;
    table->capacity = new_capacity;
    table->size = 0;

    // Перемещаем все занятые элементы в новую таблицу
    for (size_t i = 0; i < old_capacity; i++) {
        void *slot = getVectorItem(old_vector, i);
        if (!slot) continue;
        
        char status = *(char*)slot;
        if (status == SLOT_OCCUPIED) {
            void *key_ptr   = (char*)slot + 1;
            void *value_ptr = (char*)slot + 1 + old_key_size;
            setItemHashTable(table, key_ptr, value_ptr, hash, cmp);
        }
    }

    vectorFree(old_vector);
}

// Ищет элемент по ключу и возвращает указатель на его значение
void *getItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp) {
    if (table == NULL || key == NULL || hash == NULL || cmp == NULL) {
        return NULL;
    }

    // Вычисляем начальную позицию и шаг
    const size_t index = hash(key) % table->capacity;
    int step = HashStep(key, table->key_size, (int)table->capacity);

    size_t counter = 0;
    // Поиск элемента с использованием двойного хеширования
    while (counter < table->capacity) {
        size_t index_new = (index + counter * (size_t)step) % table->capacity;
        void* item = getVectorItem(table->values, index_new);

        if (item == NULL) {
            counter++;
            continue;
        }
        
        char status = *(char*)item;
        
        // Пустой слот - элемент не найден
        if (status == SLOT_EMPTY) {
            return NULL;
        }
        
        // Занятый слот - проверяем ключ
        if (status == SLOT_OCCUPIED) {
            void *item_key = (char*)item + 1;
            if (cmp(item_key, key) == 0) {
                void *item_value = (char*)item_key + table->key_size;
                return item_value;
            }
        }
        counter++;
    }
    return NULL;
}

// Подсчитывает количество коллизий в таблице
unsigned long int getCollisionCount(HashTable *table, HashFunc hash) {
    if (table == NULL || hash == NULL) return 0;
    
    unsigned long int collisions = 0;
    
    for (size_t i = 0; i < table->capacity; i++) {
        void *slot = getVectorItem(table->values, i);
        if (!slot) continue;
        
        if (*(char*)slot == SLOT_OCCUPIED) {
            void *key_in_slot = (char*)slot + 1;
            // Идеальная позиция для этого ключа
            size_t first_index = hash(key_in_slot) % table->capacity;
            
            // Возникновение коллизии
            if (i != first_index) {
                collisions++;
            }
        }
    }
    
    return collisions;
}

// Удаляет элемент из таблицы и возвращает его значение
void *popItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp) {
    if (table == NULL || key == NULL || hash == NULL || cmp == NULL) {
        return NULL;
    }

    size_t index = hash(key) % table->capacity;
    int step = HashStep(key, table->key_size, (int)table->capacity);

    size_t counter = 0;

    while (counter < table->capacity) {
        size_t idx = (index + counter * (size_t)step) % table->capacity;
        char *slot = getVectorItem(table->values, idx);
        
        if (!slot) {
            counter++;
            continue;
        }
        
        char status = slot[0];

        if (status == SLOT_EMPTY) { 
            return NULL;  // Элемент не найден
        }

        if (status == SLOT_OCCUPIED) {
            void *slot_key = slot + 1;

            if (cmp(slot_key, key) == 0) {
                void *slot_value = slot + 1 + table->key_size;
                // Копируем значение для возврата
                void *out = malloc(table->val_size);
                if (!out) return NULL;
                memcpy(out, slot_value, table->val_size);

                // Помечаем слот как удаленный
                slot[0] = SLOT_DELETED;
                table->size--;

                return out;
            }
        }

        counter++;
    }

    return NULL;
}

// Освобождает память, занятую хеш-таблицей
void freeHashTable(HashTable *table) {
    if (table == NULL) return;

    if (table->values != NULL) {
        vectorFree(table->values);
    }

    free(table);
}