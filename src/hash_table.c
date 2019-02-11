// hash_table.c - hashtable implementation from k&R

#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

/* Quick and easy hash function */
unsigned hash(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
      hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* Lookup a value */
struct nlist *lookup(struct nlist** table, char *s)
{
    struct nlist *np;
    for (np = table[hash(s)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0)
          return np; /* found */
    return NULL; /* not found */
}

/* Add a new value */
struct nlist *install(struct nlist** table, char *name, int defn)
{
    struct nlist *np;
    unsigned hashval;
    if ((np = lookup(table, name)) == NULL) { /* not found */
        np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL || (np->name = strdup(name)) == NULL)
          return NULL;
        hashval = hash(name);
        np->next = table[hashval];
        table[hashval] = np;
    } 
	np->defn = defn;
    return np;
}
