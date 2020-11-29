#include <stdlib.h>
#include <time.h>

#include "ikb.h"


/* [function] bits_encode_ability()
 * ----------------------------------------------------------------------------
 * Calculate number of bits that can be encoded by specific IKB
 * at one time. Returns bits_num as a result.
 *
 * ----------------------------------------------------------------------------
 * Link:
 * https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * ----------------------------------------------------------------------------
 */
status_code_t bits_encode_ability(ikb_t *ikb_p, uint8_t *bits_num)
{
    status_code_t status = SC_OK;
    int32_t       num = (int)ikb_p->sum;
    uint8_t       bit_counter = 0;

    while (num != 0) {
        bit_counter++;
        num = num >> 1;
    }

    *bits_num = bit_counter - 1;
    return status;
}


/* [function] ikb_init
 * ----------------------------------------------------------------------------
 * Used to initialize Ideal Key Bundle before the usage.
 *
 * ----------------------------------------------------------------------------
 */
status_code_t ikb_init(uint8_t seq_len, const uint8_t *seq_p, ikb_t *ikb_p)
{
    status_code_t status = SC_OK;
    uint8_t       i = 0, j = 0, k = 0;
    uint8_t       k_byte = 0, k_bit = 0;

    /* Перевірка вхідних параметрів */
    if (seq_len < 1 || seq_len > 30) {
        LOG_ERR("Invalid sequence size requested - %d", seq_len);
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    } else if (seq_p == NULL) {
        MSG_ERR("Sequence pointer not initialized");
        status = SC_PARAM_NULL;
        goto exit;
    } else if (ikb_p == NULL) {
        MSG_ERR("IKB pointer not initialized");
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Очищуємо пам'ять ІКВ та копіюємо дані про вхідну послідовність */
    memset(ikb_p, 0, sizeof(ikb_t));
    ikb_p->seq_len = seq_len;
    memcpy(ikb_p->sequence, seq_p, seq_len);

    /* Обчислюємо розмір буфера для стрічки */
    status = ikb_seq_get_str_size(ikb_p, &ikb_p->seq_str_len);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка при обчисленні розміру стрічки: %s", status_get_str(status));
        goto exit;
    }

    /* Виділяємо буфер і конвертуємо стрічку */
    ikb_p->seq_str = (char *)calloc(ikb_p->seq_str_len, sizeof(char));
    status = ikb_seq_to_str(ikb_p, ikb_p->seq_str_len, ikb_p->seq_str);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка при формуванні стрічки: %s", status_get_str(status));
        goto exit;
    }

    g_printf("Initialization of IKB: %s\n", ikb_p->seq_str);

    /* Розрахуємо порядок і кратність ІКВ */
    ikb_p->degree = seq_len;
    ikb_p->power = 0;
    for (i = 0; i < seq_len; i++) {
        if (seq_p[i] == 1) {
            ikb_p->power++;
        }
    }

    /* Розрахуємо похідні параметри ІКВ */
    ikb_p->sum = (ikb_p->degree * (ikb_p->degree - 1) / ikb_p->power) + 1;
    ikb_p->err_found = 2 * (ikb_p->degree - ikb_p->power) - 1;
    ikb_p->err_fixed = ikb_p->degree - ikb_p->power - 1;
    ikb_p->min_code_dist = ikb_p->sum - 2 * (ikb_p->degree - ikb_p->power);

    /* Згенеруємо таблицю кодування */
    ikb_p->enc_table.codes_num = ikb_p->sum;
    ikb_p->enc_table.code_len_bits = ikb_p->sum;
    ikb_p->enc_table.code_len_bytes = ikb_p->sum / BITS_IN_BYTE + 1;
    ikb_p->enc_table.codes = (uint8_t **)calloc(ikb_p->enc_table.codes_num, sizeof(uint8_t *));

    status = bits_encode_ability(ikb_p, &ikb_p->enc_table.encode_ability);
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Failed to define bits encode ability of IKB.\n");
        goto exit;
    }

    g_printf("Encoding table (codes: %d; chip: %db): \n", ikb_p->enc_table.codes_num, ikb_p->enc_table.code_len_bits);
    for (i = 0; i < ikb_p->enc_table.codes_num; i++) {
        /* Виділення масиву для коду */
        ikb_p->enc_table.codes[i] = (uint8_t *)calloc(ikb_p->enc_table.code_len_bytes, sizeof(uint8_t));

        /* Ініціалізація бітового вектора коду */
        for (k = i, j = 0; j < ikb_p->seq_len; k += ikb_p->sequence[j], j++) {
            /* Перенесення вказівника на початок у разі виходу за кінець масиву */
            k = k % ikb_p->enc_table.code_len_bits;

            /* Обчислюємо адресу байта і біта */
            k_byte = k / BITS_IN_BYTE;
            k_bit = k - k_byte * BITS_IN_BYTE;

            /* Встановлюємо заданий біт у значення 1 */
            ikb_p->enc_table.codes[i][k_byte] |= 0x80 >> k_bit;
        }

        /* Виведемо згенерований код */
        for (j = 0; j < ikb_p->enc_table.code_len_bytes; j++) {
            g_printf(" " BIN8_FORMAT, BIN8_NUMBER(ikb_p->enc_table.codes[i][j]));
        }
        g_printf("\n");
    }

exit:
    return status;
}


/* [function] ikb_deinit
 * ----------------------------------------------------------------------------
 * Used to de-initialize Ideal Key Bundle after the usage.
 *
 * ----------------------------------------------------------------------------
 */
status_code_t ikb_deinit(ikb_t *ikb_p)
{
    status_code_t status = SC_OK;
    uint8_t       i = 0;

    if (ikb_p == NULL) {
        MSG_ERR("IKB pointer not initialized");
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Видаляємо коди з таблиці кодування */
    for (i = 0; i < ikb_p->enc_table.codes_num; i++) {
        /* Звільняємо масив байтів коду */
        free(ikb_p->enc_table.codes[i]);
    }

    /* Звільнимо таблицю кодів */
    free(ikb_p->enc_table.codes);

    memset(ikb_p, 0, sizeof(ikb_t));

exit:
    return status;
}


/* [function] ikb_encode
 * ----------------------------------------------------------------------------
 * Used to encode some data using IKB encoding table.
 *
 * ----------------------------------------------------------------------------
 */
status_code_t ikb_encode(const ikb_t *ikb_p, uint8_t msg_size, gchararray message,
                         uint32_t *code_size_p, gchararray code_seq)
{
    status_code_t status = SC_OK;
    uint32_t      chips_num = 0;  /* number of chips used by message */
    uint32_t      chips_buf_sz = 0;  /* size of the buffer for encoded message */

    uint8_t       chip_id = 0; /* Chip index */
    uint32_t      msg_bit_i = 0; /* Bit iterator over the message buffer */
    uint32_t      chip_bit_i = 0; /* Bit iterator over the chips buffer */

    /* Check parameters are valid */
    if (code_size_p == NULL) {
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Check if IKB can be used for encoding */
    if (ikb_p->enc_table.encode_ability == 0) {
        g_printf("IKB cannot be used for data encoding.\n");
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    g_printf("Encoding MSG (%d bytes): %s", msg_size, message);
    uint32_t j = 0;
    for (j = 0; j < msg_size; j++) {
        g_printf(" " BIN8_FORMAT, BIN8_NUMBER(message[j]));
    }
    g_printf("\n");

    /* Calculate required buffer size
     * ------------------------------------------------------------------
     * Message size in bits / Encode ability in bits = Chips number
     * Chips number * Chip size in bits / Bits in byte = Chips buffer size
     * ------------------------------------------------------------------
     */
    if (msg_size > 0) {
        /* Calculate number of chips needed to encode the data */
        chips_num = (msg_size * BITS_IN_BYTE) / ikb_p->enc_table.encode_ability + 1;

        /* Calculate number of bytes needed to store all bytes of encoded message */
        chips_buf_sz = (chips_num * ikb_p->enc_table.code_len_bits) / BITS_IN_BYTE + 1;
    }
    *code_size_p = chips_buf_sz;

    g_printf("Code buffer size required (chip: %db): %d bytes\n", ikb_p->enc_table.code_len_bits, *code_size_p);

    if (code_seq != NULL) {
        g_printf("Encoding your message ...\n");
        /* Encode the message using IKB encoding table */
        for (chip_id = 0; chip_id < chips_num; chip_id++) {
            // Step 1. Divide MSG by encoding units.
            uint32_t chip_unit = 0;
            uint32_t msg_bit_start = chip_id * ikb_p->enc_table.encode_ability;
            uint32_t msg_bit_end = msg_bit_start + ikb_p->enc_table.encode_ability;
            uint8_t msg_bit_value = 0;

            /* Cut the maximum end bit */
            msg_bit_end = (msg_bit_end < (msg_size * BITS_IN_BYTE)) ? msg_bit_end : (msg_size * BITS_IN_BYTE);

            g_printf("Chip from %d to %d bit.\n", msg_bit_start, msg_bit_end);
            for (msg_bit_i = msg_bit_start; msg_bit_i < msg_bit_end; msg_bit_i++) {
                msg_bit_value = (message[msg_bit_i / BITS_IN_BYTE] & (0x80 >> (msg_bit_i % BITS_IN_BYTE))) ? 1 : 0;
                chip_unit = (chip_unit << 1) | msg_bit_value;
            }

            // Step 2. Get the code sequence for encoding unit.
            uint8_t * chip_code_p = ikb_p->enc_table.codes[chip_unit];

            // Step 3. Copy the code sequence to the buffer.
            uint32_t code_bit_start = chip_id * ikb_p->enc_table.code_len_bits;
            uint32_t code_bit_end = code_bit_start + ikb_p->enc_table.code_len_bits;
            uint8_t code_bit_value = 0;
            uint8_t code_bit_mask = 0;

            g_printf("Code from %d to %d bit.\n", code_bit_start, code_bit_end);
            for (chip_bit_i = code_bit_start; chip_bit_i < code_bit_end; chip_bit_i++) {
                code_bit_mask = 0x80 >> ((chip_bit_i - code_bit_start) % BITS_IN_BYTE);
                code_bit_value = (chip_code_p[(chip_bit_i - code_bit_start) / BITS_IN_BYTE] & code_bit_mask) ? 1 : 0;
                if (code_bit_value) {
                    code_seq[chip_bit_i / BITS_IN_BYTE] |= (0x80 >> (chip_bit_i % BITS_IN_BYTE));
                }
                g_printf(" BitMask: " BIN8_FORMAT "; BitValue: %d; ChipBit: %d;\n", BIN8_NUMBER(code_bit_mask), code_bit_value, chip_bit_i);
            }

            uint32_t i = 0;
            g_printf ("Chip ID: %d; Chip unit: " BIN32_FORMAT " (%d); Chip code:", chip_id, BIN32_NUMBER(chip_unit), chip_unit);
            for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                g_printf(" " BIN8_FORMAT, BIN8_NUMBER(chip_code_p[i]));
            }
            g_printf("; Code seq:");
            for (i = 0; i < *code_size_p; i++) {
                g_printf(" " BIN8_FORMAT, BIN8_NUMBER(code_seq[i]));
            }
            g_printf("\n");
        }
    }

    g_printf("Sum: %d; BE: %d; msg_len: %d; chips: %d; buf_sz: %d;\n", ikb_p->sum, ikb_p->enc_table.encode_ability, msg_size, chips_num, chips_buf_sz);

exit:
    return status;
}


status_code_t ikb_decode(const ikb_t *ikb_p, uint32_t code_size, gchararray code_seq,
                         uint8_t *msg_size_p, gchararray message)
{
    status_code_t status = SC_OK;
    uint32_t      chips_num = 0;  /* number of chips used by message */
    uint32_t      msg_buf_sz = 0;  /* size of the buffer for encoded message */

    uint8_t       chip_id = 0; /* Chip index */
    uint32_t      msg_bit_i = 0; /* Bit iterator over the message buffer */
    uint32_t      chip_bit_i = 0; /* Bit iterator over the chips buffer */
    uint8_t      *chip_code_p = NULL;
    uint32_t      i = 0;

    /* Check parameters are valid */
    if (msg_size_p == NULL) {
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Check if IKB can be used for encoding */
    if (ikb_p->enc_table.encode_ability == 0) {
        g_printf("Неможливо декодувати повідомлення обраною ІКВ.\n");
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    g_printf("Декодування буфера (%d байт): %s", code_size, code_seq);
    uint32_t j = 0;
    for (j = 0; j < code_size; j++) {
        g_printf(" " BIN8_FORMAT, BIN8_NUMBER(code_seq[j]));
    }
    g_printf("\n");

    /* Calculate required buffer size for message
     * ------------------------------------------------------------------
     * Message size in bits / Chip size in bits = Chips number
     * Chips number * Encode ability in bits / Bits in byte = Chips buffer size
     * ------------------------------------------------------------------
     */
    if (code_size > 0) {
        /* Calculate number of chips needed to encode the data */
        chips_num = (code_size * BITS_IN_BYTE) / ikb_p->enc_table.code_len_bits;

        /* Calculate number of bytes needed to store all bytes of encoded message */
        msg_buf_sz = (chips_num * ikb_p->enc_table.encode_ability) / BITS_IN_BYTE;
    }
    *msg_size_p = msg_buf_sz;

    g_printf("Необхідний розмір буфера (чіп: %d байт): %d байт\n", ikb_p->enc_table.code_len_bits, *msg_size_p);

    if (message != NULL) {
        g_printf("Декодуємо повідомлення ...\n");
        chip_code_p = calloc(1, ikb_p->enc_table.code_len_bytes);

        /* Decode the message using IKB encoding table */
        for (chip_id = 0; chip_id < chips_num; chip_id++) {
            // Step 0. Initialization.
            memset(chip_code_p, 0, ikb_p->enc_table.code_len_bytes);

            // Step 1. Copy the code sequence to the buffer.
            uint32_t code_bit_start = chip_id * ikb_p->enc_table.code_len_bits;
            uint32_t code_bit_end = code_bit_start + ikb_p->enc_table.code_len_bits;
            uint8_t code_bit_value = 0;
            uint8_t code_bit_mask = 0;

            /* Cut the maximum end bit */
            code_bit_end = (code_bit_end < (code_size * BITS_IN_BYTE)) ? code_bit_end : (code_size * BITS_IN_BYTE);

            g_printf("Межі коду від %d до %d байту.\n", code_bit_start, code_bit_end);
            for (chip_bit_i = code_bit_start; chip_bit_i < code_bit_end; chip_bit_i++) {
                code_bit_mask = 0x80 >> (chip_bit_i % BITS_IN_BYTE);
                code_bit_value = (code_seq[chip_bit_i / BITS_IN_BYTE] & code_bit_mask) ? 1 : 0;

                if (code_bit_value) {
                    chip_code_p[(chip_bit_i - code_bit_start) / BITS_IN_BYTE] |= (0x80 >> ((chip_bit_i - code_bit_start) % BITS_IN_BYTE));
                }
                g_printf(" BitMask: " BIN8_FORMAT "; BitValue: %d; ChipBit: %d;\n", BIN8_NUMBER(code_bit_mask), code_bit_value, chip_bit_i);
            }

            // Step 2. Get the encoding unit for the code sequence.
            int8_t chip_unit = -1;
            uint8_t code_id = 0;

            for (code_id = 0; code_id < ikb_p->enc_table.codes_num; code_id++) {
                chip_unit = code_id;

                /* Порівняємо всі байти коду */
                for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                    if (ikb_p->enc_table.codes[code_id][i] != chip_code_p[i]) {
                        chip_unit = -1;
                        break;
                    }
                }

                /* Якщо коди однакові, ми виходимо */
                if (chip_unit >= 0) {
                    break;
                }
            }

            if (chip_unit == -1) {
                bool_t is_zeroed = TRUE;
                for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                    if (0 != chip_code_p[i]) {
                        is_zeroed = FALSE;
                        break;
                    }
                }

                /* Fail if buffer not zeroed */
                if (FALSE == is_zeroed) {
                    g_printf("Помилка декодування чіпа:\n");
                    for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                        g_printf(" " BIN8_FORMAT, BIN8_NUMBER(chip_code_p[i]));
                    }
                    g_printf("\n");
                    status = SC_FLOW_FAILED;
                    goto exit;
                } else {
                    chip_unit = 0x0;
                }

            }

            // Step 3. Compound MSG by encoding units.
            uint32_t msg_bit_start = chip_id * ikb_p->enc_table.encode_ability;
            uint32_t msg_bit_end = msg_bit_start + ikb_p->enc_table.encode_ability;
            uint8_t msg_bit_value = 0;

            /* Cut the maximum end bit */
            msg_bit_end = (msg_bit_end < (*msg_size_p * BITS_IN_BYTE)) ? msg_bit_end : (*msg_size_p * BITS_IN_BYTE);

            g_printf("Chip from %d to %d bit.\n", msg_bit_start, msg_bit_end);
            for (msg_bit_i = msg_bit_start; msg_bit_i < msg_bit_end; msg_bit_i++) {
                msg_bit_value = (chip_unit & (0x1 << (msg_bit_end - msg_bit_i - 1))) ? 0x80 : 0; /* Use 0x80 instead of 0x1 to use right shift operator */
                message[msg_bit_i / BITS_IN_BYTE] |= msg_bit_value >> (msg_bit_i % BITS_IN_BYTE);
            }
            g_printf ("Chip ID: %d; Chip unit: " BIN32_FORMAT " (%d); Chip code:", chip_id, BIN32_NUMBER(chip_unit), chip_unit);
            for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                g_printf(" " BIN8_FORMAT, BIN8_NUMBER(chip_code_p[i]));
            }
            g_printf("; Message:");
            for (i = 0; i < *msg_size_p; i++) {
                g_printf(" " BIN8_FORMAT, BIN8_NUMBER(message[i]));
            }
            g_printf("\n");
        }
    }

    /* g_printf("Sum: %d; BE: %d; msg_len: %d; chips: %d; buf_sz: %d;\n", ikb_p->sum, ikb_p->enc_table.encode_ability, msg_size, chips_num, chips_buf_sz); */


exit:
    if (chip_code_p != NULL) {
        free(chip_code_p);
    }

    return status;
}


uint8_t reverse_bit_in_byte(uint8_t byte, uint8_t bit_position)
{
    uint8_t bit_value = (0x80 >> (bit_position % BITS_IN_BYTE)) & byte;

    if (0 == bit_value) {
        /* Bit value should become 1 */
        byte |= 0x80 >> (bit_position % BITS_IN_BYTE);
    } else {
        /* Bit value should become 0 */
        byte &= ~(0x80 >> (bit_position % BITS_IN_BYTE));
    }

    return byte;
}

status_code_t ikb_noise_apply(uint32_t noised_bits, uint32_t code_size_in_bits, gchararray code_seq)
{
    status_code_t status = SC_OK;
    uint32_t      bit_idx = 0;
    uint32_t      noise_applied = 0;
    bool_t        is_noised_bit = FALSE;
    uint32_t      noise_value = 0;

    srand(time(NULL));

    if (noised_bits == 0) {
        goto exit;
    }

    g_printf("code_size_in_bits: %d\n", code_size_in_bits);
    for (bit_idx = 0; bit_idx < code_size_in_bits; bit_idx++) {
        noise_value = rand() % BITS_NOISE_PROB;
        is_noised_bit = (noise_value == 0) ? TRUE : FALSE;

        if (is_noised_bit) {
            g_print("Noised %d bit in " BIN8_FORMAT " \n", bit_idx, BIN8_NUMBER(code_seq[bit_idx / BITS_IN_BYTE]));
            code_seq[bit_idx / BITS_IN_BYTE] = reverse_bit_in_byte(code_seq[bit_idx / BITS_IN_BYTE], bit_idx);
            g_printf("Noised byte: " BIN8_FORMAT "\n", BIN8_NUMBER(code_seq[bit_idx / BITS_IN_BYTE]));
            noise_applied++;
        }

        if (noise_applied >= noised_bits) {
            break;
        }
    }
    g_print("Noised applied to %d bits.\n", noise_applied);

exit:
    return status;
}

status_code_t ikb_noise_restore(const ikb_t *ikb_p, uint32_t code_size, uint8_t *code_seq)
{
    status_code_t status = SC_OK;
    uint32_t      chips_num = 0;  /* number of chips used by message */
    uint8_t       chip_id = 0; /* Chip index */
    uint32_t      chip_bit_i = 0; /* Bit iterator over the chips buffer */
    uint8_t      *chip_code_p = NULL;
    uint32_t      i = 0;

    /* Check if IKB can be used for encoding */
    if (ikb_p->enc_table.encode_ability == 0) {
        g_printf("Неможливо відновити дані обраною ІКВ.\n");
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    g_printf("Відновлення даних буфера (%d байт): %s", code_size, code_seq);
    uint32_t j = 0;
    for (j = 0; j < code_size; j++) {
        g_printf(" " BIN8_FORMAT, BIN8_NUMBER(code_seq[j]));
    }
    g_printf("\n");

    /* Calculate number of chips needed to encode the data */
    chips_num = (code_size * BITS_IN_BYTE) / ikb_p->enc_table.code_len_bits + 1;

    /* Allocate temporary code buffer */
    chip_code_p = calloc(1, ikb_p->enc_table.code_len_bytes);

    /* Restore the code using IKB encoding table */
    for (chip_id = 0; chip_id < chips_num; chip_id++) {
        // Step 0. Initialization.
        memset(chip_code_p, 0, ikb_p->enc_table.code_len_bytes);

        // Step 1. Copy the code sequence to the buffer.
        uint32_t code_bit_start = chip_id * ikb_p->enc_table.code_len_bits;
        uint32_t code_bit_end = code_bit_start + ikb_p->enc_table.code_len_bits;
        uint8_t code_bit_value = 0;
        uint8_t chip_bit_value = 0;
        uint8_t code_bit_mask = 0;
        uint8_t chip_bit_mask = 0;

        /* Cut the maximum end bit */
        code_bit_end = (code_bit_end < (code_size * BITS_IN_BYTE)) ? code_bit_end : (code_size * BITS_IN_BYTE);

        g_printf("Межі коду від %d до %d байту.\n", code_bit_start, code_bit_end);
        for (chip_bit_i = code_bit_start; chip_bit_i < code_bit_end; chip_bit_i++) {
            code_bit_mask = 0x80 >> (chip_bit_i % BITS_IN_BYTE);
            code_bit_value = (code_seq[chip_bit_i / BITS_IN_BYTE] & code_bit_mask) ? 1 : 0;

            if (code_bit_value) {
                chip_code_p[(chip_bit_i - code_bit_start) / BITS_IN_BYTE] |= (0x80 >> ((chip_bit_i - code_bit_start) % BITS_IN_BYTE));
            }
            g_printf(" BitMask: " BIN8_FORMAT "; BitValue: %d; ChipBit: %d;\n", BIN8_NUMBER(code_bit_mask), code_bit_value, chip_bit_i);
        }

        // Step 2. Get the encoding unit for the code sequence.
        int8_t chip_unit = -1;
        uint8_t code_id = 0;
        uint8_t missed_bits = 0;
        uint8_t best_chip_miss = ikb_p->enc_table.code_len_bits;

        for (code_id = 0; code_id < ikb_p->enc_table.codes_num; code_id++) {
            missed_bits = 0;
            /* Порівняємо всі біти коду */
            for (i = 0; i < ikb_p->enc_table.code_len_bits; i++) {
                code_bit_value = ikb_p->enc_table.codes[code_id][i / BITS_IN_BYTE] & (0x80 >> (i % BITS_IN_BYTE));
                chip_bit_value = chip_code_p[i / BITS_IN_BYTE] & (0x80 >> (i % BITS_IN_BYTE));

                if (code_bit_value != chip_bit_value) {
                    missed_bits++;
                }
            }

            /* Якщо коди однакові, ми виходимо */
            if (missed_bits < best_chip_miss) {
                best_chip_miss = missed_bits;
                chip_unit = code_id;
            }
            g_printf("missed_bits: %d; best_chip_miss: %d; chip_unit: %d;\n", missed_bits, best_chip_miss, chip_unit);
        }


        if (best_chip_miss <= ikb_p->err_fixed) {
            /* Кількість помилок допускає відновлення */
            for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                chip_code_p[i] = ikb_p->enc_table.codes[chip_unit][i];
            }
            g_printf("Відновлено %d пошкодених бітів.\n", best_chip_miss);

        } else {
            if (best_chip_miss <= ikb_p->err_found) {
                /* Кількість помилок не дозволяє відновити повідомлення */
                g_printf("Знайдено %d пошкодених бітів.\n", best_chip_miss);

            } else {
                /* Неможливо визначити кількість помилок */
                g_printf("Неможливо відновити пошкоджений код:");
                for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                    g_printf(" " BIN8_FORMAT, BIN8_NUMBER(chip_code_p[i]));
                }
                g_printf("\n");
            }

            /* Set zeroed chip buffer */
            for (i = 0; i < ikb_p->enc_table.code_len_bytes; i++) {
                chip_code_p[i] = 0;
            }
        }

        /* Copy final buffer */
        for (chip_bit_i = code_bit_start; chip_bit_i < code_bit_end; chip_bit_i++) {
            code_bit_mask = 0x80 >> (chip_bit_i % BITS_IN_BYTE);
            code_bit_value = (code_seq[chip_bit_i / BITS_IN_BYTE] & code_bit_mask) ? 1 : 0;

            chip_bit_mask = 0x80 >> ((chip_bit_i - code_bit_start) % BITS_IN_BYTE);
            chip_bit_value = (chip_code_p[(chip_bit_i - code_bit_start) / BITS_IN_BYTE] & chip_bit_mask) ? 1 : 0;

            if (code_bit_value != chip_bit_value) {
                if (chip_bit_value == 1) {
                    code_seq[chip_bit_i / BITS_IN_BYTE] |= code_bit_mask;
                } else {
                    code_seq[chip_bit_i / BITS_IN_BYTE] &= ~code_bit_mask;
                }
            }
            g_printf(" CodeBitMask: " BIN8_FORMAT "; ChipBitMask: " BIN8_FORMAT "; Fixed: %s;\n", BIN8_NUMBER(code_bit_mask), BIN8_NUMBER(chip_bit_mask), code_bit_value != chip_bit_value ? "yes" : "no");
        }
    }

exit:
    if (chip_code_p != NULL) {
        free(chip_code_p);
    }

    return status;
}


status_code_t ikb_seq_get_str_size(const ikb_t *ikb_p, uint8_t *buf_size)
{
    status_code_t status = SC_OK;
    uint8_t       i = 0;
    uint8_t       digits_num = 0;
    uint8_t       total_size = 0;

    if (ikb_p == NULL) {
        MSG_ERR("Вказівник на ІКВ не ініціалізований");
        status = SC_PARAM_NULL;
        goto exit;
    }

    for (i = 0; i < ikb_p->seq_len; i++) {
        /* Знайдемо кількість цифр у числі */
        digits_num = (ikb_p->sequence[i] == 0) ? 1 : (log10(ikb_p->sequence[i]) + 1);

        /* Додамо кількість цифр до розміру буфера + 1 байт для коми */
        total_size += digits_num + 1;
    }

    /* Додатковий байт для кінцевого символа '\0' */
    total_size += 1;

    *buf_size = total_size;

exit:
    return status;
}


status_code_t ikb_seq_to_str(const ikb_t *ikb_p, uint8_t buf_size, gchararray buffer)
{
    status_code_t status = SC_OK;
    uint8_t       i = 0;
    uint8_t       required_size = 0;

    if (ikb_p == NULL) {
        MSG_ERR("Вказівник на ІКВ не ініціалізований");
        status = SC_PARAM_NULL;
        goto exit;
    } else if (buffer == NULL) {
        MSG_ERR("Буфер не ініціалізовано");
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Перевіряємо розмір вхідного буфера */
    status = ikb_seq_get_str_size(ikb_p, &required_size);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка при обчисленні розміру буфера: %s", status_get_str(status));
        goto exit;
    }

    if (buf_size < required_size) {
        LOG_ERR("Розмір буферу занадто малий. Потрібно щонайменше %d байт", required_size);
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    /* Копіюємо перший елемент */
    sprintf(buffer, "%d", ikb_p->sequence[0]);

    /* Копіюємо наступні елементи */
    for (i = 1; i < ikb_p->seq_len; i++) {
        sprintf(buffer, "%s,%d", buffer, ikb_p->sequence[i]);
    }

    /* Завершуємо стрічку нуль-символом */
    sprintf(buffer, "%s%c", buffer, '\0');

exit:
    return status;
}