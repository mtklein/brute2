#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>

#define len(arr) (int)(sizeof(arr) / sizeof(*arr))
#define end(arr) (arr + len(arr))

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

static bool eval(Word word[], float const init[], float const *iend
                            , float const goal[], float const *gend) {
    float stack[64] = {0};
    float* const start = stack + 2;

    float *sp = start;
    while (init != iend) {
        *sp++ = *init++;
    }

    for (Word w; (w = *word++); sp = w(sp)) {
        if (sp < start || sp > end(stack)) {
            return false;
        }
    }

    for (float const *gp = gend; gp != goal;) {
        if (!equiv(*--sp, *--gp)) {
            return false;
        }
    }
    return sp == start;
}

static bool step(Word const dict[], Word const *dend,
                 Word const word[], Word       *wend) {
    if (wend == word) {
        return false;
    }
    Word* w = wend-1;

    for (Word const *d = dict; d != dend; d++) {
        if (*w == *d) {
            Word const *next = d+1;

            if (next == dend) {
                *w = *dict;
                return step(dict,dend, word,wend-1);
            } else {
                *w = *next;
                return true;
            }
        }
    }
    __builtin_unreachable();
}

static bool search(Word  const dict[], Word  const *dend,
                   float const init[], float const *iend,
                   float const goal[], float const *gend,
                   Word        word[], int   const words) {
    for (int len = 1; len < words-1; len++) {
        for (Word *w = word; w != word+len;) {
            *w++ = *dict;
        }
        word[len] = NULL;

        while (step(dict,dend, word,word+len)) {
            if (eval(word, init,iend, goal,gend)) {
                return true;
            }
        }
    }
    return false;
}

static bool test(float const init[], float const *iend,
                 float const goal[], float const *gend,
                 Word  const want[]) {
    Word const dict[] = { mul, sub, add, div, dup, swap, zero, one, inv };
    Word word[16];

    if (!search(dict, end(dict),
                init, iend,
                goal, gend,
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

#define expect(...) \
    if (!test(init,end(init), goal,end(goal), (Word[]){__VA_ARGS__})) return 1

int main(void) {
    {
        float init[] = {2,3,4}, goal[]={2,7};
        expect(add);
    }

    {
        float init[] = {1,2,3}, goal[]={7};
        expect(mul,add);
    }

    {
        float init[] = {3}, goal[]={6};
        expect(dup,add);
    }

    {
        float init[] = {3}, goal[]={9};
        expect(dup,mul);
    }

    {   // TODO: this is making use of underflow to let mul act as drop
        float init[] = {3}, goal[]={1};
        expect(mul,one);
    }

    {
        float init[] = {3}, goal[]={3,1};
        expect(one);
    }

    {
        float init[] = {3}, goal[]={1/3.0f};
        expect(inv);
    }

    {
        float init[] = {3}, goal[]={2/3.0f};
        expect(inv,dup,add);
    }

    {
        float init[] = {42}, goal[] = {42,2};
        expect(one,dup,add);
    }

    {
        float init[] = {42}, goal[] = {42,0.5};
        expect(one,dup,add,inv);
    }

    {
        float init[] = {42}, goal[] = {42,0.25};
        expect(one,dup,add,dup,mul,inv);
    }

    return 0;
}
