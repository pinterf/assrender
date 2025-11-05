#include "timecodes.h"

int parse_timecodesv1(FILE* f, int total, udata* ud)
{
    int start, end, n = 0;
    double t = 0.0, basefps = 0.0, fps;
    char l[BUFSIZ];
    int64_t* ts = static_cast<int64_t*>(calloc(total, sizeof(int64_t)));

    if (!ts)
        return 0;

    while ((fgets(l, BUFSIZ - 1, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        if (sscanf(l, "Assume %lf", &basefps) == 1)
            continue;

        if (!(sscanf(l, "%d,%d,%lf", &start, &end, &fps) == 3))
            continue;

        if (basefps == 0.0)
            continue;

        while (n < start && n < total) {
            ts[n++] = (int64_t)(t + 0.5);
            t += 1000.0 / basefps;
        }

        while (n <= end && n < total) {
            ts[n++] = (int64_t)(t + 0.5);
            t += 1000.0 / fps;
        }
    }

    if (basefps == 0.0) {
        _aligned_free(ts);
        return 0;
    }

    while (n < total) {
        ts[n++] = (int64_t)(t + 0.5);
        t += 1000.0 / basefps;
    }

    ud->timestamp = ts;

    return 1;
}

int parse_timecodesv2(FILE* f, int total, udata* ud)
{
    int n = 0;
    int64_t* ts = static_cast<int64_t*>(calloc(total, sizeof(int64_t)));
    char l[BUFSIZ];

    if (!ts) {
        return 0;
    }

    while ((fgets(l, BUFSIZ - 1, f) != NULL) && n < total) {
        if (l[0] == 0 || l[0] == '\n' || l[0] == '\r' || l[0] == '#')
            continue;

        ts[n++] = atoll(l);
    }

    if (n < total) {
        _aligned_free(ts);
        return 0;
    }

    ud->timestamp = ts;

    return 1;
}
