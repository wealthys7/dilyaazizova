#include <stdio.h>
#include <stdlib.h> // Для функций malloc, free, exit
#include <string.h> // Для memcpy
#include "generic.h"

GenericList *createList(size_t elem_size)
{
    // Проверяем, что размер элемента не нулевой
    if (elem_size == 0) {
        printf("Ошибка! Размер элемента не может быть равен 0\n");
        exit(EXIT_FAILURE); // Завершаем программу
    }

    // Выделяем память для структуры списка
    GenericList *list = malloc(sizeof(GenericList));
    if (list == NULL) { // Если malloc вернул NULL, значит не хватило памяти
        printf("Ошибка! Не удалось выделить память для GenericList\n");
        exit(EXIT_FAILURE); // Завершаем программу
    }

    // Создаем пустой список
    list->head = NULL; // Изначально список пуст, поэтому головы списка тоже нет
    list->elem_size = elem_size; // Сохраняем размер одного элемента, чтобы знать сколько байт копировать при добавлении/удалении элементов 

    return list; // Возвращаем указатель на созданный пустой список
}

void appendItem(GenericList *list, void *data)
{
    // Проверяем, переданы ли указатели на список и данные
    if (list == NULL || data == NULL) { // Не создан список или пустые данные
        printf("Ошибка! Не передан список или данные\n");
        exit(EXIT_FAILURE); // Завершаем программу
    }

    // Выделяем память под новый элемент списка (структура Node)
    Node *new_node = malloc(sizeof(Node)); // Возвращает указатель на выделенную область памяти
    if (new_node == NULL) { // Проверка, удалось ли выделить память
        printf("Ошибка! Не хватило памяти для создания элемента\n");
        exit(EXIT_FAILURE); // Завершаем программу
    }

    // Выделяем память под сами данные, которые будут хранится в этом элементе
    new_node->data = malloc(list->elem_size); // Размер данных известен
    if (new_node->data == NULL) {
        printf("Ошибка! Не хватило памяти для данных\n");
        free(new_node); // Освобождаем память элемента
        exit(EXIT_FAILURE); // Завершаем программу
    }

    // Копируем данные из переданного указателя в элемент (deep copy)
    memcpy(new_node->data, data, list->elem_size); // Копируем данные указанного размера
    new_node->next = NULL; // Новый элемент будет последним в списке, поэтому устанавливаем указатель next в NULL
    
    // Добавляем элемент в список
    if (list->head == NULL) { // Если список пустой - новый элемент становится первым
        list->head = new_node; // Новый элемент теперь голова списка
    }
    else { 
        // Если список не пуст - ищем последний узел
        Node *current = list->head; // Начинаем с головы
        while (current->next != NULL) { // Пока существует следующий элемент, двигаемся по списку
            current = current->next;
        }
        current->next = new_node; // Добавляем новый элемент в конец
    }

}

int findItem(GenericList *list, void *value, EqualsFunc cmp)
{
    // Проверяем, что нам передали все необходимые данные 
    if (list == NULL || list->head == NULL || value == NULL ||  cmp == NULL) {
        return (unsigned int)-1; // Возвращаем ошибку
    }

    // Подготовка к поиску
    Node *current = list->head; // Начинаем с первого элемента списка
    unsigned int index = 0; // Счетчик позиции (начинаем с 0)
    
    // Поиск по списку 
    while (current != NULL) { // Проходит по каждому элементу списка, пока не дойдем до конца
        // Если данные текущего узла и значение, которое мы ищем, равны, cmp равна 1
        if (cmp(current->data, value) == 1) { 
            return index;
        }

        // Если элемент не подошел, переходим к следующему
        current = current->next;
        index++; // Увеличиваем счетчик позиции
    }
    return (unsigned int)-1; // Прошли весь список, но не нашли элемент
}

void *popItem(GenericList *list, size_t index)
{
    // Проверяем, что список существует и не пустой
    if (list == NULL || list->head == NULL) {
        return NULL; // Нечего удалять  
    }

    // Подготовка переменных для прохода по списку
    Node *current = list->head; // Указатель на текущий элемент, который проверяем (начинаем с головы)
    Node *previous = NULL; // Указатель на предыдущий элемент
    size_t current_index = 0; // Счетчик текущей позиции

    // Ищем элемент с нужным индексом
    while (current !=  NULL && current_index < index) { // Пока не найдем нужный индекс элемента
        previous = current; // Запоминаем текущий элемент как предыдущий элемент
        current = current->next; // Переходим к следующему элементу
        current_index++; // Увеличиваем счетчик
    }

    if (current == NULL) { // Если текущий элемент стал NULL, значит индекс больше количества элементов
        return NULL;  
    }

    // Создаем копию данных перед удалением
    void *data_copy = malloc(list->elem_size);
    if (data_copy == NULL) {
        printf("Ошибка! Не хватило памяти для копии данных\n");
        return NULL; 

    }
    // Копируем данные из удаляемого элемента
    memcpy(data_copy, current->data, list->elem_size);

    // Удаляем элемент из списка
    if (previous == NULL) {
        // Удаляем голову списка (первый элемент)
        list->head = current->next; // Второй элемент становится головой списка
    }
    else {
        // Удаляем элемент из середины или конца
        previous->next = current->next; // Пропускаем текущий элемент
    }
    
    // Освобождаем память удаляемого элемента
    free(current->data); // Удаляем данные элемента
    free(current); // Удаляем сам элемент

    // Возвращаем указатель на копию данных
    return data_copy;
}

void freeList(GenericList *list)
{
    // Проверяем, существует ли вообще список для освобождения
    if (list == NULL) { // Указатель пустой
        return; // Ничего не делаем
    }

    // Начинаем с первого элемента списка
    Node *current = list->head;

    // Проходим по всем элементам списка и освобождаем их
    while (current != NULL) { // Пока не закончатся элементы
        // Сохраняем указатель на следующий элемент, чтобы учесть его при удалении текущего
        Node *next = current->next;
        
        // Освобождаем память, выделенную для данных элемента
        free(current->data);
        
        // Освобождаем память, выделенную для самого элемента
        free(current);
        
        // Переходим к следующему элементу
        current = next;
    }

    // После удаления всех элементов освобождаем память самой структуры списка
    free(list);
}

unsigned int listLength(GenericList *list)
{
    // Проверяем, не пуст ли список
    if (list == NULL || list->head == NULL) {
        return 0;  // Если списка не существует, возвращаем 0 элементов
    }

    // Создаем счетчик элементов
    unsigned int count = 0;
    
    // Начинаем обход списка с первого элемента
    Node *current = list->head;

    // Проходим по всем элементам списка до конца
    while (current != NULL) {
        // Увеличиваем счетчик на 1 для каждого найденного элемента
        count++;
        
        // Переходим к следующему элементу списка
        current = current->next;
    }

    // Возвращаем общее количество элементов в списке
    return count;
}