/**
 * $Id$
 *
 * @brief Red Pitaya Spectrum Analyzer main module.
 *
 * @Author Jure Menart <juremenart@gmail.com>
 *         
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/param.h>

#include "main.h"
#include "version.h"
#include "worker.h"
#include "fpga.h"

/* Describe app. parameters with some info/limitations */
static rp_app_params_t rp_main_params[PARAMS_NUM+1] = {
    { /* xmin - currently ignored */ 
        "xmin", -1000000, 0, 1, -10000000, +10000000 },
    { /* xmax - currently ignored   */ 
        "xmax", +1000000, 0, 1, -10000000, +10000000 },
    { /* freq_range::
       *    0 - 62.5 [MHz]
       *    1 - 7.8 [MHz]
       *    2 - 976 [kHz]
       *    3 - 61 [kHz]
       *    4 - 7.6 [kHz]
       *    5 - 953 [Hz] */
        "freq_range", 0, 1, 0,         0,         5 },
    { /* freq_unit:
       *    0 - [Hz]
       *    1 - [kHz]
       *    2 - [MHz]   */
        "freq_unit",  0, 0, 1,         0,         2 },
    { /* peak1_freq */
        "peak1_freq", 0, 0, 1,         0,         +1e6 },
    { /* peak1_power    */
        "peak1_power", 0, 0, 1, -10000000, +10000000 },
    { /* peak1_unit - same enumeration as freq_unit */
        "peak1_unit", 0, 0, 1,       0,       2 },
    { /* peak2_freq */
        "peak2_freq", 0, 0, 1,         0,         1e6 },
    { /* time_range */
        "peak2_power", 0, 0, 1,         -1e7, 1e7 },
    { /* peak2_unit - same enumeration as freq_unit */
        "peak2_unit", 0, 0, 1,         0,         2 },
    { /* JPG Waterfall files index */
        "w_idx", 0, 0, 1, 0, 1000 },
	{ /* en_avg_at_dec:
		   *    0 - disable
		   *    1 - enable */
		"en_avg_at_dec", 1, 0, 1,      0,         1 },
    { /* Must be last! */
        NULL, 0.0, -1, -1, 0.0, 0.0 }
};

/* params initialized */
static int params_init = 0;

const char *rp_app_desc(void)
{
    return (const char *)"Red Pitaya spectrum analyser application.\n";
}

int rp_app_init(int binning)
{
//  fprintf(stderr, "Loading spectrum version %s-%s.\n", VERSION_STR, REVISION_STR);

    if(rp_spectr_worker_init(binning) < 0) {
        return -1;
    }

    rp_set_params(&rp_main_params[0], PARAMS_NUM);

    rp_spectr_worker_change_state(rp_spectr_auto_state);

    return 0;
}

int rp_app_exit(void)
{
//  fprintf(stderr, "Unloading spectrum version %s-%s.\n", VERSION_STR, REVISION_STR);

    rp_spectr_worker_exit();

    return 0;
}

int rp_set_params(rp_app_params_t *p, int len)
{
    int i;
    int fpga_update = 1;
    int params_change = 0;

    if(len > PARAMS_NUM) {
        fprintf(stderr, "Too much parameters, max=%d\n", PARAMS_NUM);
        return -1;
    }

    for(i = 0; i < len; i++) {
        int p_idx = -1;
        int j = 0;

        /* Search for correct parameter name in defined parameters */
        while(rp_main_params[j].name != NULL) {
            int p_strlen = strlen(p[i].name);

            if(p_strlen != strlen(rp_main_params[j].name)) {
                j++;
                continue;
            }
            if(!strncmp(p[i].name, rp_main_params[j].name, p_strlen)) {
                p_idx = j;
                break;
            }
            j++;
        }

        if(p_idx == -1) {
            fprintf(stderr, "Parameter %s not found, ignoring it\n", p[i].name);
            continue;
        }

        if(rp_main_params[p_idx].read_only) {
            continue;
        }

        if(rp_main_params[p_idx].value != p[i].value) {
            params_change = 1;
            if(rp_main_params[p_idx].fpga_update)
                fpga_update = 1;
        }
        if(rp_main_params[p_idx].min_val > p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (min:%f), "
                    " correcting it\n", p[i].value, 
                    rp_main_params[p_idx].min_val);
            p[i].value = rp_main_params[p_idx].min_val;
        } else if(rp_main_params[p_idx].max_val < p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (max:%f), "
                    " correcting it\n", p[i].value, 
                    rp_main_params[p_idx].max_val);
            p[i].value = rp_main_params[p_idx].max_val;
        }

        rp_main_params[p_idx].value = p[i].value;
    }

    if(params_change || (params_init == 0)) {
        params_init = 1;

        rp_main_params[FREQ_UNIT_PARAM].value = 
            rp_main_params[PEAK_UNIT_CHA_PARAM].value =
            rp_main_params[PEAK_UNIT_CHB_PARAM].value =
            spectr_fpga_cnv_freq_range_to_unit(
                                         rp_main_params[FREQ_RANGE_PARAM].value);
        fprintf(stderr, "Setting FREQ_UNIT_PARAM=%f\n", rp_main_params[FREQ_UNIT_PARAM].value);

        rp_spectr_worker_update_params((rp_app_params_t *)&rp_main_params[0],
                                       fpga_update);
    }

    return 0;
}

/* Returned vector must be free'd externally! */
int rp_get_params(rp_app_params_t **p)
{
    rp_app_params_t *p_copy = NULL;
    
    int i;

    p_copy = (rp_app_params_t *)malloc((PARAMS_NUM+1) * sizeof(rp_app_params_t));
    if(p_copy == NULL)
        return -1;

    for(i = 0; i < PARAMS_NUM; i++) {
        int p_strlen = strlen(rp_main_params[i].name);
        p_copy[i].name = (char *)malloc(p_strlen+1);
        strncpy((char *)&p_copy[i].name[0], &rp_main_params[i].name[0], 
                p_strlen);
        p_copy[i].name[p_strlen]='\0';

        p_copy[i].value       = rp_main_params[i].value;
        p_copy[i].fpga_update = rp_main_params[i].fpga_update;
        p_copy[i].read_only   = rp_main_params[i].read_only;
        p_copy[i].min_val     = rp_main_params[i].min_val;
        p_copy[i].max_val     = rp_main_params[i].max_val;
    }
    p_copy[PARAMS_NUM].name = NULL;

    *p = p_copy;
    return PARAMS_NUM;
}

int rp_get_signals(float ***s, int *sig_num, int *sig_len, int binning)
{
    int ret_val;

    rp_spectr_worker_res_t result;

    if(*s == NULL)
        return -1;

    *sig_num = SPECTR_OUT_SIG_NUM;
    *sig_len = SPECTR_FFT_SIG_LEN/binning /* was SPECTR_OUT_SIG_LEN */;

    ret_val = rp_spectr_get_signals(s, &result, binning);

    /* Old signal */
    if(ret_val < 0) {
        return -1;
    }

    rp_main_params[JPG_FILE_IDX_PARAM].value     = (float)result.jpg_idx;
    rp_main_params[PEAK_PW_CHA_PARAM].value      = (float)result.peak_pw_cha;
    rp_main_params[PEAK_PW_FREQ_CHA_PARAM].value = (float)result.peak_pw_freq_cha;
    rp_main_params[PEAK_PW_CHB_PARAM].value      = (float)result.peak_pw_chb;
    rp_main_params[PEAK_PW_FREQ_CHB_PARAM].value = (float)result.peak_pw_freq_chb;


    return 0;
}

int rp_create_signals(float ***a_signals, int binning)
{
    int i;
    float **s;

    s = (float **)malloc(SPECTR_OUT_SIG_NUM * sizeof(float *));
    if(s == NULL) {
        return -1;
    }
    for(i = 0; i < SPECTR_OUT_SIG_NUM; i++)
        s[i] = NULL;

    for(i = 0; i < SPECTR_OUT_SIG_NUM; i++) {
        s[i] = (float *)malloc(SPECTR_FFT_SIG_LEN/binning/* was SPECTR_OUT_SIG_LEN=2k */ * sizeof(float));
        if(s[i] == NULL) {
            rp_cleanup_signals(a_signals);
            return -1;
        }
        memset(&s[i][0], 0, SPECTR_FFT_SIG_LEN/binning /* was SPECTR_OUT_SIG_LEN=2k */ * sizeof(float));
    }
    *a_signals = s;

    return 0;
}

void rp_cleanup_signals(float ***a_signals)
{
    int i;
    float **s = *a_signals;

    if(s) {
        for(i = 0; i < SPECTR_OUT_SIG_NUM; i++) {
            if(s[i]) {
                free(s[i]);
                s[i] = NULL;
            }
        }
        free(s);
        *a_signals = NULL;
    }
}

/** Minimal number of command line arguments */
#define MINARGS 4

/** Program name */
const char *g_argv0 = NULL;

enum SA_PARMS
{
  APP,
  CHANNEL,
  DECIMATION,
  BINNING
};

/** Max channel index */
#define CH_MAX 2
/** Channel translation table */
static int g_ch[CH_MAX] = { 1,  2 };

/** Max decimation index */
#define DEC_MAX 6
/** Decimation translation table */
static int g_dec[DEC_MAX] = { 1,  8,  64,  1024,  8192,  65536 };

/** Max binning index */
#define BIN_MAX 9
/** Binning translation table */
static int g_bin[BIN_MAX] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };

#define SIGNAL_LENGTH (16*1024)

/** Print usage information */
void usage() {

    const char *format =
            "Usage: %s CH DEC BIN\n"
            "\n"
            "    CH                     Channel [%u,%u]\n"
            "    DEC                 Decimation [%u,%u,%u,%u,%u,%u]\n"
            "    BIN                    Binning [%u,%u,%u,%u,%u,%u,%u,%u,%u]\n"
            "\nNumber of frequency domain values returned is %u / Binning\n"
            "v0.2 :: Equalization filter coefficients set to ZERO\n";

    fprintf( stderr, format, g_argv0,
             g_ch[0], g_ch[1],
             g_dec[0], g_dec[1], g_dec[2], g_dec[3], g_dec[4], g_dec[5],
             g_bin[0], g_bin[1], g_bin[2], g_bin[3], g_bin[4], g_bin[5], g_bin[6], g_bin[7], g_bin[8],
             SPECTR_FFT_SIG_LEN);
}

int main(int argc, char *argv[])
{
    uint32_t idx;
    g_argv0 = argv[0];

    if ( argc < MINARGS ) {
        usage();
        exit ( EXIT_FAILURE );
    }

    /* Channel */
    uint32_t channel = atoi(argv[CHANNEL]);
    for (idx = 0; idx < CH_MAX; idx++) {
        if (channel == g_ch[idx]) {
            break;
        }
    }
    if (idx == CH_MAX) {
        fprintf(stderr, "Invalid channel CH: %s\n", argv[CHANNEL]);
        usage();
        return -1;
    }

    /* Decimation */
    uint32_t dec = atoi(argv[DECIMATION]);
    for (idx = 0; idx < DEC_MAX; idx++) {
        if (dec == g_dec[idx]) {
            break;
        }
    }
    if (idx != DEC_MAX) {
        rp_main_params[FREQ_RANGE_PARAM].value = idx;//Setting decimation
    } else {
        fprintf(stderr, "Invalid decimation DEC: %s\n", argv[DECIMATION]);
        usage();
        return -1;
    }

    /* Binning */
    int binning = atoi(argv[BINNING]);
    for (idx = 0; idx < BIN_MAX; idx++) {
        if (binning == g_bin[idx]) {
            break;
        }
    }
    if (idx == BIN_MAX) {
        fprintf(stderr, "Invalid binning BIN: %s\n", argv[BINNING]);
        usage();
        return -1;
    }

    /* Initialization of application */
    if(rp_app_init(binning) < 0) {
        fprintf(stderr, "rp_app_init() failed!\n");
        return -1;
    }

    {
        float **s;
        int sig_num, sig_len;
        int retries = 150000;
        int i, ret_val;
//      int size = SPECTR_OUT_SIG_LEN;//2k

	s = (float **)malloc(SPECTR_OUT_SIG_NUM * sizeof(float *));
        for(i = 0; i < SPECTR_OUT_SIG_NUM; i++) {
            s[i] = (float *)malloc((SPECTR_FFT_SIG_LEN/binning)/* was SPECTR_OUT_SIG_LEN */ * sizeof(float));
        }

        while(retries >= 0) {
            if((ret_val = rp_get_signals(&s, &sig_num, &sig_len, binning)) >= 0) {
                //fprintf(stderr, "Output of %d signals\n", SPECTR_FFT_SIG_LEN/binning);
                for(i = 0; i < SPECTR_FFT_SIG_LEN/binning/*MIN(size, sig_len)*/; i++) {
                    printf("%7d\n", (int)s[channel][i]);
                    //printf("%7d %7d\n", (int)s[1][i], (int)s[2][i]);
                }
                break;
	    }

            if(retries-- == 0) {
                fprintf(stderr, "Signal scquisition was not triggered!\n");
                break;
            }
            usleep(1000);
	}
    }

    if(rp_app_exit() < 0) {
        fprintf(stderr, "rp_app_exit() failed!\n");
        return -1;
    }

    return 0;
}

