#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ikb.h"


/* Оголошуємо основні елементи вікна */
#define GUI_ELEMENT_P(name, type) type *name = NULL

GUI_ELEMENT_P(window, GtkWidget);
GUI_ELEMENT_P(window_icon, GdkPixbuf);
GUI_ELEMENT_P(window_layout, GtkBox); /* Вертикальний */

/* Панель налаштувань */
GUI_ELEMENT_P(config_frame, GtkFrame);
GUI_ELEMENT_P(config_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(ikb_label, GtkLabel);
GUI_ELEMENT_P(ikb_combo, GtkComboBox);
GUI_ELEMENT_P(config_separator1, GtkSeparator); /* Вертикальний */
GUI_ELEMENT_P(noise_label, GtkLabel);
GUI_ELEMENT_P(noise_spin_button, GtkSpinButton);
GUI_ELEMENT_P(config_separator2, GtkSeparator); /* Вертикальний */
GUI_ELEMENT_P(cut_label, GtkLabel);
GUI_ELEMENT_P(cut_spin_button, GtkSpinButton);

/* Панель повідомлень */
GUI_ELEMENT_P(msg_send_frame, GtkFrame);
GUI_ELEMENT_P(msg_send_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(msg_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(msg_entry, GtkEntry);
GUI_ELEMENT_P(msg_autoclean_check_button, GtkCheckButton);
GUI_ELEMENT_P(msg_send_button, GtkButton);

/* Панель надсилання та отримання даних */
GUI_ELEMENT_P(rx_tx_layout, GtkBox); /* Горизонтальний */
GUI_ELEMENT_P(rx_frame, GtkFrame);
GUI_ELEMENT_P(rx_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(rx_text_view, GtkTextView);
GUI_ELEMENT_P(rx_buffer, GtkTextBuffer);

GUI_ELEMENT_P(tx_frame, GtkFrame);
GUI_ELEMENT_P(tx_layout, GtkBox); /* Вертикальний */
GUI_ELEMENT_P(tx_text_view, GtkTextView);
GUI_ELEMENT_P(tx_buffer, GtkTextBuffer);


/* База даних всіх відомих ІКВ, її розмір та індекс активної ІКВ */
ikb_t   *g_ikb_db = NULL;
uint8_t  g_ikb_db_size = 0;
uint8_t  g_active_ikb_id = 0;



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
    gchararray    tx_msg_buffer = NULL;
    uint32_t      tx_msg_size = 0;

    /* Копіюємо дані з рядку вводу до буфера */
    buffer_size = strlen(gtk_entry_get_text((GtkEntry *)msg_entry));
    buffer_p = calloc(1, buffer_size + 1);
    sprintf(buffer_p, "%s%c", gtk_entry_get_text((GtkEntry *)msg_entry), '\0');

    g_printf("Надіслано повідомлення: %s.\n", buffer_p);

    /* Отримаємо необхідний розмір буфера коду в байтах */
    status = ikb_encode(&g_ikb_db[g_active_ikb_id], buffer_size, buffer_p, &code_size, NULL);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка отримання розміру коду: %s\n", status_get_str(status));
        g_printf("Помилка отримання розміру коду: %s <-\n", status_get_str(status));
        return;
    }

    /* Виділимо необхідний буфер коду та закодуємо вхідне повідомлення */
    code_p = calloc(1, code_size);

    status = ikb_encode(&g_ikb_db[g_active_ikb_id], buffer_size, buffer_p, &code_size, code_p);
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Помилка кодування повідомлення: %s", status_get_str(status));
        return;
    }

    /* Виведемо дані у вікно користувача */
    if (buffer_p) {
        tx_msg_size = buffer_size + code_size + 100;
        tx_msg_buffer = calloc(1, tx_msg_size);

        sprintf(tx_msg_buffer, "Надіслано | %s", buffer_p);
        sprintf(tx_msg_buffer, "%s\nЗакодовано | %s", tx_msg_buffer, code_p);

        tx_buffer = gtk_text_view_get_buffer(tx_text_view);
        gtk_text_buffer_set_text(tx_buffer, tx_msg_buffer, tx_msg_size);
    }

    /* Очищуємо рядок вводу */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_autoclean_check_button))) {
        gtk_entry_set_text(msg_entry, "");
        g_printf("Буфер очищено.\n");
    }

exit:
    free(tx_msg_buffer);
    free(buffer_p);

    return;
}

void ikb_changed_cb(GtkWidget *widget, gpointer window)
{
    g_active_ikb_id = gtk_combo_box_get_active(ikb_combo);
    g_printf("ІКВ змінено: %d \"%s\".\n", g_active_ikb_id, g_ikb_db[g_active_ikb_id].seq_str);
}

status_code_t ikb_codes_init()
{
    status_code_t status = SC_OK;
    uint8_t       ikb_4_1[] = {1, 2, 3, 7};
    uint8_t       ikb_4_2[] = {1, 1, 2, 3};
    uint8_t       ikb_6_3[] = {1, 1, 2, 1, 2, 4};
    uint8_t       ikb_8_4[] = {1, 1, 1, 2, 2, 1, 3, 4};

    g_ikb_db_size = 4;
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

exit:
    return status;
}

status_code_t gui_init()
{
    int i = 0;

    /* Створюємо та налаштовуємо головне вікно */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IKB Noise Protection");
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
    config_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(config_layout), 5);

    ikb_label = GTK_LABEL(gtk_label_new("IKB"));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(ikb_label));
    ikb_combo = GTK_COMBO_BOX(gtk_combo_box_text_new());
    for (i = 0; i < g_ikb_db_size; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ikb_combo), NULL, g_ikb_db[i].seq_str);
    }
    gtk_combo_box_set_active(ikb_combo, 0);
    gtk_widget_set_hexpand(GTK_WIDGET(ikb_combo), TRUE);
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(ikb_combo));

    config_separator1 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(config_separator1));

    noise_label = GTK_LABEL(gtk_label_new("Шум"));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(noise_label));
    noise_spin_button = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(0, 10, 1));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(noise_spin_button));

    config_separator2 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(config_separator2));

    cut_label = GTK_LABEL(gtk_label_new("Відрізання"));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(cut_label));
    cut_spin_button = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(0, 10, 1));
    gtk_container_add(GTK_CONTAINER(config_layout), GTK_WIDGET(cut_spin_button));

    gtk_container_add(GTK_CONTAINER(config_frame), GTK_WIDGET(config_layout));
    gtk_container_add(GTK_CONTAINER(window_layout), GTK_WIDGET(config_frame));

    /* Блок повідомлень */
    msg_send_frame = GTK_FRAME(gtk_frame_new("Повідомлення"));
    msg_send_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(msg_send_layout), 5);

    msg_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    msg_entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(msg_entry, 255);
    gtk_widget_set_hexpand(GTK_WIDGET(msg_entry), TRUE);
    gtk_container_add(GTK_CONTAINER(msg_layout), GTK_WIDGET(msg_entry));
    msg_autoclean_check_button = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Очистка"));
    gtk_container_add(GTK_CONTAINER(msg_send_layout), GTK_WIDGET(msg_layout));
    gtk_container_add(GTK_CONTAINER(msg_send_layout), GTK_WIDGET(msg_autoclean_check_button));

    msg_send_button = GTK_BUTTON(gtk_button_new_with_label("Надіслати"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(msg_send_button), "Надіслати повідомлення");
    gtk_container_add(GTK_CONTAINER(msg_send_layout), GTK_WIDGET(msg_send_button));

    gtk_container_add(GTK_CONTAINER(msg_send_frame), GTK_WIDGET(msg_send_layout));
    gtk_container_add(GTK_CONTAINER(window_layout), GTK_WIDGET(msg_send_frame));

    /* Панель надсилання та отримання даних */
    rx_tx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));

    tx_frame = GTK_FRAME(gtk_frame_new("TX"));
    tx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(tx_layout), 5);
    tx_text_view = GTK_TEXT_VIEW(gtk_text_view_new());

    gtk_text_view_set_editable(tx_text_view, FALSE);
    gtk_text_view_set_monospace(tx_text_view, TRUE);
    gtk_text_view_set_cursor_visible (tx_text_view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(tx_text_view), -1, 60);
    gtk_container_add(GTK_CONTAINER(tx_layout), GTK_WIDGET(tx_text_view));
    gtk_container_add(GTK_CONTAINER(tx_frame), GTK_WIDGET(tx_layout));
    gtk_container_add(GTK_CONTAINER(rx_tx_layout), GTK_WIDGET(tx_frame));

    rx_frame = GTK_FRAME(gtk_frame_new("RX"));
    rx_layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_container_set_border_width(GTK_CONTAINER(rx_layout), 5);
    rx_text_view = GTK_TEXT_VIEW(gtk_text_view_new());

    gtk_text_view_set_editable(rx_text_view, FALSE);
    gtk_text_view_set_monospace(rx_text_view, TRUE);
    gtk_text_view_set_cursor_visible (rx_text_view, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(rx_text_view), -1, 60);
    gtk_container_add(GTK_CONTAINER(rx_layout), GTK_WIDGET(rx_text_view));
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
    g_signal_connect(msg_send_button, "clicked", G_CALLBACK(send_msg_cb), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    return SC_OK;
}

status_code_t client_init()
{
    status_code_t status = SC_OK;

    /* Ініціалізуємо коди ІКВ */
    status = ikb_codes_init();
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація ІКВ завершилась помлкою: %s", status_get_str(status));
        goto exit;
    }

    /* Ініціалізуємо графічний інтерфейс */
    status = gui_init();
    if (CHECK_STATUS_FAIL(status)) {
        LOG_ERR("Ініціалізація графічної оболонки завершилась помилкою: %s", status_get_str(status));
        goto exit;
    }

exit:
    return status;
}

/* TODO: client de-init */

/* Основна функція програми */
int main(int argc, char *argv[])
{
    /* Ініціалізуємо GTK */
    gtk_init(&argc, &argv);

    /* Ініціалізація */
    client_init();

    /* Передаємо керування в GTK */
    gtk_main();

    /* Де-ініціалізація */

    return 0;
}