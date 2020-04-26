// utils.c
//
// Helpers

//Helper Functions
void SWAP (int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
void SWAP_UL (unsigned long *a, unsigned long *b) {
    unsigned long tmp = *a;
    *a = *b;
    *b = tmp;
}
void SWAP_SHORT (short *a, short *b) {
    short tmp = *a;
    *a = *b;
    *b = tmp;
}
