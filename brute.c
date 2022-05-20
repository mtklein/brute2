#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define len(arr) (sizeof(arr) / sizeof(*arr))

static bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

typedef float* (*Word)(float*);

static float*  add(float* sp) { float x = *--sp, y = *--sp; *sp++ = x+y;            return sp; }
static float*  sub(float* sp) { float x = *--sp, y = *--sp; *sp++ = x-y;            return sp; }
static float*  mul(float* sp) { float x = *--sp, y = *--sp; *sp++ = x*y;            return sp; }
static float*  div(float* sp) { float x = *--sp, y = *--sp; *sp++ = x/y;            return sp; }
static float*  dup(float* sp) { float x = *--sp;            *sp++ =   x; *sp++ = x; return sp; }
static float* swap(float* sp) { float x = *--sp, y = *--sp; *sp++ =   x; *sp++ = y; return sp; }
static float* zero(float* sp) {                             *sp++ =   0;            return sp; }
static float*  one(float* sp) {                             *sp++ =   1;            return sp; }
static float*  inv(float* sp) { float x = *--sp;            *sp++ = 1/x;            return sp; }

static bool eval(Word words[], const float init[], int ninit
                              , const float goal[], int ngoal) {
    float stack[64] = {0};
    float* const start = stack + 2;

    float* sp = start;
    while (ninit --> 0) {
        *sp++ = *init++;
    }

    for (Word word; (word = *words++); sp = word(sp)) {
        if (sp < start || sp > stack+len(stack)) {
            return false;
        }
    }

    for (const float* gp = goal + ngoal; ngoal --> 0;) {
        if (!equiv(*--sp, *--gp)) {
            return false;
        }
    }
    return sp == start;
}

static bool search(Word        dict[], const int ndict,
                   const float init[], const int ninit,
                   const float goal[], const int ngoal,
                   Word       words[], const int nwords) {
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

static bool search_and_display(const float init[], const int ninit,
                               const float goal[], const int ngoal,
                               Word        want[]) {
    Word dict[] = { NULL, mul, sub, add, div, dup, swap, zero, one, inv };
    Word words[16];

    bool ok = search(dict, len(dict),
                     init, ninit,
                     goal, ngoal,
                    words, len(words));
    if (ok) {
        if (want) {
            for (Word* word = words; *word; word++) {
                if (*word != *want++) {
                    return false;
                }
            }
        }

        FILE* const out = want ? stdout : stderr;
        for (Word* word = words; *word; word++) {
            void* addr = (void*)*word;
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
    return false;
}


int main(void) {
    {
        float init[] = {2,3,4}, goal[]={2,7};
        Word  want[] = {add};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {1,2,3}, goal[]={7};
        Word  want[] = {mul,add};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={6};
        Word  want[] = {dup,add};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={9};
        Word  want[] = {dup,mul};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={1};
        Word  want[] = {dup,div};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={3,1};
        Word  want[] = {one};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={1/3.0f};
        Word  want[] = {inv};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float init[] = {3}, goal[]={2/3.0f};
        Word  want[] = {inv,dup,add};
        if (!search_and_display(init,len(init), goal,len(goal), want)) { return 1; }
    }

    {
        float goal[]={2};
        Word  want[] = {one,dup,add};
        if (!search_and_display(NULL,0, goal,len(goal), want)) { return 1; }
    }

    {
        float goal[]={0.5};
        Word  want[] = {one,dup,add,inv};
        if (!search_and_display(NULL,0, goal,len(goal), want)) { return 1; }
    }

    return 0;
}
