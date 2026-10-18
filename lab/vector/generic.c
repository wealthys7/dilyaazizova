#include <stdio.h>
#include <string.h> // Для memcpy
#include <stdlib.h> // Для функций malloc, free, exit
#include "generic.h"

// Вспомогательная функция для изменения размера
static bool needToResize(Vector *vector, bool *increase) // static означает, что функция видна только в текущем файле
{
    if (vector->size >= vector->capacity) { // Вектор полон - при следующем добавлении нет места
        *increase = true; // Требуется увеличение
        return true; // Изменение размера необходимо
    }
    // Если элементов стало мало (меньше четверти capacity, чтобы избежать частых перераспределений памяти)
    else if (vector->size > 0 && vector->size <= vector->capacity / 4 && vector->capacity > MIN_SIZE) {
        *increase = false; // Можно уменьшить
        return true; // Изменение размера необходимо
    }
    return false; // Изменение размера не нужно
}

// Определяем увеличивать размер или уменьшать
static int resize(Vector *vector, bool increase)
{
    // Определяем новую вместимость (*2 при увеличении (параметр increase = true), /2 при уменьшении (параметр increase = false))
    size_t newCapacity = increase ? vector->capacity * 2 : vector->capacity / 2;

    // Гарантируем, что вместимость не меньше минимально допустимой
    if (newCapacity < MIN_SIZE)
        newCapacity = MIN_SIZE;

    // Проверяем переполнение при вычислении общего размера памяти (во избежание переполнения при newCapacity * elem_size)
    if (vector->elem_size != 0 && newCapacity > SIZE_MAX / vector->elem_size)
        return -1; // Размер слишком велик, возможно переполнение

    // Выделяем новый блок памяти (realloc копирует старые данные в новый блок и освобождает старую память автоматически)
    void *newData = realloc(vector->data, newCapacity * vector->elem_size);
    if (newData == NULL) // realloc не смог выделить память
        return -1; // Старый блок данных остается без изменений

    // Обновляем указатель и вместимость после успешного перераспределения
    vector->data = newData;
    vector->capacity = newCapacity;

    return 0;
}

Vector *createVector(size_t elem_size)
{
    if (elem_size == 0) // Проверяем, что размер элемента не нулевой
        return NULL;

    // Выделяем память под структуру Vector    
    Vector *vector = malloc(sizeof(Vector));
    if (vector == NULL) // malloc вернул NULL, значит не хватило памяти
        return NULL;

    // Выделяем блок памяти для самих элементов    
    vector->data = malloc(MIN_SIZE * elem_size); 
    if (vector->data == NULL) { // Не смогли выделить память для данных
        free(vector); // Очищаем уже выделенную структуру Vector
        return NULL;
    }
    // Создаем структуру
    vector->elem_size = elem_size; // Сколько байт занимает один элемент
    vector->size = 0; // Пока нет элементов
    vector->capacity = MIN_SIZE; // выделено место на минимальное количество элементов

    // Возвращаем указатель на готовую структуру
    return vector;
}

int appendVectorItem(Vector *vector, void *el)
{
    if (vector == NULL || el == NULL) // Проверяем, что оба указателя существуют
        return -1;

    bool increase = false; // объявляем флаг, для заполнения информацией нужно ли менять размер
    if (needToResize(vector, &increase) && increase) {
    // Пытаемся увеличить вместимость
        if (resize(vector, true) != 0)
            return -1;
    }

    // Вычисляем адрес, куда нужно вставить новый элемент
    void *target = (char *)vector->data + (vector->size * vector->elem_size);  // Используем смещение index * elem_size байт от начала массива данных
    memcpy(target, el, vector->elem_size); // Копируем содержимое элемента в рассчитанное место
    vector->size++; // Увеличиваем счетчик элементов, тк добавили один новый
    return 0;
}

void *getVectorItem(Vector *vector, size_t index)
{
    // Проверяем корректность данных
    if (vector == NULL || index >= vector->size) 
        return NULL;

    // Адрес нужного элемента
    return (char *)vector->data + (index * vector->elem_size); // приводим data к (char *) для сложения и сдвигаемся
}

int setVectorItem(Vector *vector, size_t index, void *value)
{
    // Проверка на корректность данных
    if (vector == NULL || value == NULL || index >= vector->size)
        return -1;

    // Находим адрес элемента, который нужно заменить
    void *target = (char *)vector->data + (index * vector->elem_size); // Приводим data к (char *) для сложения и сдвигаемся
    memcpy(target, value, vector->elem_size); // Копируем новые данные в память элемента
    return 0;
}

void *popVectorItem(Vector *vector, size_t index)
{
    // Проверка корректности данных
    if (vector == NULL || vector->size == 0 || index >= vector->size)
        return NULL;

    // Выделяем память для копии удаляемого элемента
    void *removed = malloc(vector->elem_size);
    if (removed == NULL) // malloc не смог выделить память
        return NULL;

    // Адрес удаляемого элемента
    void *src = (char *)vector->data + (index * vector->elem_size); // Вычислили адрес элемента, используя смещение index * elem_size байт от начала массива данных
    memcpy(removed, src, vector->elem_size); // Копируем значение удаляемого элемента в выделенную память

    // Сдвигаем последующие элементы
    if (index < vector->size - 1) { // Пытаемся удалить не последний элемент
        // Сдвигаем блок памяти: копируем все элементы справа от удалённого 
        memmove(src, (char *)src + vector->elem_size, (vector->size - index - 1) * vector->elem_size);
    }

    // Уменьшаем количество элементов
    vector->size--;

    // Проверяем, не стоит ли уменьшить размер
    bool increase = false;
    if (needToResize(vector, &increase) && increase == false) {
        resize(vector, false);
    }

    // Возвращаем копию удаленного элемента
    return removed;
}

long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    // Проверка корректности данных
    if (vector == NULL || value == NULL || vector->size == 0)
        return -1;

    // Проходим по всем элементам вектора
    for (size_t i = 0; i < vector->size; i++) {
        // Получаем адрес текущего элемента (смещение в байтах)
        void *current = (char *)vector->data + (i * vector->elem_size);
        
        // Сравниваем элемент с искомым значением
        if (cmp(current, value))
            return (long int)i; // Возвращаем индекс найденного
    }

    // Если элемент не найден
    return -1;
}

int vectorFree(Vector *vector)
{
    // Проверяем, что вектор существует
    if (vector == NULL)
        return -1;

    free(vector->data); // Освобождаем память, где хранились элементы
    free(vector); // Освобождаем саму структуру Vector

    return 0;
}