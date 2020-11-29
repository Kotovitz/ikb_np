#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ikb.h"
#include "channel.h"


#define MAX_MESSAGE_SIZE (256) /* 256 bytes */
#define FRAME_LABEL_XPOS 0.05  /* Frame label alignment on horizontal axis */
#define FRAME_LABEL_YPOS 0.5   /* Frame label alignment on vertical axis */

/* Оголошуємо основні елементи вікна */
#define GUI_ELEMENT_P(name, type) type *name = NULL

GUI_ELEMENT_P(window, GtkWidget);
GUI_ELEMENT_P(window_icon, GdkPixbuf);
GUI_ELEMENT_P(window_layout, GtkBox); /* Вертикальний */

/* Панель налаштувань */
GUI_ELEMENT_P(config_frame, GtkFrame);
GUI_ELEMENT_P(config_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(settings_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(ikb_label, GtkLabel);
GUI_ELEMENT_P(ikb_combo, GtkComboBox);
GUI_ELEMENT_P(config_separator1, GtkSeparator); /* Вертикальний */
GUI_ELEMENT_P(noise_label, GtkLabel);
GUI_ELEMENT_P(noise_spin_button, GtkSpinButton);
GUI_ELEMENT_P(config_separator2, GtkSeparator); /* Вертикальний */
GUI_ELEMENT_P(cut_label, GtkLabel);
GUI_ELEMENT_P(cut_spin_button, GtkSpinButton);
GUI_ELEMENT_P(config_separator3, GtkSeparator); /* Горизонтальний */
GUI_ELEMENT_P(values_panel_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(values_panel_grid, GtkGrid);
GUI_ELEMENT_P(chip_bits_label, GtkLabel);
GUI_ELEMENT_P(bits_enc_ability_label, GtkLabel);
GUI_ELEMENT_P(code_redund_label, GtkLabel);
GUI_ELEMENT_P(bits_unused_label, GtkLabel);
GUI_ELEMENT_P(err_found_label, GtkLabel);
GUI_ELEMENT_P(err_fixed_label, GtkLabel);
GUI_ELEMENT_P(enc_table_button, GtkButton);

/* Панель повідомлень */
GUI_ELEMENT_P(msg_send_frame, GtkFrame);
GUI_ELEMENT_P(msg_send_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(msg_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(msg_entry, GtkEntry);
GUI_ELEMENT_P(msg_len_label, GtkLabel);
GUI_ELEMENT_P(msg_aclean_send_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(msg_autoclean_check_button, GtkCheckButton);
GUI_ELEMENT_P(msg_send_button, GtkButton);

/* Панель надсилання та отримання даних */
GUI_ELEMENT_P(rx_tx_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(rx_frame, GtkFrame);
GUI_ELEMENT_P(rx_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(rx_scrolled_window, GtkScrolledWindow);
GUI_ELEMENT_P(rx_text_view, GtkTextView);
GUI_ELEMENT_P(rx_buffer, GtkTextBuffer);

GUI_ELEMENT_P(tx_frame, GtkFrame);
GUI_ELEMENT_P(tx_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(tx_scrolled_window, GtkScrolledWindow);
GUI_ELEMENT_P(tx_text_view, GtkTextView);
GUI_ELEMENT_P(tx_buffer, GtkTextBuffer);

/* Діалогове вікно таблиці кодування */
GUI_ELEMENT_P(dialog_window, GtkWidget);
GUI_ELEMENT_P(dialog_layout, GtkBox);
GUI_ELEMENT_P(dialog_text_view, GtkTextView);
GUI_ELEMENT_P(dialog_buffer, GtkTextBuffer);


/* База даних всіх відомих ІКВ, її розмір та індекс активної ІКВ */
ikb_t   *g_ikb_db = NULL;
uint8_t  g_ikb_db_size = 0;
uint8_t  g_active_ikb_id = 0;
uint8_t  g_chip_cut_bits = 0;
char    *g_client_version = "0.6.0-alpha";


/* Функція для завантаження зображення у буфер даних */
GdkPixbuf * create_pixbuf(const gchar * filepath)
{
    GdkPixbuf * buffer = NULL;
    GError    * error = NULL;

    buffer = gdk_pixbuf_new_from_file(filepath, &error);
    if (!buffer) {
        printf("%s\n", error->message);
        g_error_free(error);
    }

    return buffer;
}

/* Функція для обробки введених повідомлень */
void send_msg_cb(GtkWidget *widget, gpointer window)
{
    status_code_t status = SC_OK;
    gchararray    buffer_p = NULL;
    uint8_t       buffer_size = 0;
    gchararray    code_p = NULL;
    uint32_t      code_size = 0;
    gchararray    noised_code_p = NULL;
    uint32_t      noise_level = 0;
    uint32_t      noised_code_size_in_bits = 0;
    gchararray    tx_msg_buffer = NULL;
    uint32_t      tx_msg_size = 0;
    uint32_t      i = 0;

    /* Копіюємо дані з рядку вводу до буфера */
    buffer_size = strlen(gtk_entry_get_text((GtkEntry *)msg_entry));
    buffer_p = calloc(1, buffer_size + 1);
    sprintf(buffer_p, "%s%c", gtk_entry_get_text((GtkEntry *)msg_entry), '\0');

    g_printf("Надсилаємо повідомлення: %s.\n", buffer_p);

    /* Отримаємо необхідний розмір буфера коду в байтах */
    status = ikb_encode(&g_ikb_db[g_active_ikb_id], buffer_size, buffer_p, &code_size, NULL);
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Помилка отримання розміру коду: %s <-\n", status_get_str(status));
        goto exit;
    }

    /* Виділимо необхідний буфер коду та закодуємо вхідне повідомлення */
    code_p = calloc(1, code_size);
    noised_code_p = calloc(1, code_size);

    status = ikb_encode(&g_ikb_db[g_active_ikb_id], buffer_size, buffer_p, &code_size, code_p);
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Помилка кодування повідомлення: %s", status_get_str(status));
        goto exit;
    }

    /* Copy coded buffer to noised code buffer */
    memcpy(noised_code_p, code_p, code_size);

    noise_level = (uint32_t)gtk_spin_button_get_value_as_int(noise_spin_button);
    noised_code_size_in_bits = ((code_size * BITS_IN_BYTE) / g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits) * g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits;

    status = ikb_noise_apply(noise_level, noised_code_size_in_bits, noised_code_p);
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Помилка накладання завад: %s", status_get_str(status));
        goto exit;
    }

    /* Виведемо надіслані дані у вікно користувача */
    if (buffer_p) {
        tx_msg_size = buffer_size + (code_size * 20) + 1000;
        tx_msg_buffer = calloc(1, tx_msg_size);

        sprintf(tx_msg_buffer, " <ПОВІД> %s", buffer_p);

        sprintf(tx_msg_buffer, "%s\n <ЗАКОД>", tx_msg_buffer);
        for (i = 0; i < code_size; i++) {
            sprintf(tx_msg_buffer, "%s " BIN8_FORMAT, tx_msg_buffer, BIN8_NUMBER(code_p[i]));
        }
        sprintf(tx_msg_buffer, "%s%c", tx_msg_buffer, '\0');

        sprintf(tx_msg_buffer, "%s\n <ЗАВАД>", tx_msg_buffer);
        for (i = 0; i < code_size; i++) {
            sprintf(tx_msg_buffer, "%s " BIN8_FORMAT, tx_msg_buffer, BIN8_NUMBER(noised_code_p[i]));
        }
        sprintf(tx_msg_buffer, "%s%c", tx_msg_buffer, '\0');

        channel_write(-1, code_size, (uint8_t *)noised_code_p);
        //g_printf("MSG: %s\n", tx_msg_buffer);

        tx_buffer = gtk_text_view_get_buffer(tx_text_view);
        //g_printf("Setting buffer to tx_buffer [%p] of %lu bytes\n", tx_buffer, strlen(tx_msg_buffer));
        gtk_text_buffer_set_text(tx_buffer, tx_msg_buffer, strlen(tx_msg_buffer));
    }

    g_printf("Надіслано.\n");

    /* Очищуємо рядок вводу */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_autoclean_check_button))) {
        gtk_entry_set_text(msg_entry, "");
        g_printf("Буфер очищено.\n");
    }


exit:
    if (tx_msg_buffer != NULL) {
        free(tx_msg_buffer);
    }

    if (code_p != NULL) {
        free(code_p);
    }

    if (noised_code_p != NULL) {
        free(noised_code_p);
    }

    if (buffer_p != NULL) {
        free(buffer_p);
    }

    return;
}

/* Функція для оновлення довжини введених повідомлень */
void update_msg_data_cb(GtkWidget *widget, gpointer window)
{
    gchararray    buffer_p = NULL;

    buffer_p = calloc(1, 100);
    if (buffer_p == NULL) {
        goto exit;
    }

    sprintf(buffer_p, "Знаків використано: %3lu / %d", strlen(gtk_entry_get_text((GtkEntry *)msg_entry)), MAX_MESSAGE_SIZE);
    gtk_label_set_text(msg_len_label, buffer_p);

exit:
    if (buffer_p != NULL) {
        free(buffer_p);
    }

    return;
}

/* Функція оновлює значення панелі властивостей ІКВ */
void update_values_panel()
{
    gchararray buffer = NULL;

    /* Виділимо тимчасовий буфер */
    buffer = calloc(1, 50);

    sprintf(buffer, "Довжина чіпа: %d біт", g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits);
    gtk_label_set_text(chip_bits_label, buffer);

    sprintf(buffer, "Кодувальна здатність: %d біт", g_ikb_db[g_active_ikb_id].enc_table.encode_ability);
    gtk_label_set_text(bits_enc_ability_label, buffer);

    sprintf(buffer, "Надлишковість: %d %%", (int)(((float)g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits / g_ikb_db[g_active_ikb_id].enc_table.encode_ability) * 100));
    gtk_label_set_text(code_redund_label, buffer);

    sprintf(buffer, "Невикористано: %d кодів", (int)(g_ikb_db[g_active_ikb_id].enc_table.codes_num - pow(2, g_ikb_db[g_active_ikb_id].enc_table.encode_ability)));
    gtk_label_set_text(bits_unused_label, buffer);

    sprintf(buffer, "Помилок (знайд.): %d біт", g_ikb_db[g_active_ikb_id].err_found);
    gtk_label_set_text(err_found_label, buffer);

    sprintf(buffer, "Помилок (випр.): %d біт", g_ikb_db[g_active_ikb_id].err_fixed);
    gtk_label_set_text(err_fixed_label, buffer);

    /* Звільнимо тимчасовий буфер */
    free(buffer);
}

/* Функція оновлення довжини обрізання чіпів */
void update_chip_cut_length(GtkWidget *widget, gpointer window)
{
    uint8_t cut_value = 0;

    cut_value = gtk_spin_button_get_value_as_int(cut_spin_button);
    if (cut_value >= g_ikb_db[g_active_ikb_id].enc_table.code_len_bits) {
        /* Найменший допустимий розмір чіпу - 1 біт */
        cut_value = g_ikb_db[g_active_ikb_id].enc_table.code_len_bits - 1;
        gtk_spin_button_set_value(cut_spin_button, cut_value);
    }

    /* Зберігаємо кількість обрізаних бітів у глобальній змінній */
    g_chip_cut_bits = cut_value;

    /* Врахуємо параметри обрізання чіпів */
    g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits = g_ikb_db[g_active_ikb_id].enc_table.code_len_bits - g_chip_cut_bits;
    g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bytes = g_ikb_db[g_active_ikb_id].enc_table.cutted_len_bits / BITS_IN_BYTE + 1;

    /* Оновимо панель параметрів ІКВ */
    update_values_panel();

    return;
}

/* Іункція виконує оновлення вхідного буфера з потоку графічного інтерфейсу */
int rx_buffer_update(void *data_arg)
{
    rx_update_t * data = (rx_update_t *)data_arg;

    gtk_text_buffer_set_text(data->buffer_p, data->message_p, strlen(data->message_p));
    g_free(data);

    return 0;
}

/* Функція для обробки отриманих повідомлень */
void rx_reader(uint16_t buffer_size, uint8_t *buffer_p)
{
    status_code_t status = SC_OK;
    uint32_t      i = 0;
    uint8_t      *restored_buffer_p = NULL;
    gchararray    message_p = NULL;
    uint8_t       message_size = 0;
    uint32_t      rx_msg_size = 0;
    gchararray    rx_msg_buffer = NULL;
    rx_update_t  *data = g_new0(rx_update_t, 1);

    /* Виведення отриманого буфера даних */
    g_printf("Отримано буфер даних розміром %d байт:\n", buffer_size);
    for (i = 0; i < buffer_size; i++) {
        g_printf(" " BIN8_FORMAT, BIN8_NUMBER(buffer_p[i]));
    }
    g_printf("\n");

    /* Встановлення вказівника на буфер отриманих даних */
    data->buffer_p = gtk_text_view_get_buffer(rx_text_view);

    /* Buffer used for restored data */
    restored_buffer_p = calloc(1, buffer_size);
    if (restored_buffer_p == NULL) {
        status = SC_NO_FREE_MEMORY;
        data->message_p = g_strdup_printf("Помилка виділення пам'яті для буфера відновлення: %s\n", status_get_str(status));
        g_printf("%s", data->message_p);
        goto exit;
    }
    memcpy(restored_buffer_p, buffer_p, buffer_size);

    /* Buffer restoration */
    status = ikb_noise_restore(&g_ikb_db[g_active_ikb_id], buffer_size, restored_buffer_p);
    if (CHECK_STATUS_FAIL(status)) {
        data->message_p = g_strdup_printf("Помилка відновлення даних: %s\n", status_get_str(status));
        g_printf("%s", data->message_p);
        goto exit;
    }

    /* Decode buffer accordingly to active IKB */
    status = ikb_decode(&g_ikb_db[g_active_ikb_id], buffer_size, (gchararray)restored_buffer_p, &message_size, NULL);
    if (CHECK_STATUS_FAIL(status)) {
        data->message_p = g_strdup_printf("Помилка отримання розміру повідомлення: %s\n", status_get_str(status));
        g_printf("%s", data->message_p);
        goto exit;
    }

    message_p = calloc(1, message_size + 1);
    if (message_p == NULL) {
        status = SC_NO_FREE_MEMORY;
        data->message_p = g_strdup_printf("Помилка виділення пам'яті для повідомлення: %s\n", status_get_str(status));
        g_printf("%s", data->message_p);
        goto exit;
    }

    status = ikb_decode(&g_ikb_db[g_active_ikb_id], buffer_size, (gchararray)restored_buffer_p, &message_size, message_p);
    if (CHECK_STATUS_FAIL(status)) {
        data->message_p = g_strdup_printf("Помилка декодування повідомлення: %s\n", status_get_str(status));
        g_printf("%s", data->message_p);
        goto exit;
    }

    /* Виведемо отримані дані у вікно користувача */
    if (buffer_p) {
        rx_msg_size = (buffer_size * 20) + message_size + 10000;
        rx_msg_buffer = calloc(1, rx_msg_size);

        sprintf(rx_msg_buffer, " <ОТРИМ>");
        for (i = 0; i < buffer_size; i++) {
            sprintf(rx_msg_buffer, "%s " BIN8_FORMAT, rx_msg_buffer, BIN8_NUMBER(buffer_p[i]));
        }

        sprintf(rx_msg_buffer, "%s\n <ВІДНВ>", rx_msg_buffer);
        for (i = 0; i < buffer_size; i++) {
            sprintf(rx_msg_buffer, "%s " BIN8_FORMAT, rx_msg_buffer, BIN8_NUMBER(restored_buffer_p[i]));
        }

        sprintf(rx_msg_buffer, "%s%c", rx_msg_buffer, '\0');
        sprintf(rx_msg_buffer, "%s\n <РОЗКД> %s", rx_msg_buffer, message_p);
        g_printf("MSG: %s\n", rx_msg_buffer);
        data->message_p = g_strdup_printf("%s", rx_msg_buffer);
    }

    gdk_threads_add_idle(rx_buffer_update, data);
    g_printf("Отримано.\n");

exit:
    if (restored_buffer_p != NULL) {
        free(restored_buffer_p);
    }

    if (rx_msg_buffer != NULL) {
        free(rx_msg_buffer);
    }

    if (message_p != NULL) {
        free(message_p);
    }

    return;
}

/* Function to open a dialog box with a message */
void quick_message (GtkWindow *parent, gchar *message)
{
    GtkWidget *dialog, *label, *content_area;
    GtkDialogFlags flags;

    /* Create the widgets */
    flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    dialog = gtk_dialog_new_with_buttons("Encoding table", parent, flags,
                                        ("OK"), GTK_RESPONSE_NONE, NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
    label = gtk_label_new (message);

    /* Ensure that the dialog box is destroyed when the user responds */
    g_signal_connect_swapped (dialog,
                            "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);

    /* Add the label, and show everything we’ve added */
    gtk_container_add (GTK_CONTAINER (content_area), label);
    gtk_widget_show_all (dialog);
}

/* Функція */
status_code_t enc_table_set(ikb_t *ikb_p)
{
    status_code_t status = SC_OK;
    gchararray    table_buffer_p = NULL;
    uint32_t      table_buffer_size = 0;
    uint32_t      i = 0, j = 0;

    if (ikb_p == NULL) {
        status = SC_PARAM_NULL;
        goto exit;
    }

    /* Needed buffer size is a (size of one code doubled) multiplied by number of codes */
    table_buffer_size = 1000;
    table_buffer_p = (gchararray)calloc(1, table_buffer_size);

    for (i = 0; i < ikb_p->enc_table.codes_num; i++) {
        sprintf(table_buffer_p, "%s %03d |", table_buffer_p, i);
        for (j = 0; j < ikb_p->enc_table.code_len_bytes; j++) {
            sprintf(table_buffer_p, "%s " BIN8_FORMAT, table_buffer_p, BIN8_NUMBER(ikb_p->enc_table.codes[i][j]));
        }
        sprintf(table_buffer_p, "%s\n", table_buffer_p);
    }
    sprintf(table_buffer_p, "%s%c", table_buffer_p, '\0');

    //g_printf("Enс. Table:\n%s\n", table_buffer_p);
    dialog_buffer = gtk_text_view_get_buffer(dialog_text_view);
    gtk_text_buffer_set_text(dialog_buffer, table_buffer_p, strlen(table_buffer_p));


exit:
    if (table_buffer_p != NULL) {
        free(table_buffer_p);
    }

    return status;
}

/* Функція */
void ikb_changed_cb(GtkWidget *widget, gpointer window)
{
    /* Update active IKB */
    g_active_ikb_id = gtk_combo_box_get_active(ikb_combo);
    g_printf("ІКВ змінено: %d \"%s\".\n", g_active_ikb_id, g_ikb_db[g_active_ikb_id].seq_str);

    /* Оновимо кількість обрізаних бітів відповідно до можливостей ІКВ */
    update_chip_cut_length(NULL, NULL);
}

/* Функція */
void show_enc_table_cb(GtkWidget *widget, gpointer window)
{
    status_code_t status = SC_OK;

    /* Create a new dialog window */
    dialog_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog_window), "Таблиця кодування ІКВ");
    gtk_window_set_position(GTK_WINDOW(dialog_window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(dialog_window), 200, 200);
    gtk_container_set_border_width(GTK_CONTAINER(dialog_window), 10);

    /* Initialize major layout */
    dialog_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));

    /* Create a text view */
    dialog_text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(dialog_text_view, FALSE);
    gtk_text_view_set_monospace(dialog_text_view, TRUE);
    gtk_text_view_set_cursor_visible (dialog_text_view, FALSE);

    /* Set the encoding table to the buffer */
    status = enc_table_set(&g_ikb_db[g_active_ikb_id]);
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Failed to update GUI with new IKB.\n");
        goto exit;
    }

    gtk_box_pack_start(GTK_BOX(dialog_layout), GTK_WIDGET(dialog_text_view), TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(dialog_window), GTK_WIDGET(dialog_layout));
    gtk_widget_show_all(dialog_window);

exit:
    return;
}

/* Функція */
status_code_t validate_arguments(int argc, char *argv[])
{
    status_code_t status = SC_OK;

    /* Перевірка кількості параметрів:
     * ./program <clid> <dstclid>
     */
    if (argc != 3) {
        g_printf("Неправильна кількість параметрів. Вкажіть номер клієнта та отримувача.\n");
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

exit:
    return status;
}

/* Функція */
status_code_t ikb_codes_init()
{
    status_code_t status = SC_OK;
    uint8_t       ikb_4_1[] = {1, 2, 3, 7};
    uint8_t       ikb_4_2[] = {1, 1, 2, 3};
    uint8_t       ikb_6_3[] = {1, 1, 2, 1, 2, 4};
    uint8_t       ikb_8_4[] = {1, 1, 1, 2, 2, 1, 3, 4};
    uint8_t       ikb_11_5[] = {1, 1, 1, 2, 2, 1, 3, 1, 3, 2, 6};

    g_ikb_db_size = 5;
    g_ikb_db = (ikb_t *)calloc(g_ikb_db_size, sizeof(ikb_t));


    status = ikb_init(4, ikb_4_1, &g_ikb_db[0]);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка ініціалізації ІКВ коду #%d: %s", 1, status_get_str(status));
        goto exit;
    }

    status = ikb_init(4, ikb_4_2, &g_ikb_db[1]);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка ініціалізації ІКВ коду #%d: %s", 2, status_get_str(status));
        goto exit;
    }

    status = ikb_init(6, ikb_6_3, &g_ikb_db[2]);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка ініціалізації ІКВ коду #%d: %s", 3, status_get_str(status));
        goto exit;
    }

    status = ikb_init(8, ikb_8_4, &g_ikb_db[3]);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка ініціалізації ІКВ коду #%d: %s", 4, status_get_str(status));
        goto exit;
    }

    status = ikb_init(11, ikb_11_5, &g_ikb_db[4]);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка ініціалізації ІКВ коду #%d: %s", 4, status_get_str(status));
        goto exit;
    }

exit:
    return status;
}

/* Функція */
status_code_t gui_init(int32_t client_id)
{
    int  i = 0;
    char window_name[100] = {0};

    /* Створюємо та налаштовуємо головне вікно */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sprintf(window_name, "IKB Noise Protection v.%s - Client #%d", g_client_version, client_id);
    gtk_window_set_title(GTK_WINDOW(window), window_name);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 800);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    /* Завантажуємо та встановлюємо піктограму */
    window_icon = create_pixbuf("app_icon.png");
    gtk_window_set_icon(GTK_WINDOW(window), window_icon);

    /* Створюємо головний шар */
    window_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));

    /* Панель налаштувань */
    config_frame = GTK_FRAME(gtk_frame_new("Налаштування"));
    gtk_frame_set_label_align(config_frame, FRAME_LABEL_XPOS, FRAME_LABEL_YPOS);
    config_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    settings_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(config_layout), 5);

    ikb_label = GTK_LABEL(gtk_label_new("IKB"));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(ikb_label));
    ikb_combo = GTK_COMBO_BOX(gtk_combo_box_text_new());
    for (i = 0; i < g_ikb_db_size; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ikb_combo), NULL, g_ikb_db[i].seq_str);
    }
    gtk_combo_box_set_active(ikb_combo, 0);
    gtk_widget_set_hexpand(GTK_WIDGET(ikb_combo), TRUE);
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(ikb_combo));

    config_separator1 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(config_separator1));

    noise_label = GTK_LABEL(gtk_label_new("Шум"));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(noise_label));
    noise_spin_button = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(0, 10, 1));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(noise_spin_button));

    config_separator2 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(config_separator2));

    cut_label = GTK_LABEL(gtk_label_new("Відрізання"));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(cut_label));
    cut_spin_button = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(0, 10, 1));
    gtk_container_add(GTK_CONTAINER(settings_layout), GTK_WIDGET(cut_spin_button));

    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(settings_layout));

    /* Separator */
    config_separator3 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(config_separator3));

    /* Config panel layout */
    values_panel_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));

    /* Here we construct the container that is going pack our buttons */
    values_panel_grid = GTK_GRID(gtk_grid_new());
    gtk_grid_set_row_homogeneous(values_panel_grid, TRUE);
    gtk_grid_set_column_homogeneous(values_panel_grid, TRUE);

    /* Chips len label */
    chip_bits_label = GTK_LABEL(gtk_label_new("Довжина чіпа: 0 біт"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(chip_bits_label), 0, 0, 1, 1);

    /* Bits encode ability */
    bits_enc_ability_label = GTK_LABEL(gtk_label_new("Кодувальна здатність: 0 біт"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(bits_enc_ability_label), 1, 0, 1, 1);

    /* Code redundance */
    code_redund_label = GTK_LABEL(gtk_label_new("Надлишковість: 100 %"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(code_redund_label), 2, 0, 1, 1);

    /* Bits unused */
    bits_unused_label = GTK_LABEL(gtk_label_new("Невикористано: 0 кодів"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(bits_unused_label), 0, 1, 1, 1);

    /* Err. found */
    err_found_label = GTK_LABEL(gtk_label_new("Помилок (знайд.): 0 біт"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(err_found_label), 1, 1, 1, 1);

    /* Err. fixed */
    err_fixed_label = GTK_LABEL(gtk_label_new("Помилок (випр.): 0 біт"));
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(err_fixed_label), 2, 1, 1, 1);

    /* Enc.table button */
    enc_table_button = GTK_BUTTON(gtk_button_new_with_label("Таблиця кодування"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(enc_table_button), "Побачити таблицю кодування ІКВ");
    gtk_grid_attach(values_panel_grid, GTK_WIDGET(enc_table_button), 2, 2, 1, 1);

    /* Initialize labels */
    ikb_changed_cb(GTK_WIDGET(ikb_combo), window);

    gtk_container_add(GTK_CONTAINER(values_panel_layout), GTK_WIDGET(values_panel_grid));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(values_panel_layout));
    gtk_container_add(GTK_CONTAINER(config_frame), GTK_WIDGET(config_layout));
    gtk_container_add(GTK_CONTAINER(window_layout), GTK_WIDGET(config_frame));

    /* Блок повідомлень */
    msg_send_frame = GTK_FRAME(gtk_frame_new("Повідомлення"));
    gtk_frame_set_label_align(msg_send_frame, FRAME_LABEL_XPOS, FRAME_LABEL_YPOS);

    msg_send_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(msg_send_layout), 5);

    msg_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    msg_entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(msg_entry, MAX_MESSAGE_SIZE);
    gtk_widget_set_hexpand(GTK_WIDGET(msg_entry), TRUE);
    gtk_container_add(GTK_CONTAINER(msg_layout), GTK_WIDGET(msg_entry));

    msg_len_label = GTK_LABEL(gtk_label_new(g_strdup_printf("Знаків використано: 0 / %d", MAX_MESSAGE_SIZE)));
    gtk_label_set_xalign (msg_len_label, 0.01);
    gtk_container_add(GTK_CONTAINER(msg_layout), GTK_WIDGET(msg_len_label));
    gtk_container_add(GTK_CONTAINER(msg_send_layout), GTK_WIDGET(msg_layout));

    msg_aclean_send_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    msg_send_button = GTK_BUTTON(gtk_button_new_with_label("Надіслати"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(msg_send_button), "Надіслати повідомлення");
    gtk_container_add(GTK_CONTAINER(msg_aclean_send_layout), GTK_WIDGET(msg_send_button));

    msg_autoclean_check_button = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Очистка"));
    gtk_container_add(GTK_CONTAINER(msg_aclean_send_layout), GTK_WIDGET(msg_autoclean_check_button));

    gtk_container_add(GTK_CONTAINER(msg_send_layout), GTK_WIDGET(msg_aclean_send_layout));
    gtk_container_add(GTK_CONTAINER(msg_send_frame), GTK_WIDGET(msg_send_layout));
    gtk_container_add(GTK_CONTAINER(window_layout), GTK_WIDGET(msg_send_frame));

    /* Панель надсилання та отримання даних */
    rx_tx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));

    tx_frame = GTK_FRAME(gtk_frame_new("TX - Передавання"));
    gtk_frame_set_label_align(tx_frame, FRAME_LABEL_XPOS, FRAME_LABEL_YPOS);

    tx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(tx_layout), 5);
    tx_scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new (NULL, NULL));
    gtk_scrolled_window_set_policy(tx_scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    tx_text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(tx_text_view, FALSE);
    gtk_text_view_set_monospace(tx_text_view, TRUE);
    gtk_text_view_set_cursor_visible (tx_text_view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(tx_text_view), -1, 60);

    gtk_container_add(GTK_CONTAINER(tx_scrolled_window), GTK_WIDGET(tx_text_view));
    gtk_container_set_border_width(GTK_CONTAINER(tx_scrolled_window), 5);
    gtk_container_add(GTK_CONTAINER(tx_layout), GTK_WIDGET(tx_scrolled_window));
    gtk_container_add(GTK_CONTAINER(tx_frame), GTK_WIDGET(tx_layout));
    gtk_container_add(GTK_CONTAINER(rx_tx_layout), GTK_WIDGET(tx_frame));

    rx_frame = GTK_FRAME(gtk_frame_new("RX - Отримання"));
    gtk_frame_set_label_align(rx_frame, FRAME_LABEL_XPOS, FRAME_LABEL_YPOS);

    rx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(rx_layout), 5);
    rx_scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new (NULL, NULL));
    gtk_scrolled_window_set_policy(rx_scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

    rx_text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(rx_text_view, FALSE);
    gtk_text_view_set_monospace(rx_text_view, TRUE);
    gtk_text_view_set_cursor_visible (rx_text_view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(rx_text_view), -1, 60);

    gtk_container_add(GTK_CONTAINER(rx_scrolled_window), GTK_WIDGET(rx_text_view));
    gtk_container_set_border_width(GTK_CONTAINER(tx_scrolled_window), 5);
    gtk_container_add(GTK_CONTAINER(rx_layout), GTK_WIDGET(rx_scrolled_window));
    gtk_container_add(GTK_CONTAINER(rx_frame), GTK_WIDGET(rx_layout));
    gtk_container_add(GTK_CONTAINER(rx_tx_layout), GTK_WIDGET(rx_frame));

    gtk_container_add(GTK_CONTAINER(window_layout), GTK_WIDGET(rx_tx_layout));

    /* Вкладаємо елементи в шари */
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(window_layout));

    /* Показуємо вікно */
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_show_all(window);

    /* Приєднауємо події */
    g_signal_connect(ikb_combo, "changed", G_CALLBACK(ikb_changed_cb), NULL);
    g_signal_connect(cut_spin_button, "changed", G_CALLBACK(update_chip_cut_length), NULL);
    g_signal_connect(enc_table_button, "clicked", G_CALLBACK(show_enc_table_cb), NULL);
    g_signal_connect(msg_entry, "changed", G_CALLBACK(update_msg_data_cb), NULL);
    g_signal_connect(msg_send_button, "clicked", G_CALLBACK(send_msg_cb), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    return SC_OK;
}

/* Функція */
status_code_t client_init(int argc, char *argv[])
{
    status_code_t status = SC_OK;
    int32_t       client_id = 0;
    int32_t       dst_client_id = 0;
    char         *shmem_path = CHANNEL_ID;
    size_t        shmem_size = strlen(shmem_path);

    /* Перевіряємо вхідні параметри на правильність */
    status = validate_arguments(argc, argv);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація клієнта завершилась помилкою: %s", status_get_str(status));
        goto exit;
    }

    /* Зчитуємо ідентифікатор клієнта */
    client_id = atoi(argv[1]);
    if (client_id < 0 || client_id >= MAX_CLIENTS_NUM) {
        g_printf("Недопустиме значення номера клієнта: %d", client_id);
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    dst_client_id = atoi(argv[2]);
    if (dst_client_id < 0 || dst_client_id >= MAX_CLIENTS_NUM) {
        g_printf("Недопустиме значення номера отримувача: %d", dst_client_id);
        status = SC_PARAM_VALUE_INVALID;
        goto exit;
    }

    /* Ініціалізуємо коди ІКВ */
    status = channel_init(client_id, dst_client_id, shmem_size, shmem_path, rx_reader);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація каналу завершилась помилкою: %s", status_get_str(status));
        goto exit;
    }

    /* Ініціалізуємо коди ІКВ */
    status = ikb_codes_init();
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація ІКВ завершилась помилкою: %s", status_get_str(status));
        goto exit;
    }

    /* Ініціалізуємо графічний інтерфейс */
    status = gui_init(client_id);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація графічної оболонки завершилась помилкою: %s", status_get_str(status));
        goto exit;
    }

exit:
    return status;
}

/* Функція */
void bin_macro_test()
{
    g_printf(" BIN8_TEST  |   0: " BIN8_FORMAT "\n", BIN8_NUMBER(0));
    g_printf(" BIN8_TEST  |   1: " BIN8_FORMAT "\n", BIN8_NUMBER(1));
    g_printf(" BIN8_TEST  |   2: " BIN8_FORMAT "\n", BIN8_NUMBER(2));
    g_printf(" BIN8_TEST  |   3: " BIN8_FORMAT "\n", BIN8_NUMBER(3));
    g_printf(" BIN8_TEST  |   5: " BIN8_FORMAT "\n", BIN8_NUMBER(5));
    g_printf(" BIN8_TEST  |   8: " BIN8_FORMAT "\n", BIN8_NUMBER(8));
    g_printf(" BIN8_TEST  | 255: " BIN8_FORMAT "\n", BIN8_NUMBER(255));
    g_printf(" BIN8_TEST  | 256: " BIN8_FORMAT "\n", BIN8_NUMBER(256));
    g_printf(" BIN16_TEST |  10: " BIN16_FORMAT "\n", BIN16_NUMBER(10));
    g_printf(" BIN16_TEST |  11: " BIN16_FORMAT "\n", BIN16_NUMBER(11));
    g_printf(" BIN16_TEST |  12: " BIN16_FORMAT "\n", BIN16_NUMBER(12));
    g_printf(" BIN16_TEST |  13: " BIN16_FORMAT "\n", BIN16_NUMBER(13));
    g_printf(" BIN16_TEST |  15: " BIN16_FORMAT "\n", BIN16_NUMBER(15));
    g_printf(" BIN16_TEST |  18: " BIN16_FORMAT "\n", BIN16_NUMBER(18));
}

/* Функція */
void __attribute__ ((destructor)) client_deinit()
{
    status_code_t status = SC_OK;

    status = channel_deinit();
    if (CHECK_STATUS_FAIL(status)) {
        g_printf("Де-ініціалізація каналу завершилась помлкою: %s", status_get_str(status));
        goto exit;
    }

    g_printf("Де-ініціалізацію клієнта завершено.\n");

exit:
    return;
}

/* Основна функція програми */
int main(int argc, char *argv[])
{
    /* Ініціалізуємо GTK */
    gtk_init(&argc, &argv);

    /* Ініціалізація */
    client_init(argc, argv);

    /* Передаємо керування в GTK */
    gtk_main();

    return 0;
}