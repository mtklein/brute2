#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define len(arr) (sizeof(arr) / sizeof(*arr))

static bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

typedef float* Word(float*);

static float*  add(float* sp) { float x = *--sp, y = *--sp; *sp++ = x+y;            return sp; }
static float*  sub(float* sp) { float x = *--sp, y = *--sp; *sp++ = x-y;            return sp; }
static float*  mul(float* sp) { float x = *--sp, y = *--sp; *sp++ = x*y;            return sp; }
static float*  div(float* sp) { float x = *--sp, y = *--sp; *sp++ = x/y;            return sp; }
static float*  dup(float* sp) { float x = *--sp;            *sp++ =   x; *sp++ = x; return sp; }
static float* swap(float* sp) { float x = *--sp, y = *--sp; *sp++ =   x; *sp++ = y; return sp; }
static float* zero(float* sp) {                             *sp++ =   0;            return sp; }
static float*  one(float* sp) {                             *sp++ =   1;            return sp; }

static bool eval(Word* words[], const float init[], int ninit
                              , const float goal[], int ngoal) {
    float stack[1024*1024] = {0};
    memcpy(stack + len(stack)/2, init, sizeof(*init) * (size_t)ninit);

    float* sp = stack + len(stack)/2 + ninit;
    for (Word* word; (word = *words++); sp = word(sp));

    for (const float* gp = goal + ngoal; ngoal --> 0;) {
        if (!equiv(*--sp, *--gp)) {
            return false;
        }
    }
    return true;
}

static bool search(Word*       dict[], int ndict,
                   const float init[], int ninit,
                   const float goal[], int ngoal,
                   Word*      words[], int nwords) {
    const int max = nwords-1;
    words[max] = NULL;

    intmax_t limit = 1;
    for (int i = 0; i < max; i++) {
        limit *= ndict;
    }

    for (intmax_t index = 0; index < limit; index++) {
        for (intmax_t i = 0, ix = index; i < max; i++) {
            words[i] = dict[ix % ndict];
            ix /= ndict;
        }

        if (eval(words, init,ninit, goal,ngoal)) {
            return true;
        }
    }

    return false;
}

static bool search_and_display(const float init[], int ninit,
                               const float goal[], int ngoal) {
    Word* dict[] = { NULL, mul, sub, add, div, dup, swap, zero, one };
    Word* words[16];

    bool ok = search(dict, len(dict),
                     init, ninit,
                     goal, ngoal,
                    words, len(words));
    if (ok) {
        for (Word* *word = words; *word; word++) {
            void* addr = (void*)*word;
            Dl_info info;
            if (dladdr(addr, &info) && addr == info.dli_saddr) {
                printf("%s ", info.dli_sname);
            } else {
                printf("%p ", addr);
            }
        }
        printf("\n");
        return true;
    }
    return false;
}


int main(void) {
    {   // add
        float init[] = {2,3,4}, goal[]={2,7};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // mul add
        float init[] = {1,2,3}, goal[]={7};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // dup add
        float init[] = {3}, goal[]={6};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // dup mul
        float init[] = {3}, goal[]={9};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // dup div; one  (TODO: stack is left {3,1} here)
        float init[] = {3}, goal[]={1};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // dup dup div; one
        float init[] = {3}, goal[]={3,1};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    {   // dup dup div div; one div
        float init[] = {3}, goal[]={1/3.0f};
        if (!search_and_display(init,len(init), goal,len(goal))) { return 1; }
    }

    return 0;
}
