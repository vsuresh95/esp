// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include <sstream>
#include "system.hpp"

// Helper random generator
static std::uniform_real_distribution<float> *dis;
static std::random_device rd;
static std::mt19937 *gen;

#include "in_data"
#include "out_data"

typedef struct {
    float r;
    float i;
} cpx_num;

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#   define C_FIXDIV(c,div) \
    do{ (c).r /= (div);\
        (c).i /= (div); }while(0)
#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r *= (s);\
        (c).i *= (s); }while(0)

#define  C_ADD( res, a,b)\
    do { \
        (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  C_SUB( res, a,b)\
    do { \
        (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)

#  define HALF_OF(x) ((x)*.5)

static void init_random_distribution(void)
{
    const float LO = -1.5;
    const float HI = 1.5;

    gen = new std::mt19937(rd());
    dis = new std::uniform_real_distribution<float>(LO, HI);
}

static float gen_random_float(void)
{
    return (*dis)(*gen);
}

static float sim_inputs[] = 
{
    -2.22961,
    2.77771,
    2.55493,
    8.75732,
    2.43591,
    -20.6572,
    3.82434,
    10.7007
};

static float sim_filter[] = 
{
    2.2467,
    4.29749,
    -6.35498,
    0.323486,
    1.23901,
    -5.02686,
    8.39172,
    -7.7832,
    3.16711,
    -3.43079
};

static float sim_twd[] = 
{
    -6.29456,
    3.4198,
    8.37463,
    -7.37061,
    -1.78833,
    6.98242,
    -1.80786,
    -9.34631
};

// Process
void system_t::config_proc()
{

    // Reset
    {
        conf_done.write(false);
        conf_info.write(conf_info_t());
        wait();
    }

    ESP_REPORT_INFO("reset done");

    // Config
    load_memory();
    {
        conf_info_t config;
        // Custom configuration
        /* <<--params-->> */
        config.logn_samples = logn_samples;
        config.num_firs = num_firs;
        config.do_inverse = do_inverse;
        config.do_shift = do_shift;
        config.scale_factor = scale_factor;

        wait(); 
        conf_info.write(config);
        conf_done.write(true);
    }

    ESP_REPORT_INFO("config done");

    // Compute
    {
        // Print information about begin time
        sc_time begin_time = sc_time_stamp();
        ESP_REPORT_TIME(begin_time, "BEGIN - fir");

        // Wait the termination of the accelerator
        do { wait(); } while (!acc_done.read());
        debug_info_t debug_code = debug.read();

        // Print information about end time
        sc_time end_time = sc_time_stamp();
        ESP_REPORT_TIME(end_time, "END - fir");

        esc_log_latency(sc_object::basename(), clock_cycle(end_time - begin_time));
        wait(); 
        conf_done.write(false);
    }

    // Validate
    {
        const double ERROR_COUNT_TH = 0.02;
        dump_memory(); // store the output in more suitable data structure if needed
        // check the results with the golden model
        int errs = validate();
        double pct_err = ((double)errs / (double)(2 * num_firs * num_samples));
        if (pct_err > ERROR_COUNT_TH)
        {
            ESP_REPORT_ERROR("DMA %u FX %u nFIR %u logn %u : Exceeding error count threshold : %d of %u = %g vs %g : validation failed!", DMA_WIDTH, FX_WIDTH, num_firs, logn_samples, errs, (2 * num_firs * num_samples), pct_err, ERROR_COUNT_TH);
        } else {
            ESP_REPORT_INFO("DMA %u FX %u nFIR %u logn %u : Not exceeding error count threshold : %d of %u = %g vs %g: validation passed!", DMA_WIDTH, FX_WIDTH, num_firs, logn_samples, errs, (2 * num_firs * num_samples), pct_err, ERROR_COUNT_TH);
        }
    }

    // Conclude
    {
        sc_stop();
    }
}

// Functions
void system_t::load_memory()
{
    // Optional usage check
#ifdef CADENCE
    if (esc_argc() != 1)
    {
        ESP_REPORT_INFO("usage: %s\n", esc_argv()[0]);
        sc_stop();
    }
#endif

    // Input data and golden output (aligned to DMA_WIDTH makes your life easier)
#if (DMA_WORD_PER_BEAT == 0)
    in_words_adj  = 2 * num_firs * num_samples;
    out_words_adj = 2 * num_firs * num_samples;
#else
    in_words_adj  = round_up(2 * num_firs * num_samples, DMA_WORD_PER_BEAT);
    out_words_adj = round_up(2 * num_firs * num_samples, DMA_WORD_PER_BEAT);
#endif

    in_size  = in_words_adj;
    out_size = out_words_adj;
    printf("TEST : MEM : MEM_SIZE = %u : in_size = %u  out_size = %u  gold_size = %u\n", MEM_SIZE, in_size, out_size, out_size);
    if (in_size > MEM_SIZE) {
        ESP_REPORT_INFO("ERROR : Specified in_size is %u and MEM_SIZE is %u -- in_size must be <= MEM_SIZE", in_size, MEM_SIZE);
    }
    if (out_size > MEM_SIZE) {
        ESP_REPORT_INFO("ERROR : Specified out_size is %u and MEM_SIZE is %u -- out_size must be <= MEM_SIZE", out_size, MEM_SIZE);
    }

    init_random_distribution();

    total_size = (7*in_size) + SYNC_VAR_SIZE;

    in = new float[total_size];
    printf("TEST : MEM : in[%d] @ %p  :: in[%d] @ %p\n", 0, &in[0], (in_size-1), &in[in_size-1]);
    printf("IN_DATA:\n");
    printf("float FIR_INPUTS[%u] = {\n", 2 * num_firs * num_samples);
    for (int j = 0; j < 2 * num_firs * num_samples; j++) {
        if (use_input_files) { 
            in[SYNC_VAR_SIZE+j] = sim_inputs[j];
        } else {
            in[SYNC_VAR_SIZE+j] = gen_random_float();
        }
        //in[j] = ((float)j); ///10000.0;
        // printf("    %.15f,\n", in[SYNC_VAR_SIZE+j]);

        // ESP_REPORT_INFO("INPUTS[%u] = %.15f,", SYNC_VAR_SIZE+j, in[SYNC_VAR_SIZE+j]);
    }
    printf("IN_FILTER:\n");
    printf("float FIR_INPUTS[%u] = {\n", 2 * num_firs * num_samples);
    for (int j = 0; j < 2 * num_firs * (num_samples+1); j++) {
        if (use_input_files) { 
            in[4 * in_size + j] = sim_filter[j];
        } else {
            in[4 * in_size + j] = gen_random_float();
        }
        //in[j] = ((float)j); ///10000.0;
        // printf("    %.15f,\n", in[4 * in_size + j]);

        // ESP_REPORT_INFO("FILTERS[%u] = %.15f,", 4 * in_size + j, in[4 * in_size + j]);
    }
    printf("IN_TWIDDLE:\n");
    printf("float FIR_INPUTS[%u] = {\n", 2 * num_firs * num_samples);
    for (int j = 0; j < 2 * num_firs * num_samples; j+=2) {
        double phase = -3.14159 * ((double) (j+1) / num_firs * num_samples+ .5);
        in[6 * in_size + j] = cos(phase);
        in[6 * in_size + j + 1] = sin(phase);
        // in[6 * in_size + j] = sim_twd[j];
        //in[j] = ((float)j); ///10000.0;
        // printf("    %.15f,\n", in[6 * in_size + j]);
        // printf("    %.15f,\n", in[6 * in_size + j + 1]);

        // ESP_REPORT_INFO("TWD[%u] = %.15f,", 6 * in_size + j, in[6 * in_size + j]);
        // ESP_REPORT_INFO("TWD[%u] = %.15f,", 6 * in_size + j + 1, in[6 * in_size + j + 1]);
    }
    printf("};\nEND_OF_IN_DATA\n");

    // Compute golden output
    gold = new float[out_size+2];
    float* gold_freqdata = new float[out_size+2];
    memcpy(gold, in+SYNC_VAR_SIZE, out_size * sizeof(float));

    // fir_comp(gold, num_firs, num_samples, logn_samples, do_inverse, do_shift); // do_bitrev is always true

    cpx_num fpnk, fpk, f1k, f2k, tw, tdc;
    cpx_num fk, fnkc, fek, fok, tmp;
    cpx_num cptemp;
    cpx_num inv_twd;
    cpx_num *tmpbuf = (cpx_num *) gold;
    cpx_num *freqdata = (cpx_num *) gold_freqdata;
    cpx_num *super_twiddles = (cpx_num *) (in + 6 * in_size);
    cpx_num *filter = (cpx_num *) (in + 4 * in_size);
    const unsigned len = num_samples;

	// for (int j = 0; j < 2 * len; j++) {
	// 	printf("  1 GOLD[%u] = %f\n", j, gold[j]);
    // }

    // Post-processing
    tdc.r = tmpbuf[0].r;
    tdc.i = tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[len].r = tdc.r - tdc.i;
    freqdata[len].i = freqdata[0].i = 0;

    for (int j=1;j <= len/2 ; ++j ) {
        fpk    = tmpbuf[j];
        fpnk.r =   tmpbuf[len-j].r;
        fpnk.i = - tmpbuf[len-j].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , super_twiddles[j-1]);

        freqdata[j].r = HALF_OF(f1k.r + tw.r);
        freqdata[j].i = HALF_OF(f1k.i + tw.i);
        freqdata[len-j].r = HALF_OF(f1k.r - tw.r);
        freqdata[len-j].i = HALF_OF(tw.i - f1k.i);
    }

	// for (int j = 0; j < 2 * (len+1); j++) {
	// 	printf("  2 GOLD[%u] = %f\n", j, gold_freqdata[j]);
    // }

    // FIR
	for (int j = 0; j < len+1; j++) {
        C_MUL( cptemp, freqdata[j], filter[j]);
        freqdata[j] = cptemp;
    }

	// for (int j = 0; j < 2 * (len+1); j++) {
	// 	printf("  3 GOLD[%u] = %f\n", j, gold_freqdata[j]);
    // }

    // Pre-processing
    tmpbuf[0].r = freqdata[0].r + freqdata[len].r;
    tmpbuf[0].i = freqdata[0].r - freqdata[len].r;
    C_FIXDIV(tmpbuf[0],2);

    for (int j = 1; j <= len/2; ++j) {
        fk = freqdata[j];
        fnkc.r = freqdata[len-j].r;
        fnkc.i = -freqdata[len-j].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );
        inv_twd = super_twiddles[j-1];
        C_MULBYSCALAR(inv_twd,-1);

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, inv_twd);
        C_ADD (tmpbuf[j],     fek, fok);
        C_SUB (tmpbuf[len-j], fek, fok);
        tmpbuf[len-j].i *= -1;
    }

	// for (int j = 0; j < 2 * len; j++) {
	// 	printf("  4 GOLD[%u] = %f\n", j, gold[j]);
    // }

    // printf("OUT_DATA:\n");
    // printf("float FIR_OUTPUTS[%u] = {\n", 2 * num_firs * num_samples);
    // for (int j = 0; j < 2 * num_firs * num_samples; j++) {
    //     printf("    %.15f,\n", gold[j]);
    // }
    // printf("};\nEND_OF_OUT_DATA\n");

    // Memory initialization:
#if (DMA_WORD_PER_BEAT == 0)
    for (int i = 0; i < total_size-SYNC_VAR_SIZE; i++)  {
        sc_dt::sc_bv<DATA_WIDTH> data_bv(in[i]);
        for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
            mem[DMA_BEAT_PER_WORD * i + j] = data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH);
    }
#else
    for (int i = 0; i < total_size/ DMA_WORD_PER_BEAT; i++)  {
        sc_dt::sc_bv<DMA_WIDTH> data_bv;
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++) {
            data_bv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = fp2bv<FPDATA, WORD_SIZE>(FPDATA(in[i * DMA_WORD_PER_BEAT + j]));
            // ESP_REPORT_INFO("    in[%u + %u] %.15f,", i, j, in[i * DMA_WORD_PER_BEAT + j]);
        }
        mem[i] = data_bv;
    }
#endif
    mem[0] = 1;

    ESP_REPORT_INFO("load memory completed");
}

void system_t::dump_memory()
{
    // Get results from memory
    out = new float[out_size];
    uint32_t offset = in_size + 2*SYNC_VAR_SIZE;

    // printf("float FIR_OUTPUTS[%u] = {\n", 2 * num_firs * num_samples);
#if (DMA_WORD_PER_BEAT == 0)
    offset = offset * DMA_BEAT_PER_WORD;
    for (int i = 0; i < out_size; i++)  {
        sc_dt::sc_bv<DATA_WIDTH> data_bv;

        for (int j = 0; j < DMA_BEAT_PER_WORD; j++)
            data_bv.range((j + 1) * DMA_WIDTH - 1, j * DMA_WIDTH) = mem[offset + DMA_BEAT_PER_WORD * i + j];

        FPDATA out_fx = bv2fp<FPDATA, WORD_SIZE>(data_bv);
        out[i] = (float) out_fx;
        // printf("    %.15f,\n", out[i]);
    }
#else
    offset = offset / DMA_WORD_PER_BEAT;
    for (int i = 0; i < out_size / DMA_WORD_PER_BEAT; i++)
        for (int j = 0; j < DMA_WORD_PER_BEAT; j++) {
            FPDATA out_fx = bv2fp<FPDATA, WORD_SIZE>(mem[offset + i].range((j + 1) * DATA_WIDTH - 1, j * DATA_WIDTH));
            out[i * DMA_WORD_PER_BEAT + j] = (float) out_fx;
            // printf("    %.15f,\n", out[i * DMA_WORD_PER_BEAT + j]);
        }
#endif

    ESP_REPORT_INFO("dump memory completed");
}

int system_t::validate()
{
    // Check for mismatches
    uint32_t errors1 = 0;
    uint32_t errors2 = 0;
    uint32_t errors3 = 0;
    const float ERR_TH = 0.05;
    const float minV = 1.0/(float)(1<<14);
    printf("minV = %.15f\n", minV);
    for (int nf = 0; nf < num_firs; nf++ ) {
        for (int j = 0; j < 2 * num_samples; j++) {
            int idx = 2 * nf * num_samples + j;
            float eOvG = (fabs(gold[idx] - out[idx] ) / fabs(gold[idx]));
            float eGvO = (fabs(out[idx]  - gold[idx]) / fabs(out[idx]));
            ESP_REPORT_INFO(" idx %u : gold %.15f  out %.15f", idx, gold[idx], out[idx]);
            //if ((fabs(gold[idx] - out[idx]) / fabs(gold[idx])) > ERR_TH) {
            if ((eOvG > ERR_TH) && (eGvO > ERR_TH)) {
                if (errors1 < 4) {
                    //ESP_REPORT_INFO(" ERROR : GOLD[%u] = %f vs %f = out[%u]\n", idx, gold[idx], out[idx], idx);
                    ESP_REPORT_INFO(" ERROR %u : fir %u : idx %u : gold %.15f  out %.15f", errors1, nf, idx, gold[idx], out[idx]);
                }
                errors1++;
                if (fabs(gold[j]) > minV) {
                    if (errors2 < 4) {
                        //ESP_REPORT_INFO(" ERROR : GOLD[%u] = %f vs %f = out[%u]\n", idx, gold[idx], out[idx], idx);
                        ESP_REPORT_INFO(" ERROR %u : fir %u : idx %u : gold %.15f  out %.15f", errors2, nf, idx, gold[idx], out[idx]);
                    }
                    errors2++;
                }
                if (fabs(out[j]) > minV) {
                    if (errors3 < 4) {
                        //ESP_REPORT_INFO(" ERROR : GOLD[%u] = %f vs %f = out[%u]\n", idx, gold[idx], out[idx], idx);
                        ESP_REPORT_INFO(" ERROR %u : fir %u : idx %u : gold %.15f  out %.15f", errors3, nf, idx, gold[idx], out[idx]);
                    }
                    errors3++;
                }
            }
            /*
            if ((fabs(FIR_OUTPUTS[idx] - out[idx]) / fabs(FIR_OUTPUTS[idx])) > ERR_TH) {
                if (errors2 < 4) {
                    //ESP_REPORT_INFO(" ERROR : FIR_OUTPUTS[%u] = %f vs %f = out[%u]\n", idx, FIR_OUTPUTS[idx], out[idx], idx);
                    ESP_REPORT_INFO(" ERROR %u : fir %u : idx %u : FIR_OUTPUTS %.15f  out %.15f", errors2, nf, idx, FIR_OUTPUTS[idx], out[idx]);
                }
                errors2++;
            }
            if ((fabs(FIR_OUTPUTS[idx] - gold[idx]) / fabs(FIR_OUTPUTS[idx])) > ERR_TH) {
                if (errors3 < 4) {
                    //ESP_REPORT_INFO(" ERROR : FIR_OUTPUTS[%u] = %f vs %f = gold[%u]\n", idx, FIR_OUTPUTS[idx], gold[idx], idx);
                    ESP_REPORT_INFO(" ERROR %u : fir %u : idx %u : FIR_OUTPUTS %.15f  GOLD %.15f", errors3, nf, idx, FIR_OUTPUTS[idx], gold[idx]);
                }
                errors3++;
            }
            */
        }
    }

    ESP_REPORT_INFO("DMA %u FX %u nFIR %u logn %u : Rel-Err > %f for %d OvG %d Gmin %d Omin values out of %d\n", 
                    DMA_WIDTH, FX_WIDTH, num_firs,logn_samples,  ERR_TH, errors1, errors2, errors3, 2*num_firs*num_samples);

    delete [] in;
    delete [] out;
    delete [] gold;

    return errors1;
}
