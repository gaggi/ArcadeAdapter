#ifndef uchar
#define uchar   unsigned char
#endif

typedef struct {
	char	reportid;	// 0
	uchar	b1;			// 6
	uchar	b2;			// 7
	char	x;			// 1
	char	y;			// 2
} report_t;