#ifndef _IKB_H_
#define _IKB_H_

#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "logger.h"
#include "status.h"

#define MAX_IKB_SEQ_SIZE 30
#define BITS_IN_BYTE 8
#define ALPHABET_LEN 256  /* ASCII chars */


#define BIN8_FORMAT "%d%d%d%d%d%d%d%d"
#define BIN16_FORMAT BIN8_FORMAT " " BIN8_FORMAT
#define BIN32_FORMAT BIN16_FORMAT "  " BIN16_FORMAT

#define BIN_BIT(number, bit) ((number & (1 << bit)) ? 1 : 0)
#define BIN8_NUMBER(number) \
        BIN_BIT(number, 7), \
        BIN_BIT(number, 6), \
        BIN_BIT(number, 5), \
        BIN_BIT(number, 4), \
        BIN_BIT(number, 3), \
        BIN_BIT(number, 2), \
        BIN_BIT(number, 1), \
        BIN_BIT(number, 0)

#define BIN16_NUMBER(number) \
        BIN8_NUMBER(((number >> 8) & 0xff)), \
        BIN8_NUMBER((number & 0xff))

#define BIN32_NUMBER(number) \
        BIN16_NUMBER(((number >> 16) & 0xffff)), \
        BIN16_NUMBER((number & 0xffff))

typedef struct ikb_enc_table {
    uint8_t   codes_num;       /* Кількість кодів у таблиці */
    uint8_t   encode_ability;  /* Кількість бітів, що може бути закодована ІКВ */
    uint8_t   code_len_bits;   /* Довжина кодів у бітах */
    uint8_t   code_len_bytes;  /* Розмір буфера коду у байтах */
    uint8_t **codes;           /* Коди у вигляді масивів чисел uint8_t */
} ikb_enc_table_t;

typedef struct ikb {
    uint8_t         degree;
    uint8_t         power;
    uint8_t         seq_len;
    uint8_t         sequence[MAX_IKB_SEQ_SIZE];
    uint8_t         seq_str_len;
    char           *seq_str;
    uint8_t         sum;
    uint8_t         err_found;
    uint8_t         err_fixed;
    uint8_t         min_code_dist;
    ikb_enc_table_t enc_table;
} ikb_t;


status_code_t ikb_init(uint8_t seq_len, const uint8_t *seq_p, ikb_t *ikb_p);
status_code_t ikb_deinit(ikb_t *ikb_p);

status_code_t ikb_encode(const ikb_t *ikb_p,
                         uint8_t msg_size, gchararray message,
                         uint32_t *code_size_p, gchararray code_seq);
status_code_t ikb_decode(const ikb_t *ikb_p,
                         uint32_t code_size, gchararray code_seq,
                         uint8_t *msg_size_p, gchararray message);

status_code_t ikb_code_to_str(uint32_t code_size, gchararray code_seq,
                              uint32_t *msg_size, gchararray message);

status_code_t ikb_seq_get_str_size(const ikb_t *ikb_p, uint8_t *buf_size);

status_code_t ikb_seq_to_str(const ikb_t *ikb_p, uint8_t buf_size, gchararray buffer);



#endif /* _IKB_H_ */