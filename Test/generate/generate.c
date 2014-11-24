/**
 * $Id: generate.c 882 2013-12-16 12:46:01Z crt.valentincic $
 *
 * @brief Red Pitaya simple signal/function generator with pre-defined
 *        signal types.
 *
 * @Author Ales Bardorfer <ales.bardorfer@redpitaya.com>
 * @Author Marko Meza <marko.meza@gmail.com>
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 *
 * For all functions from standard C library it is easy to see the description
 * using the manuals with 'man' command in GNU/Linux console. For example, to see 
 * the description, usage and examples of function 'cos()' use command:
 * 'man cos'
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fpga_awg.h"
#include "calib.h"

/**
 * GENERAL DESCRIPTION:
 *
 * The code below performs a function of a signal generator, which produces
 * a a signal of user-selectable pred-defined Signal shape
 * [Sine, Square, Triangle], Amplitude and Frequency on a selected Channel:
 *
 *
 *                   /-----\
 *   Signal shape -->|     | -->[data]--+-->[FPGA buf 1]--><DAC 1>
 *   Amplitude ----->| AWG |            |
 *   Frequency ----->|     |             -->[FPGA buf 2]--><DAC 2>
 *                   \-----/            ^
 *                                      |
 *   Channel ---------------------------+ 
 *
 *
 * This is achieved by first parsing the four parameters defining the 
 * signal properties from the command line, followed by synthesizing the 
 * signal in data[] buffer @ 125 MHz sample rate within the
 * generate_signal() function, depending on the Signal shape, Amplitude
 * and Frequency parameters. The data[] buffer is then transferred
 * to the specific FPGA buffer, defined by the Channel parameter -
 * within the write_signal_fpga() function.
 * The FPGA logic repeatably sends the data from both FPGA buffers to the
 * corresponding DACs @ 125 MHz, which in turn produces the synthesized
 * signal on Red Pitaya SMA output connectors labeled DAC1 & DAC2.
 *
 */

/** Maximal signal frequency [Hz] */
const double c_max_frequency = 10e6;

/** Maximal signal amplitude [Vpp] */
const double c_max_amplitude = 2.0;

/** Maximal Signal Voltage on DAC outputs on channel A. It is expressed in [V],
 * and calculated from apparent Back End Full Scale calibration parameter.
 */
float ch1_max_dac_v;

/** Maximal Signal Voltage on DAC outputs on channel B. It is expressed in [V],
 * and calculated from apparent Back End Full Scale calibration parameter.
 */
float ch2_max_dac_v;

/** DAC number of bits */
const int c_awg_fpga_dac_bits = 14;

/** AWG buffer length [samples]*/
#define n (16*1024)

/** AWG data buffer */
int32_t data[n];

/** Program name */
const char *g_argv0 = NULL;

rp_calib_params_t rp_calib_params;
/** Pointer to externally defined calibration parameters. */
rp_calib_params_t *gen_calib_params = NULL;

/** Signal types */
typedef enum {
    eSignalSine,         ///< Sinusoidal waveform.
    eSignalSquare,       ///< Square waveform.
    eSignalTriangle      ///< Triangular waveform.
} awg_signal_t;

/** AWG FPGA parameters */
typedef struct {
    int32_t  offsgain;   ///< AWG offset & gain.
    uint32_t wrap;       ///< AWG buffer wrap value.
    uint32_t step;       ///< AWG step interval.
} awg_param_t;

/* Forward declarations */
void synthesize_signal(float ampl, float freq, int calib_dc_offs, int calib_fs,
                       float max_dac_v, float user_dc_offs, awg_signal_t type, 
                       int32_t *data, awg_param_t *awg);
void write_data_fpga(uint32_t ch,
                     const int32_t *data,
                     const awg_param_t *awg);

/** Print usage information */
void usage() {

    const char *format =
        "\n"
        "version: Marko-edgo 2014-10-28 (beta)\n"
        "Usage: %s   channel amplitude frequency <type> <dc-offset>\n"
        "\n"
        "\tchannel     Channel to generate signal on [1, 2].\n"
        "\tamplitude   Peak-to-peak signal amplitude in Vpp [0.0 - %1.1f].\n"
        "\tfrequency   Signal frequency in Hz [0 - %2.1e].\n"
        "\ttype        Signal type [sine, sqr, tri].\n"
        "\tdc-offset   DC Offset (Warning: max signal swing is 2.0 Vpp,\n"
        "\t            one must be careful not to exceed it with offset)\n"
        "\n";

    fprintf( stderr, format, g_argv0, c_max_amplitude, c_max_frequency);
}


/** Signal generator main */
int main(int argc, char *argv[])
{
    g_argv0 = argv[0];    

    if ( argc < 4 ) {

        usage();
        return -1;
    }

    /* Channel argument parsing */
    uint32_t ch = atoi(argv[1]) - 1; /* Zero based internally */
    if (ch > 1) {
        fprintf(stderr, "Invalid channel: %s\n", argv[1]);
        usage();
        return -1;
    }

    /* Signal amplitude argument parsing */
    float ampl = strtod(argv[2], NULL);
    if ( (ampl < 0.0) || (ampl > c_max_amplitude) ) {
        fprintf(stderr, "Invalid amplitude: %s\n", argv[2]);
        usage();
        return -1;
    }

    /* Signal frequency argument parsing */
    float freq = strtod(argv[3], NULL);
    if ( (freq < 0.0) || (freq > c_max_frequency ) ) {
        fprintf(stderr, "Invalid frequency: %s\n", argv[3]);
        usage();
        return -1;
    }

    /* Signal type argument parsing */
    awg_signal_t type = eSignalSine;
    if (argc > 4) {
        if ( strcmp(argv[4], "sine") == 0) {
            type = eSignalSine;
        } else if ( strcmp(argv[4], "sqr") == 0) {
            type = eSignalSquare;
        } else if ( strcmp(argv[4], "tri") == 0) {
            type = eSignalTriangle;
        } else {
            fprintf(stderr, "Invalid signal type: %s\n", argv[4]);
            usage();
            return -1;
        }
    }

    //Added by Marko
    float offsetVolts = 0.0;
    if(argc > 5){
        offsetVolts = strtod(argv[5], NULL);
        //fprintf(stderr, "Using offset: %f\n", offsetVolts);
    } 

    rp_default_calib_params(&rp_calib_params);
    gen_calib_params = &rp_calib_params;
    if(rp_read_calib_params(gen_calib_params) < 0) {
        fprintf(stderr, "rp_read_calib_params() failed, using default"
            " parameters\n");
    }
    ch1_max_dac_v = fpga_awg_calc_dac_max_v(gen_calib_params->be_ch1_fs);
    ch2_max_dac_v = fpga_awg_calc_dac_max_v(gen_calib_params->be_ch2_fs);

    awg_param_t params;
    /* Prepare data buffer (calculate from input arguments) */
    synthesize_signal(ampl,//Amplitude
                      freq,//Frequency
                      (ch == 0) ? gen_calib_params->be_ch1_dc_offs : gen_calib_params->be_ch2_dc_offs,//Calibrated Instrument DC offset
                      (ch == 0) ? gen_calib_params->be_ch1_fs : gen_calib_params->be_ch2_fs,//Calibrated Back-End full scale coefficient
                      (ch == 0) ? ch1_max_dac_v : ch2_max_dac_v,//Maximum DAC voltage
                      offsetVolts,//DC offset voltage
                      type,//Signal type/shape
                      data,//Returned synthesized AWG data vector
                      &params);//Returned AWG parameters

    /* Write the data to the FPGA and set FPGA AWG state machine */
    write_data_fpga(ch, data, &params);
}

/*----------------------------------------------------------------------------------*/
/**
 * Synthesize a desired signal.
 *
 * Generates/synthesizes a signal, based on three predefined signal
 * types/shapes, signal amplitude & frequency. The data[] vector of 
 * samples at 125 MHz is generated to be re-played by the FPGA AWG module.
 *
 * @param[in]  ampl           Signal amplitude [Vpp].
 * @param[in]  freq           Signal frequency [Hz].
 * @param[in]  calib_dc_offs  Calibrated Instrument DC offset
 * @param[in]  max_dac_v      Maximum DAC voltage in [V]
 * @param[in]  user_dc_offs   User defined DC offset
 * @param[in]  type           Signal type/shape [Sine, Square, Triangle].
 * @param[in]  data           Returned synthesized AWG data vector.
 * @param[out] awg            Returned AWG parameters.
 */
void synthesize_signal(float ampl, float freq, int calib_dc_offs, int calib_fs,
                       float max_dac_v, float user_dc_offs, awg_signal_t type, 
                       int32_t *data, awg_param_t *awg) 
{
    uint32_t i;

    /* Various locally used constants - HW specific parameters */
    const int trans0 = 30;
    const int trans1 = 300;
    const float tt2 = 0.249;
    const int c_dac_max =  (1 << (c_awg_fpga_dac_bits - 1)) - 1;
    const int c_dac_min = -(1 << (c_awg_fpga_dac_bits - 1));

    int trans = round(freq / 1e6 * ((float) trans1)); /* 300 samples at 1 MHz */
    int user_dc_off_cnt = 
        round((1<<(c_awg_fpga_dac_bits-1)) * user_dc_offs / max_dac_v);
    uint32_t amp; 

    /* Saturate offset - depending on calibration offset, it could overflow */
    int offsgain = calib_dc_offs + user_dc_off_cnt;
    offsgain = (offsgain > c_dac_max) ? c_dac_max : offsgain;
    offsgain = (offsgain < c_dac_min) ? c_dac_min : offsgain;

    awg->offsgain = (offsgain << 16) | 0x2000;
    awg->step = round(65536.0 * freq/c_awg_smpl_freq * ((float) AWG_SIG_LEN));
    awg->wrap = round(65536 * (AWG_SIG_LEN - 1));
    
    //= (ampl) * (1<<(c_awg_fpga_dac_bits-2));
    //fpga_awg_calc_dac_max_v(calib_fs)
    
    amp= round(ampl/2/fpga_awg_calc_dac_max_v(calib_fs)* c_dac_max );
    
    /* Truncate to max value */
    amp = (amp > c_dac_max) ? c_dac_max : amp;

    if (trans <= 10) {
        trans = trans0;
    }

    /* Fill data[] with appropriate buffer samples */
    for(i = 0; i < AWG_SIG_LEN; i++) {
        /* Sine */
        if (type == eSignalSine) {
            data[i] = round(amp * cos(2*M_PI*(float)i/(float)AWG_SIG_LEN));
        }
 
        /* Square */
        if (type == eSignalSquare) {
            data[i] = round(amp * cos(2*M_PI*(float)i/(float)AWG_SIG_LEN));
            data[i] = (data[i] > 0) ? amp : -amp;

            /* Soft linear transitions */
            float mm, qq, xx, xm;
            float x1, x2, y1, y2;    

            xx = i;
            xm = AWG_SIG_LEN;
            mm = -2.0*(float)amp/(float)trans; 
            qq = (float)amp * (2 + xm/(2.0*(float)trans));

            x1 = xm * tt2;
            x2 = xm * tt2 + (float)trans;

            if ( (xx > x1) && (xx <= x2) ) {

                y1 = (float)amp;
                y2 = -(float)amp;

                mm = (y2 - y1) / (x2 - x1);
                qq = y1 - mm * x1;

                data[i] = round(mm * xx + qq); 
            }

            x1 = xm * 0.75;
            x2 = xm * 0.75 + trans;

            if ( (xx > x1) && (xx <= x2)) {  

                y1 = -(float)amp;
                y2 = (float)amp;

                mm = (y2 - y1) / (x2 - x1);
                qq = y1 - mm * x1;

                data[i] = round(mm * xx + qq); 
            }
        }

        /* Triangle */
        if (type == eSignalTriangle) {
            data[i] = round(-1.0 * (float)amp *
                     (acos(cos(2*M_PI*(float)i/(float)AWG_SIG_LEN))/M_PI*2-1));
        }
    }
}

/**
 * Write synthesized data[] to FPGA buffer.
 *
 * @param ch    Channel number [0, 1].
 * @param data  AWG data to write to FPGA.
 * @param awg   AWG paramters to write to FPGA.
 */
void write_data_fpga(uint32_t ch,
                     const int32_t *data,
                     const awg_param_t *awg) {

    uint32_t i;

    fpga_awg_init();

    if(ch == 0) {
        /* Channel A */
        g_awg_reg->state_machine_conf = 0x000041;
        g_awg_reg->cha_scale_off      = awg->offsgain;
        g_awg_reg->cha_count_wrap     = awg->wrap;
        g_awg_reg->cha_count_step     = awg->step;
        g_awg_reg->cha_start_off      = 0;

        for(i = 0; i < n; i++) {
            g_awg_cha_mem[i] = data[i];
        }
    } else {
        /* Channel B */
        g_awg_reg->state_machine_conf = 0x410000;
        g_awg_reg->chb_scale_off      = awg->offsgain;
        g_awg_reg->chb_count_wrap     = awg->wrap;
        g_awg_reg->chb_count_step     = awg->step;
        g_awg_reg->chb_start_off      = 0;

        for(i = 0; i < n; i++) {
            g_awg_chb_mem[i] = data[i];
        }
    }

    /* Enable both channels */
    /* TODO: Should this only happen for the specified channel?
     *       Otherwise, the not-to-be-affected channel is restarted as well
     *       causing unwanted disturbances on that channel.
     */
    g_awg_reg->state_machine_conf = 0x110011;

    fpga_awg_exit();
}
