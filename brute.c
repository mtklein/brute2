#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define len(arr) (sizeof(arr) / sizeof(*arr))

static bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

typedef float* Word(float*);

static float* add(float* sp) { float x = *--sp, y = *--sp; *sp++ = x+y; return sp; }
static float* sub(float* sp) { float x = *--sp, y = *--sp; *sp++ = x-y; return sp; }
static float* mul(float* sp) { float x = *--sp, y = *--sp; *sp++ = x*y; return sp; }
static float* div(float* sp) { float x = *--sp, y = *--sp; *sp++ = x/y; return sp; }

static bool eval(Word* words[], const float init[], int ninit
                              , const float goal[], int ngoal) {
    float stack[1024*1024];
    memcpy(stack, init, sizeof(*init) * (size_t)ninit);

    float* sp = stack + ninit;
    for (Word* word; (word = *words++); sp = word(sp));

    for (const float* gp = goal + ngoal; ngoal --> 0;) {
        if (!equiv(*--sp, *--gp)) {
            return false;
        }
    }
    return true;
}

#define MAX 10
static bool search(Word*       dict[], int ndict,
                   const float init[], int ninit,
                   const float goal[], int ngoal,
                   Word* words[static MAX+1]) {
    words[MAX] = NULL;

    int limit = 1;
    for (int i = 0; i < MAX; i++) {
        limit *= ndict;
    }

    for (int index = 0; index < limit; index++) {
        for (int i = 0, ix = index; i < MAX; i++) {
            words[i] = dict[ix % ndict];
            ix /= ndict;
        }

        if (eval(words, init,ninit, goal,ngoal)) {
            return true;
        }
    }

    return false;
}

int main(void) {
    Word* dict[] = { NULL, mul, sub, add, div };
    const float init[] = {2,3,4},
                goal[] = {2,7};
    Word* words[MAX+1];

    bool ok = search(dict, len(dict),
                     init, len(init),
                     goal, len(goal),
                     words);
    if (ok) {
        for (Word* *word = words; *word; word++) {
            void* addr = (void*)*word;
            printf("%p", addr);
            Dl_info info;
            if (dladdr(addr, &info) && addr == info.dli_saddr) {
                printf(" %s", info.dli_sname);
            }
            printf("\n");
        }
        return 0;
    }
    return 1;
}
