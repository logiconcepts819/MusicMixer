/* vim:ts=4:sw=4:expandtab
 */

/* A very minimalistic FFT library */

#include "minfft.h"
#include <math.h>

/* 2 pi */
#define TWO_PI 6.28318530717958647692528676655900576839433879875021164194988918

/* sqrt(2)/2 */
#define H_SQ_2 .707106781186547524400844362104849039284835937688474036588339869
#define SQRT_2 1.41421356237309504880168872420969807856967187537694807317667974

/* sin(pi/8) = cos(3 pi/8) = -cos(5 pi/8) = sin(7 pi/8) */
#define SN_1P8 .382683432365089771728459984030398866761344562485627041433800636
#define TSN1P8 .765366864730179543456919968060797733522689124971254082867601271

/* cos(pi/8) = sin(3 pi/8) = sin(5 pi/8) = -cos(7 pi/8) */
#define CS_1P8 .923879532511286756128183189396788286822416625863642486115097731
#define TCS1P8 1.84775906502257351225636637879357657364483325172728497223019546

#define RAD2_MASK 0x92492492
#define RAD4_MASK 0x24924924

/**
 * Format of time-domain data:
 *   x[0], x[1], x[2], ..., x[N-1]
 *
 * Format of frequency-domain data (just like FFTW):
 * - For even N:
 *   X[0], Re(X[1]), ..., Re(X[N/2-1]), X[N/2], Im(X[N/2-1]), ..., Im(X[1])
 * - For odd N (which is not the case here since I am only using radix 2/4/8):
 *   X[0], Re(X[1]), ..., Re(X[(N-1)/2]), Im(X[(N-1)/2]), ..., Im(X[1])
 **/

int setup_plan(fft_plan_t * plan, float * samples, float * cache, int N)
{
    int i;
    int quar_N;
    int ret = ((N & (N - 1)) == 0);

    /* Ensure that we are passed in a power of 2 */
    if (ret)
    {
        float * r_tfac_piby2;

        /* Set up samples and temporary cache */
        plan->samples = samples;
        plan->cache = cache;

        /* Set up N */
        plan->N = N;
        quar_N  = N >> 2;

        /* Set aside memory for twiddle factors */
        r_tfac_piby2 = cache + N + quar_N + quar_N;
        plan->r_tfac = cache + N + quar_N;
        plan->i_tfac = cache + N;

        /* Initialize the twiddle factors.  We only need to cover [0, pi) */
        for (i = 0; i < quar_N; i++)
        {
            float angle = TWO_PI * (float) i / (float) N;
            plan->r_tfac[i] = cos(angle);
            plan->i_tfac[i] = sin(angle);
            r_tfac_piby2[i] = -plan->i_tfac[i];
        }
    }

    /* Return success if a power of 2 was passed in */
    return ret;
}

/* First pass using the radix-2 real-to-half-complex transform */
static inline void pass2_r2hc(float * samples, float * cache, int half_N)
{
    /* Implementation based on FFTPACK, except the real parts and imaginary
     * parts are packed similar to FFTW */

    int i;

    for (i = 0; i < half_N; i++)
    {
        int in_index1 = i;
        int in_index2 = i + half_N;

        int out_index = i << 1;

        float val1 = samples[in_index1];
        float val2 = samples[in_index2];

        cache[out_index] = val1 + val2;
        cache[out_index + 1] = val1 - val2;
    }
}

// N = 1
// stride = N' / 4

/* First pass using the radix-4 real-to-half-complex transform */
static inline void pass4_r2hc(float * samples, float * cache, int quar_N)
{
    /* Implementation based on FFTPACK, except the real parts and imaginary
     * parts are packed similar to FFTW */

    int i;

    int half_N  = quar_N + quar_N;
    int thqu_N  = half_N + quar_N;

    for (i = 0; i < quar_N; i++)
    {
        int in_index1 = i;
        int in_index2 = i + quar_N;
        int in_index3 = i + half_N;
        int in_index4 = i + thqu_N;

        int out_index = i << 2;

        float val1 = samples[in_index1];
        float val2 = samples[in_index2];
        float val3 = samples[in_index3];
        float val4 = samples[in_index4];

        float r_temp1 = val2 + val4;
        float r_temp2 = val1 + val3;

        cache[out_index]     = r_temp1 + r_temp2;
        cache[out_index + 2] = r_temp2 - r_temp1;
        cache[out_index + 1] = val1 - val3;
        cache[out_index + 3] = val4 - val2;
    }
}

/* Radix-8 real-to-half-complex transform */
static inline void pass8_r2hc(float * samples, float * cache,
                              float * r_tfacs, float * i_tfacs,
                              int N, int stride)
{
    /* Implementation based on FFTPACK and the outputs from OCaml generators in
     * FFTW, except the real parts and imaginary parts are packed similar to
     * FFTW */

    int i, j;

    int twice_stride = stride       + stride;
    int three_stride = twice_stride + stride;
    int four_stride  = three_stride + stride;
    int five_stride  = four_stride  + stride;
    int six_stride   = five_stride  + stride;
    int seven_stride = six_stride   + stride;

    int half_N  = N >> 1;
    int twice_N = N       + N;
    int three_N = twice_N + N;
    int four_N  = three_N + N;
    int five_N  = four_N  + N;
    int six_N   = five_N  + N;
    int seven_N = six_N   + N;
    int eight_N = seven_N + N;

    for (i = 0; i < stride; i++)
    {
        int in_index1 = N *  i;
        int in_index2 = N * (i + stride);
        int in_index3 = N * (i + twice_stride);
        int in_index4 = N * (i + three_stride);
        int in_index5 = N * (i + four_stride);
        int in_index6 = N * (i + five_stride);
        int in_index7 = N * (i + six_stride);
        int in_index8 = N * (i + seven_stride);

        int out_index = i * eight_N;

        float val1 = samples[in_index1];
        float val2 = samples[in_index2];
        float val3 = samples[in_index3];
        float val4 = samples[in_index4];
        float val5 = samples[in_index5];
        float val6 = samples[in_index6];
        float val7 = samples[in_index7];
        float val8 = samples[in_index8];

        float r_temp1 = val1 + val5; // T3
        float r_temp2 = val1 - val5; // T7
        float r_temp3 = val8 - val4; // T13
        float r_temp4 = val8 + val4; // T19
        float r_temp5 = val3 + val7; // T6
        float r_temp6 = val3 - val7; // T16
        float r_temp7 = val2 - val6; // T10
        float r_temp8 = val2 + val6; // T18

        float r_tempa = H_SQ_2 * (r_temp7 + r_temp3); // T14
        float r_tempb = H_SQ_2 * (r_temp3 - r_temp7); // T15
        float r_tempc = r_temp1 + r_temp5; // T17
        float r_tempd = r_temp8 + r_temp4; // T20

        cache[out_index]           = r_tempc + r_tempd;
        cache[out_index + N]       = r_temp2 + r_tempa;
        cache[out_index + twice_N] = r_temp1 - r_temp5;
        cache[out_index + three_N] = r_temp2 - r_tempa;
        cache[out_index + four_N]  = r_tempc - r_tempd;
        cache[out_index + five_N]  = r_temp6 + r_tempb;
        cache[out_index + six_N]   = r_temp4 - r_temp8;
        cache[out_index + seven_N] = r_tempb - r_temp6;
    }

    if (N > 2)
    {
        for (i = 0; i < stride; i++)
        {
            int in_index1 = N *  i;
            int in_index2 = N * (i + stride);
            int in_index3 = N * (i + twice_stride);
            int in_index4 = N * (i + three_stride);
            int in_index5 = N * (i + four_stride);
            int in_index6 = N * (i + five_stride);
            int in_index7 = N * (i + six_stride);
            int in_index8 = N * (i + seven_stride);

            int out_index = i * eight_N;

            for (j = 1; j < half_N; j++)
            {
                int j_strided1 = stride * j;
                int j_strided2 = j_strided1 + j_strided1;
                int j_strided3 = j_strided2 + j_strided1;
                int j_strided4 = j_strided3 + j_strided1;
                int j_strided5 = j_strided4 + j_strided1;
                int j_strided6 = j_strided5 + j_strided1;
                int j_strided7 = j_strided6 + j_strided1;

                int j_c = eight_N - j;

                int j_d = N - j;
                int j_e = eight_N - j_d;

                int j_f = N + j;
                int j_g = eight_N - j_f;

                int j_h = N + j_d;
                int j_i = eight_N - j_h;

                int j_j = N + j_f;
                int j_k = eight_N - j_j;

                int j_l = N + j_h;
                int j_m = eight_N - j_l;

                int j_n = N + j_j;
                int j_o = eight_N - j_n;

                int j_p = N + j_l;
                int j_q = eight_N - j_p;

                float r_tfac1 = r_tfacs[j_strided1];
                float i_tfac1 = i_tfacs[j_strided1];

                float r_tfac2 = r_tfacs[j_strided2];
                float i_tfac2 = i_tfacs[j_strided2];

                float r_tfac3 = r_tfacs[j_strided3];
                float i_tfac3 = i_tfacs[j_strided3];

                float r_tfac4 = r_tfacs[j_strided4];
                float i_tfac4 = i_tfacs[j_strided4];

                float r_tfac5 = r_tfacs[j_strided5];
                float i_tfac5 = i_tfacs[j_strided5];

                float r_tfac6 = r_tfacs[j_strided6];
                float i_tfac6 = i_tfacs[j_strided6];

                float r_tfac7 = r_tfacs[j_strided7];
                float i_tfac7 = i_tfacs[j_strided7];

                float real_val_1 = samples[in_index1 + j];   // T1
                float imag_val_1 = samples[in_index1 + j_d]; // T70

                float real_val_2 = samples[in_index2 + j];   // T21
                float imag_val_2 = samples[in_index2 + j_d]; // T23

                float real_val_3 = samples[in_index3 + j];   // T9
                float imag_val_3 = samples[in_index3 + j_d]; // T11

                float real_val_4 = samples[in_index4 + j];   // T37
                float imag_val_4 = samples[in_index4 + j_d]; // T39

                float real_val_5 = samples[in_index5 + j];   // T3
                float imag_val_5 = samples[in_index5 + j_d]; // T5

                float real_val_6 = samples[in_index6 + j];   // T26
                float imag_val_6 = samples[in_index6 + j_d]; // T28

                float real_val_7 = samples[in_index7 + j];   // T14
                float imag_val_7 = samples[in_index7 + j_d]; // T16

                float real_val_8 = samples[in_index8 + j];   // T32
                float imag_val_8 = samples[in_index8 + j_d]; // T34

                // T24, T49
                float r_temp01 = real_val_2 * r_tfac1 + imag_val_2 * i_tfac1;
                float i_temp01 = imag_val_2 * r_tfac1 - real_val_2 * i_tfac1;

                // T12, T44
                float r_temp02 = real_val_3 * r_tfac2 + imag_val_3 * i_tfac2;
                float i_temp02 = imag_val_3 * r_tfac2 - real_val_3 * i_tfac2;

                // T40, T55
                float r_temp03 = real_val_4 * r_tfac3 + imag_val_4 * i_tfac3;
                float i_temp03 = imag_val_4 * r_tfac3 - real_val_4 * i_tfac3;

                // T6, T69
                float r_temp04 = real_val_5 * r_tfac4 + imag_val_5 * i_tfac4;
                float i_temp04 = imag_val_5 * r_tfac4 - real_val_5 * i_tfac4;

                // T29, T50
                float r_temp05 = real_val_6 * r_tfac5 + imag_val_6 * i_tfac5;
                float i_temp05 = imag_val_6 * r_tfac5 - real_val_6 * i_tfac5;

                // T17, T45
                float r_temp06 = real_val_7 * r_tfac6 + imag_val_7 * i_tfac6;
                float i_temp06 = imag_val_7 * r_tfac6 - real_val_7 * i_tfac6;

                // T35, T54
                float r_temp07 = real_val_8 * r_tfac7 + imag_val_8 * i_tfac7;
                float i_temp07 = imag_val_8 * r_tfac7 - real_val_8 * i_tfac7;

                float r_temp08 = r_temp01 + r_temp05; // T30
                float i_temp08 = i_temp01 + i_temp05; // T64

                float r_temp09 = r_temp01 - r_temp05; // T48
                float i_temp09 = i_temp01 - i_temp05; // T51

                float r_temp10 = r_temp02 + r_temp06; // T18
                float i_temp10 = i_temp02 - i_temp06; // T46

                float r_temp11 = r_temp02 - r_temp06; // T77
                float i_temp11 = i_temp02 + i_temp06; // T68

                float r_temp12 = r_temp07 + r_temp03; // T41
                float i_temp12 = i_temp07 + i_temp03; // T65

                float r_temp13 = r_temp07 - r_temp03; // T53
                float i_temp13 = i_temp07 - i_temp03; // T56

                float r_temp14 = real_val_1 + r_temp04; // T7
                float i_temp14 = imag_val_1 - i_temp04; // T76

                float r_temp15 = real_val_1 - r_temp04; // T43
                float i_temp15 = imag_val_1 + i_temp04; // T71

                float r_temp16 = r_temp14 + r_temp10; // T19
                float r_temp20 = r_temp14 - r_temp10; // T63

                float i_temp16 = i_temp15 + i_temp11; // T72
                float i_temp21 = i_temp15 - i_temp11; // T74

                float r_temp17 = r_temp08 + r_temp12; // T42
                float i_temp17 = i_temp08 + i_temp12; // T67

                float r_temp18 = r_temp15 + i_temp10; // T47
                float r_temp22 = r_temp15 - i_temp10; // T59

                float i_temp19 = i_temp14 - r_temp11; // T78
                float i_temp23 = i_temp14 + r_temp11; // T80

                float r_temp21 = i_temp08 - i_temp12; // T66
                float i_temp20 = r_temp12 - r_temp08; // T73

                float i_temp24 = r_temp13 + i_temp13; // T61
                float i_temp25 = r_temp13 - i_temp13; // T57

                float r_temp24 = i_temp09 - r_temp09; // T60
                float r_temp25 = i_temp09 + r_temp09; // T52

                float r_temp23 = H_SQ_2 * (r_temp24 - i_temp24); // T62
                float i_temp18 = H_SQ_2 * (r_temp24 + i_temp24); // T75

                float r_temp19 = H_SQ_2 * (r_temp25 + i_temp25); // T58
                float i_temp22 = H_SQ_2 * (i_temp25 - r_temp25); // T79

                cache[out_index + j]   = r_temp16 + r_temp17;
                cache[out_index + j_c] = i_temp16 + i_temp17;

                cache[out_index + j_f] = r_temp18 + r_temp19;
                cache[out_index + j_g] = i_temp18 + i_temp19;

                cache[out_index + j_j] = r_temp20 + r_temp21;
                cache[out_index + j_k] = i_temp20 + i_temp21;

                cache[out_index + j_n] = r_temp22 + r_temp23;
                cache[out_index + j_o] = i_temp22 + i_temp23;

                cache[out_index + j_p] = r_temp16 - r_temp17;
                cache[out_index + j_q] = i_temp17 - i_temp16;

                cache[out_index + j_l] = r_temp18 - r_temp19;
                cache[out_index + j_m] = i_temp18 - i_temp19;

                cache[out_index + j_h] = r_temp20 - r_temp21;
                cache[out_index + j_i] = i_temp20 - i_temp21;

                cache[out_index + j_d] = r_temp22 - r_temp23;
                cache[out_index + j_e] = i_temp22 - i_temp23;
            }
        }
    }

    if (N > 1)
    {
        for (i = 0; i < stride; i++)
        {
            int in_index1 = N *  i;
            int in_index2 = N * (i + stride);
            int in_index3 = N * (i + twice_stride);
            int in_index4 = N * (i + three_stride);
            int in_index5 = N * (i + four_stride);
            int in_index6 = N * (i + five_stride);
            int in_index7 = N * (i + six_stride);
            int in_index8 = N * (i + seven_stride);

            int out_index = i * eight_N;

            float val1 = samples[in_index1 + half_N];
            float val2 = samples[in_index2 + half_N];
            float val3 = samples[in_index3 + half_N];
            float val4 = samples[in_index4 + half_N];
            float val5 = samples[in_index5 + half_N];
            float val6 = samples[in_index6 + half_N];
            float val7 = samples[in_index7 + half_N];
            float val8 = samples[in_index8 + half_N];

            float temp01 =  val2 + val8;
            float temp02 =  val2 - val8;
            float temp03 =  H_SQ_2 * (val3 + val7);
            float temp04 =  H_SQ_2 * (val3 - val7);
            float temp05 =  val4 + val6;
            float temp06 =  val4 - val6;

            float temp07 =  val1 + temp04;
            float temp08 =  val1 - temp04;
            float temp09 =  temp03 - val5;
            float temp10 =  temp03 + val5;

            float temp11 =  SN_1P8 * temp05 - CS_1P8 * temp01;
            float temp12 = -SN_1P8 * temp01 - CS_1P8 * temp05;
            float temp13 =  SN_1P8 * temp06 + CS_1P8 * temp02;
            float temp14 =  SN_1P8 * temp02 - CS_1P8 * temp06;

            cache[out_index + half_N]           = temp07 + temp13;
            cache[out_index + half_N + seven_N] = temp12 - temp10;

            cache[out_index + half_N + N]       = temp08 + temp14;
            cache[out_index + half_N + six_N]   = temp11 - temp09;

            cache[out_index + half_N + twice_N] = temp08 - temp14;
            cache[out_index + half_N + five_N]  = temp11 + temp09;

            cache[out_index + half_N + three_N] = temp07 - temp13;
            cache[out_index + half_N + four_N]  = temp12 + temp10;
        }
    }
}

/* Real-to-half-complex transform */
void r2hc(fft_plan_t * plan)
{
    float * samples = plan->samples;
    float * cache = plan->cache;
    float * r_tfacs = plan->r_tfac;
    float * i_tfacs = plan->i_tfac;

    int curr_N = 1;
    int curr_stride = plan->N;

    float * temp;

    /* Check if bit number 3k + 1 in N is set.  If so, do the radix-2 FFT step
     * first.  Otherwise, check if bit number 3k + 2 in N is set.  If so, do
     * the radix-4 FFT step first. */
    if (curr_stride & RAD2_MASK)
    {
        /* Update the current stride */
        curr_stride >>= 1;

        /* Perform the radix-2 transform */
        pass2_r2hc(samples, cache, curr_stride);

        /* Update the current FFT size */
        curr_N <<= 1;

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }
    else if (curr_stride & RAD4_MASK)
    {
        /* Update the current stride */
        curr_stride >>= 2;

        /* Perform the radix-4 transform */
        pass4_r2hc(samples, cache, curr_stride);

        /* Update the current FFT size */
        curr_N <<= 2;

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }

    /* Now that the initial case has been completed, we can do a radix-8 FFT
     * the rest of the way. */
    while (curr_stride > 1)
    {
        /* Update the current stride */
        curr_stride >>= 3;

        /* Perform the radix-8 transform */
        pass8_r2hc(samples, cache, r_tfacs, i_tfacs, curr_N, curr_stride);

        /* Update the current FFT size */
        curr_N <<= 3;

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }

    /* Make final pointers swapped if necessary */
    plan->samples = samples;
    plan->cache = cache;
}

/* Final pass using the radix-2 half-complex-to-real transform */
static inline void pass2_hc2r(float * samples, float * cache, int half_N)
{
    /* Implementation based on FFTPACK, except the real parts and imaginary
     * parts are packed similar to FFTW */

    int i;

    for (i = 0; i < half_N; i++)
    {
        int out_index1 = i;
        int out_index2 = i + half_N;

        int in_index = i << 1;

        float val1 = samples[in_index];
        float val2 = samples[in_index + 1];

        cache[out_index1] = val1 + val2;
        cache[out_index2] = val1 - val2;
    }
}

/* Final pass using the radix-4 half-complex-to-real transform */
static inline void pass4_hc2r(float * samples, float * cache, int quar_N)
{
    /* Implementation based on FFTPACK, except the real parts and imaginary
     * parts are packed similar to FFTW */

    int i;

    int half_N = quar_N + quar_N;
    int thqu_N = half_N + quar_N;

    for (i = 0; i < quar_N; i++)
    {
        int out_index1 = i;
        int out_index2 = i + quar_N;
        int out_index3 = i + half_N;
        int out_index4 = i + thqu_N;

        int in_index = i << 2;

        float val1 = samples[in_index]; // CC(1,1,k)
        float val2 = samples[in_index + 1]; // CC(IDO,2,k)
        float val3 = samples[in_index + 3]; // CC(1,3,k)
        float val4 = samples[in_index + 2]; // CC(IDO,4,k)

        float r_temp1 = val1 - val4;
        float r_temp2 = val1 + val4;
        float r_temp3 = val2 + val2;
        float r_temp4 = val3 + val3;

        cache[out_index1] = r_temp2 + r_temp3;
        cache[out_index2] = r_temp1 - r_temp4;
        cache[out_index3] = r_temp2 - r_temp3;
        cache[out_index4] = r_temp1 + r_temp4;
    }
}

/* Radix-8 half-complex-to-real transform */
static inline void pass8_hc2r(float * samples, float * cache,
                              float * r_tfacs, float * i_tfacs,
                              int N, int stride)
{
    /* Implementation based on FFTPACK and the outputs from OCaml generators in
     * FFTW, except the real parts and imaginary parts are packed similar to
     * FFTW */

    int i, j;

    int twice_stride = stride       + stride;
    int three_stride = twice_stride + stride;
    int four_stride  = three_stride + stride;
    int five_stride  = four_stride  + stride;
    int six_stride   = five_stride  + stride;
    int seven_stride = six_stride   + stride;
    int half_N  = N >> 1;
    int twice_N = N       + N;
    int three_N = twice_N + N;
    int four_N  = three_N + N;
    int five_N  = four_N  + N;
    int six_N   = five_N  + N;
    int seven_N = six_N   + N;
    int eight_N = seven_N + N;

    for (i = 0; i < stride; i++)
    {
        int out_index1 = N *  i;
        int out_index2 = N * (i + stride);
        int out_index3 = N * (i + twice_stride);
        int out_index4 = N * (i + three_stride);
        int out_index5 = N * (i + four_stride);
        int out_index6 = N * (i + five_stride);
        int out_index7 = N * (i + six_stride);
        int out_index8 = N * (i + seven_stride);

        int in_index = i * eight_N;

        float val1 = samples[in_index];           // T1
        float val2 = samples[in_index + N];       // T7
        float val3 = samples[in_index + twice_N]; // T4
        float val4 = samples[in_index + three_N]; // T8
        float val5 = samples[in_index + four_N];  // T2
        float val6 = samples[in_index + five_N];  // T12
        float val7 = samples[in_index + six_N];   // T15
        float val8 = samples[in_index + seven_N]; // T11

        float r_temp1 = val3 + val3;       // T5
        float r_temp2 = val7 + val7;       // T16
        float r_temp3 = val1 + val5;       // T3
        float r_temp4 = val1 - val5;       // T14
        float r_temp5 = val2 + val4;       // intermediate of T9
        float r_temp6 = val2 - val4;       // T18
        float r_temp7 = r_temp5 + r_temp5; // T9
        float r_temp8 = val6 - val8;       // intermediate of T13
        float r_temp9 = val6 + val8;       // T19
        float r_temp0 = r_temp8 + r_temp8; // T13

        float r_tempa = r_temp3 + r_temp1;             // T6
        float r_tempb = r_temp3 - r_temp1;             // T10
        float r_tempc = r_temp4 - r_temp2;             // T17
        float r_tempd = r_temp4 + r_temp2;             // T21
        float r_tempe =  SQRT_2 * (r_temp6 - r_temp9); // T20
        float r_tempf = -SQRT_2 * (r_temp9 + r_temp6); // T22

        cache[out_index1] = r_tempa + r_temp7;
        cache[out_index2] = r_tempc + r_tempe;
        cache[out_index3] = r_tempb + r_temp0;
        cache[out_index4] = r_tempd + r_tempf;
        cache[out_index5] = r_tempa - r_temp7;
        cache[out_index6] = r_tempc - r_tempe;
        cache[out_index7] = r_tempb - r_temp0;
        cache[out_index8] = r_tempd - r_tempf;
    }

    if (N > 2)
    {
        for (i = 0; i < stride; i++)
        {
            int out_index1 = N *  i;
            int out_index2 = N * (i + stride);
            int out_index3 = N * (i + twice_stride);
            int out_index4 = N * (i + three_stride);
            int out_index5 = N * (i + four_stride);
            int out_index6 = N * (i + five_stride);
            int out_index7 = N * (i + six_stride);
            int out_index8 = N * (i + seven_stride);

            int in_index = i * eight_N;

            for (j = 1; j < half_N; j++)
            {
                int j_strided1 = stride * j;
                int j_strided2 = j_strided1 + j_strided1;
                int j_strided3 = j_strided2 + j_strided1;
                int j_strided4 = j_strided3 + j_strided1;
                int j_strided5 = j_strided4 + j_strided1;
                int j_strided6 = j_strided5 + j_strided1;
                int j_strided7 = j_strided6 + j_strided1;

                int j_c = eight_N - j;

                int j_d = N - j;
                int j_e = eight_N - j_d;

                int j_f = N + j;
                int j_g = eight_N - j_f;

                int j_h = N + j_d;
                int j_i = eight_N - j_h;

                int j_j = N + j_f;
                int j_k = eight_N - j_j;

                int j_l = N + j_h;
                int j_m = eight_N - j_l;

                int j_n = N + j_j;
                int j_o = eight_N - j_n;

                int j_p = N + j_l;
                int j_q = eight_N - j_p;

                float r_tfac1 = r_tfacs[j_strided1];
                float i_tfac1 = i_tfacs[j_strided1];

                float r_tfac2 = r_tfacs[j_strided2];
                float i_tfac2 = i_tfacs[j_strided2];

                float r_tfac3 = r_tfacs[j_strided3];
                float i_tfac3 = i_tfacs[j_strided3];

                float r_tfac4 = r_tfacs[j_strided4];
                float i_tfac4 = i_tfacs[j_strided4];

                float r_tfac5 = r_tfacs[j_strided5];
                float i_tfac5 = i_tfacs[j_strided5];

                float r_tfac6 = r_tfacs[j_strided6];
                float i_tfac6 = i_tfacs[j_strided6];

                float r_tfac7 = r_tfacs[j_strided7];
                float i_tfac7 = i_tfacs[j_strided7];

                float real_val_1 = samples[in_index + j];
                float imag_val_1 = samples[in_index + j_c];

                float real_val_2 = samples[in_index + j_d];
                float imag_val_2 = samples[in_index + j_e];

                float real_val_3 = samples[in_index + j_f];
                float imag_val_3 = samples[in_index + j_g];

                float real_val_4 = samples[in_index + j_h];
                float imag_val_4 = samples[in_index + j_i];

                float real_val_5 = samples[in_index + j_j];
                float imag_val_5 = samples[in_index + j_k];

                float real_val_6 = samples[in_index + j_l];
                float imag_val_6 = samples[in_index + j_m];

                float real_val_7 = samples[in_index + j_n];
                float imag_val_7 = samples[in_index + j_o];

                float real_val_8 = samples[in_index + j_p];
                float imag_val_8 = samples[in_index + j_q];

                float r_temp01 = real_val_1 + real_val_8;
                float i_temp01 = imag_val_1 + imag_val_8;

                float r_temp02 = real_val_1 - real_val_8;
                float i_temp02 = imag_val_1 - imag_val_8;

                float r_temp03 = real_val_2 + real_val_7;
                float i_temp03 = imag_val_2 + imag_val_7;

                float r_temp04 = real_val_2 - real_val_7;
                float i_temp04 = imag_val_7 - imag_val_2;

                float r_temp05 = real_val_3 + real_val_6;
                float i_temp05 = imag_val_3 + imag_val_6;

                float r_temp06 = real_val_3 - real_val_6;
                float i_temp06 = imag_val_3 - imag_val_6;

                float r_temp07 = real_val_5 + real_val_4;
                float i_temp07 = imag_val_5 + imag_val_4;

                float r_temp08 = real_val_5 - real_val_4;
                float i_temp08 = imag_val_5 - imag_val_4;

                float r_temp09 = r_temp01 + r_temp07;
                float i_temp09 = i_temp02 + i_temp08;

                float r_temp10 = r_temp05 + r_temp03;
                float i_temp21 = r_temp03 - r_temp05;

                float r_temp18 = r_temp01 - r_temp07;
                float i_temp18 = i_temp02 - i_temp08;

                float r_temp19 = r_temp08 + i_temp01;
                float i_temp19 = i_temp01 - r_temp08;

                float r_temp20 = r_temp02 - i_temp07;
                float i_temp20 = r_temp02 + i_temp07;

                float r_temp21 = i_temp06 - i_temp04;
                float i_temp10 = i_temp06 + i_temp04;

                float r_temp12 = r_temp18 + r_temp21;
                float i_temp12 = i_temp21 + i_temp18;

                float r_temp14 = r_temp09 - r_temp10;
                float i_temp14 = i_temp09 - i_temp10;

                float r_temp22 = r_temp04 + i_temp03;
                float i_temp22 = r_temp04 - i_temp03;

                float r_temp23 = r_temp06 + i_temp05;
                float i_temp23 = i_temp05 - r_temp06;

                float r_temp24 = H_SQ_2 * (r_temp23 + r_temp22);
                float i_temp24 = H_SQ_2 * (r_temp22 - r_temp23);

                float r_temp15 = i_temp20 - r_temp24;
                float r_temp11 = i_temp20 + r_temp24;

                float r_temp16 = r_temp18 - r_temp21;
                float i_temp16 = i_temp18 - i_temp21;

                float r_temp25 = H_SQ_2 * (i_temp23 + i_temp22);
                float i_temp25 = H_SQ_2 * (i_temp23 - i_temp22);

                float i_temp15 = i_temp19 - r_temp25;
                float i_temp11 = i_temp19 + r_temp25;

                float r_temp13 = r_temp20 + i_temp25;
                float r_temp17 = r_temp20 - i_temp25;

                float i_temp13 = r_temp19 + i_temp24;
                float i_temp17 = r_temp19 - i_temp24;

                cache[out_index1 + j]   = r_temp09 + r_temp10;
                cache[out_index1 + j_d] = i_temp09 + i_temp10;

                cache[out_index2 + j]   = r_tfac1 * r_temp11
                                        - i_tfac1 * i_temp11;
                cache[out_index2 + j_d] = i_tfac1 * r_temp11
                                        + r_tfac1 * i_temp11;

                cache[out_index3 + j]   = r_tfac2 * r_temp12
                                        - i_tfac2 * i_temp12;
                cache[out_index3 + j_d] = i_tfac2 * r_temp12
                                        + r_tfac2 * i_temp12;

                cache[out_index4 + j]   = r_tfac3 * r_temp13
                                        - i_tfac3 * i_temp13;
                cache[out_index4 + j_d] = i_tfac3 * r_temp13
                                        + r_tfac3 * i_temp13;

                cache[out_index5 + j]   = r_tfac4 * r_temp14
                                        - i_tfac4 * i_temp14;
                cache[out_index5 + j_d] = i_tfac4 * r_temp14
                                        + r_tfac4 * i_temp14;

                cache[out_index6 + j]   = r_tfac5 * r_temp15
                                        - i_tfac5 * i_temp15;
                cache[out_index6 + j_d] = i_tfac5 * r_temp15
                                        + r_tfac5 * i_temp15;

                cache[out_index7 + j]   = r_tfac6 * r_temp16
                                        - i_tfac6 * i_temp16;
                cache[out_index7 + j_d] = i_tfac6 * r_temp16
                                        + r_tfac6 * i_temp16;

                cache[out_index8 + j]   = r_tfac7 * r_temp17
                                        - i_tfac7 * i_temp17;
                cache[out_index8 + j_d] = i_tfac7 * r_temp17
                                        + r_tfac7 * i_temp17;
            }
        }
    }

    if (N > 1)
    {
        for (i = 0; i < stride; i++)
        {
            int out_index1 = N *  i;
            int out_index2 = N * (i + stride);
            int out_index3 = N * (i + twice_stride);
            int out_index4 = N * (i + three_stride);
            int out_index5 = N * (i + four_stride);
            int out_index6 = N * (i + five_stride);
            int out_index7 = N * (i + six_stride);
            int out_index8 = N * (i + seven_stride);

            int in_index = i * eight_N;

            float val1 = samples[in_index + half_N];
            float val2 = samples[in_index + half_N + seven_N];
            float val3 = samples[in_index + half_N + N];
            float val4 = samples[in_index + half_N + six_N];
            float val5 = samples[in_index + half_N + twice_N];
            float val6 = samples[in_index + half_N + five_N];
            float val7 = samples[in_index + half_N + three_N];
            float val8 = samples[in_index + half_N + four_N];

            float temp01 = val1 + val7;
            float temp02 = val1 - val7;
            float temp03 = val2 + val8;
            float temp04 = val2 - val8;
            float temp05 = val3 + val5;
            float temp06 = val3 - val5;
            float temp07 = val4 + val6;
            float temp08 = val4 - val6;

            float temp09 = temp05 + temp01;
            float temp10 = temp05 - temp01;
            float temp11 = temp06 - temp04;
            float temp12 = temp06 + temp04;
            float temp13 = temp07 + temp02;
            float temp14 = temp07 - temp02;
            float temp15 = temp08 - temp03;
            float temp16 = temp08 + temp03;

            cache[out_index1 + half_N] =  temp09 + temp09;
            cache[out_index2 + half_N] =  TCS1P8 * temp13 + TSN1P8 * temp11;
            cache[out_index3 + half_N] =  SQRT_2 * (temp15 - temp10);
            cache[out_index4 + half_N] = -TCS1P8 * temp12 - TSN1P8 * temp14;
            cache[out_index5 + half_N] = -temp16 - temp16;
            cache[out_index6 + half_N] =  TCS1P8 * temp11 - TSN1P8 * temp13;
            cache[out_index7 + half_N] =  SQRT_2 * (temp15 + temp10);
            cache[out_index8 + half_N] =  TCS1P8 * temp14 - TSN1P8 * temp12;
        }
    }
}

/* Half-complex-to-real transform */
void hc2r(fft_plan_t * plan)
{
    float * samples = plan->samples;
    float * cache = plan->cache;
    float * r_tfacs = plan->r_tfac;
    float * i_tfacs = plan->i_tfac;

    int curr_N = plan->N;
    int curr_stride = 1;

    float * temp;

    /* If we have at least three 2s in the prime factorization of N, take care
     * of the radix-8 FFT first */
    while (curr_N > 4)
    {
        /* Update the current FFT size */
        curr_N >>= 3;

        /* Perform the radix-8 transform */
        pass8_hc2r(samples, cache, r_tfacs, i_tfacs, curr_N, curr_stride);

        /* Update the current stride */
        curr_stride <<= 3;

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }

    /* Check if we are left with 4 as our current FFT size.  If so, do the
     * remaining radix-4 FFT step.  Otherwise, check if we are left with 2 as
     * our current FFT size.  If so, do the remaining radix-2 FFT step. */
    if (curr_N == 4)
    {
        /* Perform the final radix-4 transform */
        pass4_hc2r(samples, cache, curr_stride);

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }
    else if (curr_N == 2)
    {
        /* Perform the final radix-2 transform */
        pass2_hc2r(samples, cache, curr_stride);

        /* Swap pointers */
        temp = samples;
        samples = cache;
        cache = temp;
    }

    /* Make final pointers swapped if necessary */
    plan->samples = samples;
    plan->cache = cache;
}
