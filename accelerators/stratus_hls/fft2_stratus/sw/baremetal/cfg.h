/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

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
#define FFT2_LOGN_SAMPLES_REG 0x40
#define FFT2_NUM_FFTS_REG 0x44
#define FFT2_DO_INVERSE_REG 0x48
#define FFT2_DO_SHIFT_REG 0x4c
#define FFT2_SCALE_FACTOR_REG 0x50

#define FIR_LOGN_SAMPLES_REG 0x40
#define FIR_NUM_FIRS_REG 0x44
#define FIR_DO_INVERSE_REG 0x48
#define FIR_DO_SHIFT_REG 0x4c
#define FIR_SCALE_FACTOR_REG 0x50

#define SLD_FFT2 0x057
#define DEV_NAME "sld,fft2_stratus"

#define SLD_FIR 0x050
#define FIR_DEV_NAME "sld,fir_stratus"

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
// Complex number math for SW implementation
///////////////////////////////////////////////////////////////
typedef struct {
    float r;
    float i;
} cpx_num;

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#   define C_FIXDIV(c,div) /* NOOP */
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

// Size and parameter defines
#define SYNC_VAR_SIZE 4
#define SPANDEX_CONFIG_VAR_SIZE 4
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
unsigned acc_offset;
unsigned acc_size;
unsigned sync_size;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

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
	mem_size = (out_offset * sizeof(token_t)) + out_size + ((SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE) * sizeof(token_t));

    acc_size = mem_size;
    acc_offset = out_offset + out_len + SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE;
    mem_size *= NUM_DEVICES+5;

    sync_size = SYNC_VAR_SIZE * sizeof(token_t);
	// printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);
}