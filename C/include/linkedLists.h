#include <stddef.h>

struct list_node
{
    struct list_node *next;
};

int list_get_length(struct list_node **head)
{
    struct list_node *current_node = *head;
    int c = 0;
    while (current_node)
    {
        c++;
        current_node = current_node->next;
    }
    return c;
}

struct list_node *list_get_tail(struct list_node **head)
{
    struct list_node *current_node = *head;
    struct list_node *last_node = NULL;

    while (current_node)
    {
        last_node = current_node;
        current_node = current_node->next;
    }

    return last_node;
}

struct list_node *list_append(struct list_node **head, struct list_node *item)
{
    struct list_node *tail = list_get_tail(head);
    if (!tail)
    {
        *head = item;
    }
    else
    {
        tail->next = item;
    }

    item->next = NULL;
    return item;
}

struct list_node *list_pop(struct list_node **head)
{
    struct list_node *current_head = *head;
    if (!current_head)
    {
        return NULL;
    }

    *head = (*head)->next;
    current_head->next = NULL;

    return current_head;
}

struct list_node *list_find(struct list_node **head, struct list_node *item)
{
    struct list_node *current_node = *head;
    if (!current_node)
    {
        return NULL;
    }

    while (current_node)
    {
        if (current_node == item)
        {
            return current_node;
        }
        current_node = current_node->next;
    }

    return current_node;
}

struct list_node *list_remove(struct list_node **head, struct list_node *item)
{
    struct list_node *last_node = NULL;
    struct list_node *current_node = *head;
    if (!current_node)
    {
        return NULL;
    }

    //Check if head is the item to remove
    if (*head == item)
    {
        return list_pop(head);
    }

    while (current_node)
    {
        if (current_node == item)
        {
            last_node->next = current_node->next;
            current_node->next = NULL;
            return current_node;
        }
        last_node = current_node;
        current_node = current_node->next;
    }

    return current_node;
}

struct list_node *list_reverse(struct list_node **head)
{
    struct list_node *next_item;
    struct list_node *current_item;
    struct list_node *last_item = *head;
    if (!last_item)
    {
        return NULL;
    }

    current_item = (*head)->next;
    (*head)->next = NULL;

    while (current_item)
    {
        next_item = current_item->next;
        current_item->next = last_item;
        last_item = current_item;
        current_item = next_item;
    }

    *head = last_item;
    
    return last_item;
}