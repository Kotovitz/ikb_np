#include "ikb.h"


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

    for (i = 0; i < ikb_p->enc_table.codes_num; i++) {
        /* Виділення масиву для коду */
        ikb_p->enc_table.codes[i] = (uint8_t *)calloc(ikb_p->enc_table.code_len_bytes, sizeof(uint8_t));

        /* Ініціалізація бітового вектора коду */
        for (j = 0, k = 0; j < ikb_p->seq_len; j++, k += ikb_p->sequence[j]) {
            /* Перенесення вказівника у разі виходу за межі масиву */
            k = k % ikb_p->enc_table.code_len_bits;

            /* Обчислюємо адресу байта і біта */
            k_byte = k / BITS_IN_BYTE;
            k_bit = k - k_byte * BITS_IN_BYTE;

            /* Встановлюємо заданий біт у значення 1 */
            ikb_p->enc_table.codes[i][k_byte] = 0x80 >> k_bit;
        }
    }

exit:
    return status;
}


status_code_t ikb_delete(ikb_t *ikb_p)
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


status_code_t ikb_encode(const ikb_t *ikb_p,
                         uint8_t msg_size, gchararray message,
                         uint32_t *code_size, gchararray code_seq)
{
    status_code_t status = SC_FLOW_UNSUPPORTED;

exit:
    return status;
}


status_code_t ikb_decode(const ikb_t *ikb_p,
                         uint32_t code_size, gchararray code_seq,
                         uint8_t *msg_size, gchararray message)
{
    status_code_t status = SC_FLOW_UNSUPPORTED;

exit:
    return status;
}


status_code_t ikb_code_to_str(uint32_t code_size, gchararray code_seq,
                              uint32_t *msg_size, gchararray message)
{
    status_code_t status = SC_FLOW_UNSUPPORTED;

exit:
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