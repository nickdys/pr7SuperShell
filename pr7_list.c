/* CMPSC 311 Project 7 Linked List Toolkit
 *
 * Author:  Nicholas Dyszel
 * Email:   nwd5069@psu.edu
 *
 * version 1.0, 7 Apr 2013
 *   implementation of process list
 */

#include <stdio.h>
#include <stdlib.h>
#include "pr7_list.h"

extern int verbose;

// Initializes list
void list_init(struct pr7_list * list)
{
  list->length = 0;
  list->head = NULL;
  list->tail = NULL;
  list->name = NULL;
}

// Adds a new process with the given PID to the head of the list
// Returns a pointer to the added node, otherwise returns NULL
struct pr7_process *list_add(struct pr7_list *list, pid_t pid)
{
  struct pr7_process *process = malloc(sizeof(struct pr7_process));
  if (process == NULL)
  {
    fprintf(stderr, "malloc() failed: %s: %d\n", __FILE__, __LINE__);
    return NULL;
  }
  if (verbose) { printf("%s: malloc() allocated %p\n", __FUNCTION__, process); }
  
  process->pid = pid;
  process->state = STATE_RUNNING;
  process->exit_status = 0;
  
  process->next = list->head;
  list->head = process;
  process->prev = NULL;
  if (process->next != NULL) process->next->prev = process;
  if (list->tail == NULL) list->tail = process;
  
  list->length++;
  
  return process;
}

// Removes the entry pointed to by entry from the list
int list_remove(struct pr7_list *list, struct pr7_process *entry)
{
  if (entry->prev == NULL) list->head = entry->next;
  else entry->prev->next = entry->next;
  
  if (entry->next == NULL) list->tail = entry->prev;
  else entry->next->prev = entry->prev;
  
  free(entry);
  //  if (verbose) { print something??? }
  
  list->length--;
  
  return list->length;
}

// Searches list for process with given PID, returns NULL if not found
struct pr7_process *list_search(struct pr7_list *list, pid_t key)
{
  if (list == NULL) return NULL;
  if (list->head == NULL) return NULL;
  
  struct pr7_process *p = list->head;
  while ((p != NULL) && (p->pid != key)) p = p->next;
  return p;
}

// Adds a new process with the given PID, returns pointer to addded node
// Ensures only one process with the given PID is in the list. Mode can be set
// to 0 (no overwrite) or 1 (overwrite) so if the PID already exists:
// 0: does not add new PID and returns NULL
// 1: updates existing PID and returns pointer to updated PID
struct pr7_process *list_add_once(struct pr7_list *list, pid_t pid, \
                                  int mode)
{
  struct pr7_process *found = list_search(list, pid);
  if (found == NULL) return list_add(list, pid);
  else if (mode == 0) return NULL;
  else if (mode == 1)
  { found->state = STATE_RUNNING; return found; }
  else
  { fprintf(stderr, "list_add_once: Invalid mode\n"); return NULL; }
}

// Updates the specified PID entry, does nothing if PID not found
struct pr7_process *list_update_entry(struct pr7_list *list, pid_t pid, int status)
{
  struct pr7_process *entry = list_search(list, pid);
  if (entry == NULL) return NULL;
  
  entry->state = STATE_TERMINATED;
  entry->exit_status = status;
  
  return entry;
}

// Removes (if possible) the specified PID from the list, returns new length
int list_remove_entry(struct pr7_list *list, pid_t pid)
{
  struct pr7_process *entry = list_search(list, pid);
  if (entry == NULL) return list->length;
  
  return list_remove(list, entry);
}

// Prints the contents of the list
void list_print(struct pr7_list *list)
{
  printf("%s: %d processes\n", list->name, list->length);
  
  struct pr7_process *entry = list->head;
  
  while (entry != NULL)
  {
    if (entry->state == STATE_RUNNING)
    { printf("  %6d: Running\n", entry->pid); }
    else if (entry->state == STATE_TERMINATED)
    {
      printf("  %6d: Exited with status %d\n", entry->pid, entry->exit_status);
    }
    
    entry = entry->next;
  }
  
  printf("---\n");
}
