///////////////////////////////////////////////////////////////
// SECTION 1: Helper defines, variables, etc.
// Global Variables
// Coherence Protocol Helpers
///////////////////////////////////////////////////////////////

/* Size of the contiguous chunks for scatter/gather */
#ifndef __linux__
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)
#endif

///////////////////////////////////////////////////////////////
// FFT accelerator related defines
///////////////////////////////////////////////////////////////
#if (FFT2_FX_WIDTH == 64)
typedef long long token_t;
typedef double native_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 42
#else // (FFT2_FX_WIDTH == 32)
typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 4
#endif /* FFT2_FX_WIDTH */

const float ERR_TH = 0.05;

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFT_DO_SHIFT_REG 0x40
#define AUDIO_FFT_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFT_PROD_READY_OFFSET 0x50
#define AUDIO_FFT_CONS_VALID_OFFSET 0x54
#define AUDIO_FFT_CONS_READY_OFFSET 0x58
#define AUDIO_FFT_INPUT_OFFSET 0x5C
#define AUDIO_FFT_OUTPUT_OFFSET 0x60

#define AUDIO_FIR_DO_INVERSE_REG 0x48
#define AUDIO_FIR_LOGN_SAMPLES_REG 0x44
#define AUDIO_FIR_DO_SHIFT_REG 0x40
#define AUDIO_FIR_PROD_VALID_OFFSET 0x4C
#define AUDIO_FIR_PROD_READY_OFFSET 0x50
#define AUDIO_FIR_FLT_PROD_VALID_OFFSET 0x54
#define AUDIO_FIR_FLT_PROD_READY_OFFSET 0x58
#define AUDIO_FIR_CONS_VALID_OFFSET 0x5C
#define AUDIO_FIR_CONS_READY_OFFSET 0x60
#define AUDIO_FIR_INPUT_OFFSET 0x64
#define AUDIO_FIR_FLT_INPUT_OFFSET 0x68
#define AUDIO_FIR_TWD_INPUT_OFFSET 0x6C
#define AUDIO_FIR_OUTPUT_OFFSET 0x70

#define SLD_FFT2 0x063
#define DEV_NAME "sld,audio_fft_stratus"

#define SLD_FIR 0x064
#define FIR_DEV_NAME "sld,audio_fir_stratus"

///////////////////////////////////////////////////////////////
// Complex number math for SW implementation
///////////////////////////////////////////////////////////////
typedef struct {
    float r;
    float i;
} cpx_num;

#define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#define C_FIXDIV(c,div) /* NOOP */
#define C_MULBYSCALAR( c, s ) \
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

#define HALF_OF(x) ((x)*.5)

// Size and parameter defines
#define SYNC_VAR_SIZE 10
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define END_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4
#define FLT_VALID_FLAG_OFFSET 6
#define FLT_READY_FLAG_OFFSET 8
#define NUM_DEVICES 3

/* <<--params-->> */
const int32_t logn_samples = LOG_LEN;
const int32_t num_samples = (1 << logn_samples);
const int32_t num_ffts = 1;
const int32_t do_inverse = 0;
const int32_t do_shift = 0;
const int32_t scale_factor = 1;
unsigned len;

unsigned in_words_adj;
unsigned out_words_adj;
unsigned in_len;
unsigned out_len;
unsigned in_size;
unsigned out_size;
unsigned out_offset;
unsigned mem_size;
unsigned acc_len;
unsigned acc_offset;
unsigned acc_size;
unsigned sync_size;

#ifndef __linux__
static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}
#endif

void init_params()
{
	len = num_ffts * (1 << logn_samples);
	// printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, num_ffts, do_inverse, do_shift, len);
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 2 * len;
		out_words_adj = 2 * len;
	} else {
		in_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = 0;
	mem_size = (out_offset * sizeof(token_t)) + out_size + (SYNC_VAR_SIZE * sizeof(token_t));

    acc_size = mem_size;
    acc_len = SYNC_VAR_SIZE + out_len;
    acc_offset = out_offset + acc_len;

	// Total size for all accelerators combined:
	// 0*acc_size - 1*acc_size: FFT Input
	// 1*acc_size - 2*acc_size: FIR Input
	// 2*acc_size - 3*acc_size: IFFT Input
	// 3*acc_size - 4*acc_size: CPU Input
	// 4*acc_size - 5*acc_size: Unused
	// 5*acc_size - 7*acc_size: FIR filters
	// 7*acc_size - 7*acc_size: Twiddle factors
	// Therefore, NUM_DEVICES+5 = 8.
    mem_size *= NUM_DEVICES+5;

    sync_size = SYNC_VAR_SIZE * sizeof(token_t);
}

///////////////////////////////////////////////////////////////
// Acc invocation related variables
///////////////////////////////////////////////////////////////
#ifndef __linux__
int n;
int ndev;
struct esp_device *espdevs;
struct esp_device *fft_dev;
struct esp_device *ifft_dev;
struct esp_device *fir_dev;
unsigned **ptable = NULL;
token_t *mem;
float *gold;
token_t *fxp_filters;
const float ERROR_COUNT_TH = 0.001;
#endif

///////////////////////////////////////////////////////////////
// Helper unions
///////////////////////////////////////////////////////////////
typedef union
{
  struct
  {
    unsigned char r_en   : 1;
    unsigned char r_op   : 1;
    unsigned char r_type : 2;
    unsigned char r_cid  : 4;
    unsigned char w_en   : 1;
    unsigned char w_op   : 1;
    unsigned char w_type : 2;
    unsigned char w_cid  : 4;
	uint16_t reserved: 16;
  };
  uint32_t spandex_reg;
} spandex_config_t;

typedef union
{
  struct
  {
    int32_t value_32_1;
    int32_t value_32_2;
  };
  int64_t value_64;
} spandex_token_t;

///////////////////////////////////////////////////////////////
// Choosing the read, write code and coherence register config
///////////////////////////////////////////////////////////////
#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#if (IS_ESP == 1)
// ESP COHERENCE PROTOCOLS
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config;

#if (COH_MODE == 3)
unsigned coherence = ACC_COH_NONE;
const char print_coh[] = "Non-Coherent DMA";
#elif (COH_MODE == 2)
unsigned coherence = ACC_COH_LLC;
const char print_coh[] = "LLC-Coherent DMA";
#elif (COH_MODE == 1)
unsigned coherence = ACC_COH_RECALL;
const char print_coh[] = "Coherent DMA";
#else
unsigned coherence = ACC_COH_FULL;
const char print_coh[] = "Baseline MESI";
#endif

#else
//SPANDEX COHERENCE PROTOCOLS
unsigned coherence = ACC_COH_FULL;
#if (COH_MODE == 3)
// Owner Prediction
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_op = 1, .w_type = 1};
const char print_coh[] = "Owner Prediction";
#elif (COH_MODE == 2)
// Write-through forwarding
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_type = 1};
const char print_coh[] = "Write-through forwarding";
#elif (COH_MODE == 1)
// Baseline Spandex
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 1};
const char print_coh[] = "Baseline Spandex (ReqV)";
#else
// Fully Coherent MESI
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config;
const char print_coh[] = "Baseline Spandex";
#endif
#endif

///////////////////////////////////////////////////////////////
// SECTION 2: Accelerator helper functions
///////////////////////////////////////////////////////////////
#ifndef __linux__
///////////////////////////////////////////////////////////////
// Probe accelerator devices
///////////////////////////////////////////////////////////////
int probe_fft()
{
	ndev = probe(&espdevs, VENDOR_SLD, SLD_FFT2, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	fft_dev = &espdevs[0];
	ifft_dev = &espdevs[1];
}

int probe_fir()
{
	ndev = probe(&espdevs, VENDOR_SLD, SLD_FIR, FIR_DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", FIR_DEV_NAME);
		return 0;
	}

	fir_dev = &espdevs[0];
}

///////////////////////////////////////////////////////////////
// Start accelerator devices
///////////////////////////////////////////////////////////////
int start_fft(struct esp_device *dev, int inverse)
{
	// printf("**************** %s.%d ****************\n", DEV_NAME, inverse);

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return 0;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return 0;
	}

	#if (COH_MODE == 3 && IS_ESP == 0)
	spandex_config.w_cid = (inverse+3)%4;
	#endif

	iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

	// Pass common configuration parameters
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0);
	iowrite32(dev, DST_OFFSET_REG, 0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG, logn_samples);
	iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG, do_shift);
	iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG, inverse);

	iowrite32(dev, AUDIO_FFT_PROD_VALID_OFFSET, (2 * inverse * acc_len) + VALID_FLAG_OFFSET);
	iowrite32(dev, AUDIO_FFT_PROD_READY_OFFSET, (2 * inverse * acc_len) + READY_FLAG_OFFSET);
	iowrite32(dev, AUDIO_FFT_CONS_VALID_OFFSET, (2 * inverse * acc_len) + acc_len + VALID_FLAG_OFFSET);
	iowrite32(dev, AUDIO_FFT_CONS_READY_OFFSET, (2 * inverse * acc_len) + acc_len + READY_FLAG_OFFSET);
	iowrite32(dev, AUDIO_FFT_INPUT_OFFSET, (2 * inverse * acc_len) + SYNC_VAR_SIZE);
	iowrite32(dev, AUDIO_FFT_OUTPUT_OFFSET, (2 * inverse * acc_len) + acc_len + SYNC_VAR_SIZE);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);
}

int start_fir()
{
	// printf("**************** %s ****************\n", FIR_DEV_NAME);

	// Check DMA capabilities
	if (ioread32(fir_dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return 0;
	}

	if (ioread32(fir_dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return 0;
	}

	#if (COH_MODE == 3 && IS_ESP == 0)
	spandex_config.w_cid = 2;
	#endif

	iowrite32(fir_dev, SPANDEX_REG, spandex_config.spandex_reg);

	// Pass common configuration parameters
	iowrite32(fir_dev, SELECT_REG, ioread32(fir_dev, DEVID_REG));
	iowrite32(fir_dev, COHERENCE_REG, coherence);

	iowrite32(fir_dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(fir_dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(fir_dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(fir_dev, SRC_OFFSET_REG, 0);
	iowrite32(fir_dev, DST_OFFSET_REG, 0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(fir_dev, AUDIO_FIR_LOGN_SAMPLES_REG, logn_samples);

	iowrite32(fir_dev, AUDIO_FIR_PROD_VALID_OFFSET, acc_len + VALID_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_PROD_READY_OFFSET, acc_len + READY_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_FLT_PROD_VALID_OFFSET, acc_len + FLT_VALID_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_FLT_PROD_READY_OFFSET, acc_len + FLT_READY_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_CONS_VALID_OFFSET, (2 * acc_len) + VALID_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_CONS_READY_OFFSET, (2 * acc_len) + READY_FLAG_OFFSET);
	iowrite32(fir_dev, AUDIO_FIR_INPUT_OFFSET, acc_len + SYNC_VAR_SIZE);
	iowrite32(fir_dev, AUDIO_FIR_FLT_INPUT_OFFSET, 5 * acc_len);
	iowrite32(fir_dev, AUDIO_FIR_TWD_INPUT_OFFSET, 7 * acc_len);
	iowrite32(fir_dev, AUDIO_FIR_OUTPUT_OFFSET, (2 * acc_len) + SYNC_VAR_SIZE);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	iowrite32(fir_dev, CMD_REG, CMD_MASK_START);
}

///////////////////////////////////////////////////////////////
// Check for termination of accelerator devices
///////////////////////////////////////////////////////////////
void terminate_fft(struct esp_device *dev)
{
    unsigned done;
    unsigned spin_ct;

	// Wait for completion
	done = 0;
	spin_ct = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
		spin_ct++;
	}

	iowrite32(dev, CMD_REG, 0x0);
}

void terminate_fir()
{
    unsigned done;
    unsigned spin_ct;

	// Wait for completion
	done = 0;
	spin_ct = 0;
	while (!done) {
	    done = ioread32(fir_dev, STATUS_REG);
	    done &= STATUS_MASK_DONE;
	    spin_ct++;
	}

	iowrite32(fir_dev, CMD_REG, 0x0);
}

///////////////////////////////////////////////////////////////
// Perform all for 3 start and terminate
///////////////////////////////////////////////////////////////
void start_acc()
{
	start_fft(fft_dev, 0);
	start_fft(ifft_dev, 1);
	start_fir();
}

void terminate_acc()
{
	terminate_fft(fft_dev);
	terminate_fft(ifft_dev);
	terminate_fir();
}
#endif

///////////////////////////////////////////////////////////////
// SECTION 3: Non-accelerated (i.e., SW) functions to run
// on CPU for FFT, FIR
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// Bare metal math APIs for SW implementation
///////////////////////////////////////////////////////////////
native_t _pow(native_t a, native_t b) {
    native_t c = 1;
    for (int i=0; i<b; i++)
        c *= a;
    return c;
}

native_t _fact(native_t x) {
    native_t ret = 1;
    for (int i=1; i<=x; i++)
        ret *= i;
    return ret;
}

native_t _sin(native_t x) {
    native_t y = x;
    native_t s = -1;
    for (int i=3; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }
    return y;
}

native_t _cos(native_t x) {
    native_t y = 1;
    native_t s = -1;
    for (int i=2; i<=100; i+=2) {
        y+=s*(_pow(x,i)/_fact(i));
        s *= -1;
    }
    return y;
}

native_t _tan(native_t x) {
     return (_sin(x)/_cos(x));
}

///////////////////////////////////////////////////////////////
// Initialize golden data for SW computation
///////////////////////////////////////////////////////////////
void golden_data_init(float *gold, float *gold_ref, float *gold_filter, float *gold_twiddle)
{
	int j;
	const float LO = -1.0;
	const float HI = 1.0;

	for (j = 0; j < 2 * len; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold[j] = LO + scaling_factor * (HI - LO);
		gold_ref[j] = LO + scaling_factor * (HI - LO);
		// uint32_t ig = ((uint32_t*)gold)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * (len+1); j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_filter[j] = LO + scaling_factor * (HI - LO);
		// uint32_t ig = ((uint32_t*)gold_filter)[j];
		// printf("  FLT[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * len; j+=2) {
        // native_t phase = -3.14159265358979323846264338327 * ((native_t) ((j+1) / len) + .5);
        gold_twiddle[j] = -6.42796e-08; // _cos(phase);
        gold_twiddle[j + 1] = -1; // _sin(phase);
		// uint32_t ig = ((uint32_t*)gold_twiddle)[j];
		// printf("  TWD[%u] = 0x%08x\n", j, ig);
		// ig = ((uint32_t*)gold_twiddle)[j+1];
		// printf("  TWD[%u] = 0x%08x\n", j+1, ig);
	} 
}

///////////////////////////////////////////////////////////////
// Perform SW FFT, FIR or IFFT
///////////////////////////////////////////////////////////////
void fft_sw_impl(float *gold)
{
	fft2_comp(gold, num_ffts, num_samples, logn_samples, 0 /* do_inverse */, do_shift);
}

void fir_sw_impl(float *gold, float *gold_filter, float *gold_twiddle, float *gold_freqdata)
{
	int j;

    cpx_num fpnk, fpk, f1k, f2k, tw, tdc;
    cpx_num fk, fnkc, fek, fok, tmp;
    cpx_num cptemp;
    cpx_num inv_twd;
    cpx_num *tmpbuf = (cpx_num *) gold;
    cpx_num *freqdata = (cpx_num *) gold_freqdata;
    cpx_num *super_twiddles = (cpx_num *) gold_twiddle;
    cpx_num *filter = (cpx_num *) gold_filter;

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  1 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }

    // Post-processing
    tdc.r = tmpbuf[0].r;
    tdc.i = tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[len].r = tdc.r - tdc.i;
    freqdata[len].i = freqdata[0].i = 0;

    for ( j=1;j <= len/2 ; ++j ) {
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

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  2 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // FIR
	for (j = 0; j < len + 1; j++) {
        C_MUL( cptemp, freqdata[j], filter[j]);
        freqdata[j] = cptemp;
    }

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  3 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // Pre-processing
    tmpbuf[0].r = freqdata[0].r + freqdata[len].r;
    tmpbuf[0].i = freqdata[0].r - freqdata[len].r;
    C_FIXDIV(tmpbuf[0],2);

    for (j = 1; j <= len/2; ++j) {
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

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  4 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }
}

void ifft_sw_impl(float *gold)
{
	fft2_comp(gold, num_ffts, num_samples, logn_samples, 1 /* do_inverse */, do_shift);
}

///////////////////////////////////////////////////////////////
// Perform all for 3 SW functions (i.e., chain)
///////////////////////////////////////////////////////////////
void chain_sw_impl(float *gold, float *gold_filter, float *gold_twiddle, float *gold_freqdata)
{
    fft_sw_impl(gold);
    fir_sw_impl(gold, gold_filter, gold_twiddle, gold_freqdata);
    ifft_sw_impl(gold);
}