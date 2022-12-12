/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

// typedef float token_t;
typedef int32_t token_t;

/* User defined registers */
/* <<--regs-->> */
#define FUSION_UNOPT_VENO_REG 0x54
#define FUSION_UNOPT_IMGWIDTH_REG 0x50
#define FUSION_UNOPT_HTDIM_REG 0x4c
#define FUSION_UNOPT_IMGHEIGHT_REG 0x48
#define FUSION_UNOPT_SDF_BLOCK_SIZE_REG 0x44
#define FUSION_UNOPT_SDF_BLOCK_SIZE3_REG 0x40

#define SLD_FUSION_UNOPT 0x167
#define DEV_NAME "sld,fusion_unopt_stratus"

/* <<--params-->> */
#define VENO 10
#define IMGWIDTH 640
#define IMGHEIGHT 480
#define HTDIM 512
#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_SIZE3 512
// const int32_t imgwidth = 640;
// const int32_t imgheight = 480;
// const int32_t veno = 10;
// const int32_t htdim = 512;
// const int32_t sdf_block_size = 8;
// const int32_t sdf_block_size3 = 512;

const int32_t imgwidth = 160;
const int32_t imgheight = 120;
const int32_t veno = 8;
const int32_t htdim = 64;
const int32_t sdf_block_size = 4;
const int32_t sdf_block_size3 = 64;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

void init_params() {
    if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 25 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5 + veno + 1;
		out_words_adj = veno * sdf_block_size * sdf_block_size * sdf_block_size * 3;
	} else {
		in_words_adj = round_up(25+imgwidth*imgheight+sdf_block_size3 * 2+htdim * 5+veno + 1, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(veno * sdf_block_size * sdf_block_size * sdf_block_size * 3, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len = out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;
	// printf("mem_size: %u\n", mem_size);
}
