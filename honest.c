// honest: hunts for a sentence that is true about two things at once: how many
// of each letter it contains, and exactly how many false sentences came before it.
//
//   honest seed                     walk until the first true sentence and print it
//   honest seed trace T first last  replay that walk, writing JSON of its steps to stdout
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *OPENING = "Thanks for the journey, Chuck: on your Mac mini, this sentence told exactly";
static const char *MIDDLE = "before it could truthfully say that it has";

#define MAXN 100
typedef struct { uint8_t c[32]; } __attribute__((aligned(32))) counts;

static counts number_letters[MAXN];
static counts group_letters[1000];
static counts fixed;

static const char *ONES[] = {"", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
    "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
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
    for (; *s; s++) {
        char ch = *s | 32;
        if (ch >= 'a' && ch <= 'z') into->c[ch - 'a']++;
    }
}

static inline void add(counts *a, const counts *b) {
    for (int j = 0; j < 32; j++) a->c[j] += b->c[j];
}

static uint64_t rng_state;
static inline uint64_t rng(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Everything in the sentence at step t except the 26 claims and the last three digits' words.
static void slow_part(uint64_t t, counts *into) {
    char w[160];
    *into = fixed;
    spell_lies(t - t % 1000, w);
    if (t >= 1000) tally(w, into);
    if (t < 1000 && t % 1000 == 0) tally("zero", into);
}

static void print_sentence(uint64_t t, const uint8_t *v) {
    char w[160];
    spell_lies(t, w);
    printf("%s %s %s %s", OPENING, w, t == 1 ? "lie" : "lies", MIDDLE);
    for (int i = 0; i < 26; i++) {
        spell_under_100(v[i], w);
        printf("%s%s %c%s", i == 0 ? " " : i == 25 ? ", and " : ", ", w, 'a' + i, v[i] == 1 ? "" : "'s");
    }
    printf(".\n");
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    rng_state = strtoull(argv[1], 0, 10);
    int trace = argc > 2 && !strcmp(argv[2], "trace");
    uint64_t stop_at = trace ? strtoull(argv[3], 0, 10) : UINT64_MAX;
    uint64_t first_n = trace ? strtoull(argv[4], 0, 10) : 0;
    uint64_t last_n = trace ? strtoull(argv[5], 0, 10) : 0;

    for (int n = 1; n < MAXN; n++) {
        char w[32];
        spell_under_100(n, w);
        tally(w, &number_letters[n]);
        if (n != 1) number_letters[n].c['s' - 'a']++;
    }
    for (int n = 1; n < 1000; n++) {
        char w[64];
        spell_group(n, w);
        tally(w, &group_letters[n]);
    }
    tally(OPENING, &fixed);
    tally("lies", &fixed);
    tally(MIDDLE, &fixed);
    tally("and", &fixed);
    for (int i = 0; i < 26; i++) fixed.c[i]++;

    uint8_t v[32] = {0};
    for (int i = 0; i < 26; i++) v[i] = 1;

    uint64_t by_false[27] = {0};
    int printed = 0;
    if (trace) printf("{\"steps\":[\n");

    counts slow, c;
    for (uint64_t t = 0; t <= stop_at; t++) {
        unsigned low = t % 1000;
        if (low == 0) slow_part(t, &slow);
        c = slow;
        add(&c, &group_letters[low]);
        if (t == 1) c.c['s' - 'a']--;
        for (int i = 0; i < 26; i++) add(&c, &number_letters[v[i]]);

        if (__builtin_expect(trace, 0)) {
            int wrong = 0;
            for (int i = 0; i < 26; i++) wrong += v[i] != c.c[i];
            by_false[wrong]++;
            int keep = t < first_n || t + last_n > stop_at;
            int near = wrong == 1;
            if (keep || near) {
                printf("%s[%llu,%d", printed++ ? ",\n" : "", (unsigned long long)t, near && !keep);
                for (int i = 0; i < 26; i++) printf(",%d", v[i]);
                printf("]");
            }
        }
        if (__builtin_expect(memcmp(v, c.c, 26) == 0, 0)) {
            if (trace) break;
            printf("TRUE lies=%llu seed=%s\n", (unsigned long long)t, argv[1]);
            print_sentence(t, v);
            return 0;
        }

        // About a quarter of the wrong claims get corrected on each step.
        uint64_t r = rng();
        for (int i = 0; i < 26; i++, r >>= 2)
            if ((r & 3) == 0) v[i] = c.c[i] >= MAXN ? MAXN - 1 : c.c[i];
    }
    if (trace) {
        printf("\n],\"by_false\":[");
        for (int i = 0; i < 27; i++) printf("%s%llu", i ? "," : "", (unsigned long long)by_false[i]);
        printf("]}\n");
    }
    return 0;
}
