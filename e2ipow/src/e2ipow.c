/*
 * e2ipow - proof of work solvers of E2iPlayer, the native versions of its
 * Python solvers (about a hundred times faster on the box)
 *
 *   e2ipow <algorithm> <arguments...>
 *   e2ipow --list       one supported algorithm per line
 *   e2ipow --version
 *
 * Exit codes of every algorithm: 0 solved, 1 internal error, 2 usage or
 * unknown algorithm, 3 time limit reached.
 *
 * byse - the Byse video player (E2iPlayer urlparser parserBYSE), same
 * arguments and output as E2iPlayer's scripts/bysepow.py:
 *
 *   e2ipow byse <pow_nonce> <difficulty> <start> <step> <timeout seconds>
 *
 * Finds the first counter s = start, start + step, ... whose hash of
 * "<pow_nonce>:<s>" starts with <difficulty> zero bits and prints one JSON
 * line {"solution": "<counter>" or null, "hashes": <tried>}. The hash is the
 * player's gr()/wr() (assets/pow-*.js): a 512-word ChaCha-style construction
 * on a 4-word state. Bytes are taken as they are, like the player's yr()
 * (char code & 255) for the ASCII nonces it sends.
 *
 * A new algorithm is one more entry in the solvers table at the end.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define E2IPOW_VERSION "1.0"

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* the player's ChaCha quarter round */
#define QR(a, b, c, d) do { \
        a += b; d ^= a; d = ROTL(d, 16); \
        c += d; b ^= c; b = ROTL(b, 12); \
        a += b; d ^= a; d = ROTL(d, 8); \
        c += d; b ^= c; b = ROTL(b, 7); \
    } while (0)

typedef struct {
    uint32_t a, b, c, d;
} pow_state_t;

static void absorb(pow_state_t *st, const unsigned char *data, size_t len)
{
    uint32_t a = st->a, b = st->b, c = st->c, d = st->d;
    size_t i;

    for (i = 0; i < len; i++) {
        a += data[i];
        a = ROTL(a, 7);
        QR(a, b, c, d);
    }
    st->a = a;
    st->b = b;
    st->c = c;
    st->d = d;
}

/* number of leading zero bits of the hash of (absorbed prefix + tail); the
 * eight output words are only computed until one is non-zero */
static int leading_zeros(const pow_state_t *prefix, const unsigned char *tail, size_t len)
{
    pow_state_t st = *prefix;
    uint32_t r[512];
    uint32_t a, b, c, d;
    int i, pass, zeros;

    absorb(&st, tail, len);
    a = st.a;
    b = st.b;
    c = st.c;
    d = st.d;

    for (i = 0; i < 8; i++)
        QR(a, b, c, d);
    for (i = 0; i < 512; i++) {
        QR(a, b, c, d);
        r[i] = a ^ c;
    }
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < 512; i++) {
            uint32_t x = r[i];
            x += r[x & 511];
            x = ROTL(x, 13);
            x ^= r[(i + 1) & 511] * 2654435761u;
            r[i] = x;
            a ^= x;
            QR(a, b, c, d);
        }
    }
    zeros = 0;
    for (i = 0; i < 512; i += 64) {
        uint32_t s;
        int k;

        QR(a, b, c, d);
        s = a;
        for (k = i; k < i + 64; k++) {
            s += r[k];
            s = ROTL(s, 5);
            s ^= r[k] * 2246822519u;
        }
        s ^= c;
        if (s)
            return zeros + __builtin_clz(s);
        zeros += 32;
    }
    return zeros;
}

static double now_seconds(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int parse_ull(const char *text, unsigned long long *value)
{
    char *end = NULL;

    if (!text[0] || text[0] == '-')
        return -1;
    errno = 0;
    *value = strtoull(text, &end, 10);
    return (errno || *end) ? -1 : 0;
}

/* argv[0] is the algorithm name, argv[1..5] its arguments */
static int byse(int argc, char *argv[])
{
    pow_state_t prefix = {1779033703u, 3144134277u, 1013904242u, 2773480762u};
    unsigned long long start, step, s, tried = 0;
    long difficulty;
    double timeout, begin;
    char *end = NULL;
    char counter[24];

    if (argc != 6) {
        fprintf(stderr, "usage: e2ipow byse <pow_nonce> <difficulty> <start> <step> <timeout>\n");
        return 2;
    }
    errno = 0;
    difficulty = strtol(argv[2], &end, 10);
    if (errno || *end || !argv[2][0] || parse_ull(argv[3], &start) || parse_ull(argv[4], &step)) {
        fprintf(stderr, "e2ipow: bad arguments\n");
        return 2;
    }
    timeout = strtod(argv[5], &end);
    if (*end || !argv[5][0]) {
        fprintf(stderr, "e2ipow: bad arguments\n");
        return 2;
    }
    if (step < 1)
        step = 1;

    if (difficulty <= 0) {
        printf("{\"solution\": \"0\", \"hashes\": 0}\n");
        return 0;
    }

    absorb(&prefix, (const unsigned char *)argv[1], strlen(argv[1]));
    absorb(&prefix, (const unsigned char *)":", 1);

    begin = now_seconds();
    s = start;
    for (;;) {
        int i;

        for (i = 0; i < 1024; i++) {
            int len = snprintf(counter, sizeof(counter), "%llu", s);

            tried++;
            if (leading_zeros(&prefix, (const unsigned char *)counter, (size_t)len) >= difficulty) {
                printf("{\"solution\": \"%s\", \"hashes\": %llu}\n", counter, tried);
                return fflush(stdout) ? 1 : 0;
            }
            s += step;
        }
        if (now_seconds() - begin > timeout) {
            printf("{\"solution\": null, \"hashes\": %llu}\n", tried);
            fflush(stdout);
            return 3;
        }
    }
}

static const struct {
    const char *name;
    int (*run)(int argc, char *argv[]);
} solvers[] = {
    {"byse", byse},
};

#define SOLVERS ((int)(sizeof(solvers) / sizeof(solvers[0])))

int main(int argc, char *argv[])
{
    int i;

    if (argc == 2 && !strcmp(argv[1], "--version")) {
        printf("e2ipow %s\n", E2IPOW_VERSION);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--list")) {
        for (i = 0; i < SOLVERS; i++)
            printf("%s\n", solvers[i].name);
        return 0;
    }
    for (i = 0; argc >= 2 && i < SOLVERS; i++) {
        if (!strcmp(argv[1], solvers[i].name)) {
            /* the box keeps playing / recording while all cores are busy here */
            if (nice(10) == -1 && errno)
                errno = 0;
            return solvers[i].run(argc - 1, argv + 1);
        }
    }
    fprintf(stderr, "usage: e2ipow <algorithm> <arguments...> | --list | --version\n");
    return 2;
}
