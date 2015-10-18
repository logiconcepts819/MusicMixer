/* vim:ts=4:sw=4:expandtab
 */

/* A very minimalistic FFT library */

#ifndef MINFFT_H
#define MINFFT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float *samples; /* size N (initially)  */
    float *cache;   /* size 7N/4 (initially) */
    float *r_tfac;  /* shared with cache memory */
    float *i_tfac;  /* shared with cache memory */
    int N;

} fft_plan_t;

typedef struct
{
    float start;
    float end;

} fft_range_t;

/* Initialization of an FFT plan */
int setup_plan(fft_plan_t * plan, float * samples, float * cache, int N);

/* Retrieval of cache size from a given FFT size */
static inline int plan_cache_size(int N)
{
    return N + (N >> 1) + (N >> 2);
}

/* Accessor for the samples in the FFT plan */
static inline float *plan_samples(fft_plan_t * plan)
{
    return plan->samples;
}

/* Retrieval of the range of frequencies represented by a particular bin */
static inline void bin_frequency_range(fft_plan_t * plan, int index, int rate,
                                       fft_range_t * range)
{
    float step_size = (float) rate / (float) plan->N;
    range->start = step_size * (float) index;
    range->end = range->start + step_size;
}

/* Real-to-half-complex transform */
void r2hc(fft_plan_t * plan);

/* Half-complex-to-real transform */
void hc2r(fft_plan_t * plan);

#ifdef __cplusplus
}
#endif

#endif
