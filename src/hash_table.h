// hash_table.h
// A real simple hash table, borrowed from K&R

#ifndef _hash_table_h
#define _hash_table_h

#define HASHSIZE 101

struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    char *name; /* defined name */
    int defn; /* replacement text, here I want to store integers */
};

static struct nlist *hashtab[HASHSIZE]; /* pointer table */

unsigned hash(char *s);
struct nlist *lookup(struct nlist** table, char *s);
struct nlist *install(struct nlist** table, char *name, int defn);

#endif
