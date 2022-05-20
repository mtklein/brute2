#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>

#define len(arr) (int)(sizeof(arr) / sizeof(*arr))

static bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

typedef float* (*Word)(float*);

static float*  add(float *sp) { float x = *--sp, y = *--sp; *sp++ = x+y;            return sp; }
static float*  sub(float *sp) { float x = *--sp, y = *--sp; *sp++ = x-y;            return sp; }
static float*  mul(float *sp) { float x = *--sp, y = *--sp; *sp++ = x*y;            return sp; }
static float*  div(float *sp) { float x = *--sp, y = *--sp; *sp++ = x/y;            return sp; }
static float*  dup(float *sp) { float x = *--sp;            *sp++ =   x; *sp++ = x; return sp; }
static float* swap(float *sp) { float x = *--sp, y = *--sp; *sp++ =   x; *sp++ = y; return sp; }
static float* zero(float *sp) {                             *sp++ =   0;            return sp; }
static float*  one(float *sp) {                             *sp++ =   1;            return sp; }
static float*  inv(float *sp) { float x = *--sp;            *sp++ = 1/x;            return sp; }

static bool eval(Word word[], float const init[], int ninit
                            , float const goal[], int ngoal) {
    float stack[64] = {0};
    float* const start = stack + 2;

    float *sp = start;
    while (ninit --> 0) {
        *sp++ = *init++;
    }

    for (Word w; (w = *word++); sp = w(sp)) {
        if (sp < start || sp > stack+len(stack)) {
            return false;
        }
    }

    for (float const *gp = goal + ngoal; ngoal --> 0;) {
        if (!equiv(*--sp, *--gp)) {
            return false;
        }
    }
    return sp == start;
}

static bool step(Word const dict[], int const ndict, Word *word, int nword) {
    if (nword == 0) {
        return false;
    }
    for (Word const *d = dict; d != dict+ndict; d++) {
        if (*word == *d) {
            Word const *next = d+1;

            if (next == dict+ndict) {
                *word = *dict;
                return step(dict,ndict, word-1,nword-1);
            } else {
                *word = *next;
                return true;
            }
        }
    }
    __builtin_unreachable();
}

static bool search(Word  const dict[], int const ndict,
                   float const init[], int const ninit,
                   float const goal[], int const ngoal,
                   Word        word[], int const nword) {
    for (int len = 1; len < nword-1; len++) {
        for (Word *w = word; w != word+len;) {
            *w++ = *dict;
        }
        word[len] = NULL;

        while (step(dict,ndict, word+len-1,len)) {
            if (eval(word, init,ninit, goal,ngoal)) {
                return true;
            }
        }
    }
    return false;
}

static bool test(float const init[], int const ninit,
                 float const goal[], int const ngoal,
                 Word  const want[]) {
    Word const dict[] = { mul, sub, add, div, dup, swap, zero, one, inv };
    Word word[16];

    if (!search(dict, len(dict),
                init, ninit,
                goal, ngoal,
                word, len(word))) {
        return false;
    }

    for (Word const *w = word; want && *w;) {
        if (*w++ != *want++) {
            return false;
        }
    }

    FILE* const out = want ? stdout : stderr;
    for (Word *w = word; *w; w++) {
        void *addr = (void*)*w;
        Dl_info info;
        if (dladdr(addr, &info) && addr == info.dli_saddr) {
            fprintf(out, "%s ", info.dli_sname);
        } else {
            fprintf(out, "%p ", addr);
        }
    }
    fprintf(out, "\n");

    return true;
}


int main(void) {
    {
        float init[] = {2,3,4}, goal[]={2,7};
        if (!test(init,len(init), goal,len(goal), (Word[]){add})) { return 1; }
    }

    {
        float init[] = {1,2,3}, goal[]={7};
        if (!test(init,len(init), goal,len(goal), (Word[]){mul,add})) { return 1; }
    }

    {
        float init[] = {3}, goal[]={6};
        if (!test(init,len(init), goal,len(goal), (Word[]){dup,add})) { return 1; }
    }

    {
        float init[] = {3}, goal[]={9};
        if (!test(init,len(init), goal,len(goal), (Word[]){dup,mul})) { return 1; }
    }

    {   // TODO: this is making use of the zeros before the start of the stack
        float init[] = {3}, goal[]={1};
        if (!test(init,len(init), goal,len(goal), (Word[]){mul,one})) { return 1; }
    }

    {
        float init[] = {3}, goal[]={3,1};
        if (!test(init,len(init), goal,len(goal), (Word[]){one})) { return 1; }
    }

    {
        float init[] = {3}, goal[]={1/3.0f};
        if (!test(init,len(init), goal,len(goal), (Word[]){inv})) { return 1; }
    }

    {
        float init[] = {3}, goal[]={2/3.0f};
        if (!test(init,len(init), goal,len(goal), (Word[]){inv,dup,add})) { return 1; }
    }

    {
        float goal[] = {2};
        if (!test(NULL,0, goal,len(goal), (Word[]){one,dup,add})) { return 1; }
    }

    {
        float goal[] = {0.5};
        if (!test(NULL,0, goal,len(goal), (Word[]){one,dup,add,inv})) { return 1; }
    }

    {
        float goal[] = {0.25};
        if (!test(NULL,0, goal,len(goal), (Word[]){one,dup,add,dup,mul,inv})) { return 1; }
    }

    return 0;
}
