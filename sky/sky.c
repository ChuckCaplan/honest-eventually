// Plots every sentence of the official walk as one point of light. Each sentence's 26 claims
// become a position, and each pixel remembers how many sentences landed on it and how false they were.
//   sky size projection stats_steps out.raw      projection -1 renders all four, side by side files
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *OPENING = "Thanks for the journey, Chuck: on your Mac mini, this sentence told exactly";
static const char *MIDDLE = "before it could truthfully say that it has";
#define MAXN 100
#define TRUTH 88426057ULL
#define SEED 20261105ULL
#define NP 4
typedef struct { uint8_t c[32]; } __attribute__((aligned(32))) counts;
static counts number_letters[MAXN], group_letters[1000], fixed;
static const char *ONES[] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
    "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
static const char *TENS[] = {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

static void spell_under_100(int n, char *out) {
    if (n < 20) strcpy(out, ONES[n]);
    else if (n % 10 == 0) strcpy(out, TENS[n / 10]);
    else sprintf(out, "%s-%s", TENS[n / 10], ONES[n % 10]);
}
static void spell_group(int n, char *out) {
    char rest[32];
    spell_under_100(n % 100, rest);
    if (n >= 100) sprintf(out, "%s hundred%s%s", ONES[n / 100], n % 100 ? " " : "", rest);
    else strcpy(out, rest);
}
static void spell_lies(uint64_t n, char *out) {
    static const char *SCALE[] = {"", " thousand", " million", " billion"};
    char g[64];
    out[0] = 0;
    if (n == 0) { strcpy(out, "zero"); return; }
    uint64_t div = 1000000000ULL;
    for (int s = 3; s >= 0; s--, div /= 1000) {
        int part = (n / div) % 1000;
        if (!part) continue;
        spell_group(part, g);
        if (out[0]) strcat(out, ", ");
        strcat(out, g);
        strcat(out, SCALE[s]);
    }
}
static void tally(const char *s, counts *into) {
    for (; *s; s++) { char ch = *s | 32; if (ch >= 'a' && ch <= 'z') into->c[ch - 'a']++; }
}
static inline void add(counts *a, const counts *b) { for (int j = 0; j < 32; j++) a->c[j] += b->c[j]; }
static uint64_t rng_state;
static inline uint64_t rng(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Letters from most to least restless over the walk.
static const char ORDER[] = "etsnioafhryulwvcdxgmkbjpqz";
static float wa[NP][26], wb[NP][26];
static void make_weights(void) {
    for (int k = 0; k < 26; k++) {
        int i = ORDER[k] - 'a';
        wa[0][i] = cos(2 * M_PI * k / 26); wb[0][i] = sin(2 * M_PI * k / 26);
        double r = pow(0.86, k);
        wa[1][i] = r * cos(k * 2.39996); wb[1][i] = r * sin(k * 2.39996);
    }
    #define L(ch) ((ch) - 'a')
    wa[2][L('e')] = 1; wa[2][L('n')] = .19; wa[2][L('f')] = .036;
    wb[2][L('t')] = 1; wb[2][L('i')] = .19; wb[2][L('h')] = .036;
    wa[3][L('e')] = 1; wa[3][L('n')] = atof(getenv("SKY_RA")); wa[3][L('o')] = atof(getenv("SKY_RB"));
    wb[3][L('s')] = 1; wb[3][L('t')] = atof(getenv("SKY_TA")); wb[3][L('i')] = atof(getenv("SKY_TB"));
}

typedef struct { uint32_t *n; uint64_t *wrong; double ma, mb, sa, sb; } plane;
static plane planes[NP];
static int size;
static uint64_t truth_x[NP], truth_y[NP];

static inline void locate(int p, double a, double b, int *x, int *y) {
    const plane *q = &planes[p];
    double u, v;
    if (p == 3) {
        double r = 0.1 + 0.85 * ((a - q->ma) / (3.2 * q->sa) + 1) / 2;
        double th = 2 * M_PI * b / atof(getenv("SKY_PERIOD"));
        u = 0.9 * r * cos(th); v = 0.9 * r * sin(th);
    } else {
        u = (a - q->ma) / (3.6 * q->sa); v = (b - q->mb) / (3.6 * q->sb);
    }
    *x = (int)((u + 1) * .5 * size);
    *y = (int)((v + 1) * .5 * size);
}

static void walk(uint64_t stop, int mode, int only) {
    rng_state = SEED;
    uint8_t v[32] = {0};
    for (int i = 0; i < 26; i++) v[i] = 1;
    counts slow, c;
    double sum_a[NP] = {0}, sum_b[NP] = {0}, sq_a[NP] = {0}, sq_b[NP] = {0};
    for (uint64_t t = 0; t <= stop; t++) {
        unsigned low = t % 1000;
        if (low == 0) {
            char w[160];
            slow = fixed;
            spell_lies(t, w);
            tally(w, &slow);
        }
        c = slow;
        add(&c, &group_letters[low]);
        if (t == 1) c.c['s' - 'a']--;
        for (int i = 0; i < 26; i++) add(&c, &number_letters[v[i]]);
        int wrong = 0;
        for (int i = 0; i < 26; i++) wrong += v[i] != c.c[i];
        for (int p = 0; p < NP; p++) {
            if (only >= 0 && p != only) continue;
            float a = 0, b = 0;
            for (int i = 0; i < 26; i++) { a += wa[p][i] * v[i]; b += wb[p][i] * v[i]; }
            if (mode == 0) { sum_a[p] += a; sum_b[p] += b; sq_a[p] += a * a; sq_b[p] += b * b; }
            else {
                int x, y;
                locate(p, a, b, &x, &y);
                if (wrong == 0) { truth_x[p] = x; truth_y[p] = y; }
                if (x >= 0 && y >= 0 && x < size && y < size) {
                    planes[p].n[y * size + x]++;
                    planes[p].wrong[y * size + x] += wrong;
                }
            }
        }
        if (wrong == 0) {
            if (t != TRUTH) { fprintf(stderr, "walk disagrees: truth at %llu\n", (unsigned long long)t); exit(1); }
            break;
        }
        uint64_t r = rng();
        for (int i = 0; i < 26; i++, r >>= 2)
            if ((r & 3) == 0) v[i] = c.c[i] >= MAXN ? MAXN - 1 : c.c[i];
    }
    if (mode == 0)
        for (int p = 0; p < NP; p++) {
            double n = stop + 1;
            planes[p].ma = sum_a[p] / n; planes[p].mb = sum_b[p] / n;
            planes[p].sa = sqrt(sq_a[p] / n - planes[p].ma * planes[p].ma);
            planes[p].sb = sqrt(sq_b[p] / n - planes[p].mb * planes[p].mb);
        }
}

int main(int argc, char **argv) {
    if (argc < 3) return 2;
    size = atoi(argv[1]);
    int only = atoi(argv[2]);
    const char *prefix = argc > 3 ? argv[3] : "plane";
    for (int n = 1; n < MAXN; n++) {
        char w[32];
        spell_under_100(n, w);
        tally(w, &number_letters[n]);
        if (n != 1) number_letters[n].c['s' - 'a']++;
    }
    for (int n = 1; n < 1000; n++) { char w[64]; spell_group(n, w); tally(w, &group_letters[n]); }
    tally(OPENING, &fixed); tally("lies", &fixed); tally(MIDDLE, &fixed); tally("and", &fixed);
    for (int i = 0; i < 26; i++) fixed.c[i]++;
    make_weights();
    walk(3000000, 0, only);
    for (int p = 0; p < NP; p++) {
        if (only >= 0 && p != only) continue;
        planes[p].n = calloc(size * size, 4);
        planes[p].wrong = calloc(size * size, 8);
    }
    walk(TRUTH, 1, only);
    for (int p = 0; p < NP; p++) {
        if (only >= 0 && p != only) continue;
        char name[128];
        sprintf(name, "%s%d.bin", prefix, p);
        FILE *f = fopen(name, "wb");
        uint64_t head[3] = {size, truth_x[p], truth_y[p]};
        fwrite(head, 8, 3, f);
        fwrite(planes[p].n, 4, size * size, f);
        fwrite(planes[p].wrong, 8, size * size, f);
        fclose(f);
        printf("plane %d truth at %llu,%llu\n", p, (unsigned long long)truth_x[p], (unsigned long long)truth_y[p]);
    }
    return 0;
}
