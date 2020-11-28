#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <pthread.h>

#include "common.h"
#include "logger.h"
#include "status.h"

#include "channel.h"


/* Global channel object */
channel_t channel_g;


void * __channel_polling(void *t_arg)
{
    channel_t       *channel_p = (channel_t *)t_arg;
    client_header_t *client_p = CLIENT_HEADER(channel_p->stream, channel_p->client_id);
    uint8_t         *stream_p = NULL;

    if (client_p->client_id != channel_p->client_id) {
        perror("Polling failed. Client ID mismatch.\n");
        return NULL;
    }

    /* Start income messages polling */
    while (channel_p->polling) {
        if (client_p->mode == CM_READY) {
            /* There is a message in the buffer */
            client_p->mode = CM_READING;

            /* Call user reader callback */
            stream_p = (uint8_t *)client_p + sizeof(client_header_t);
            channel_p->reader_cb(client_p->buffer_size, stream_p);

            /* Free income buffer for writing */
            client_p->buffer_size = 0;
            client_p->mode = CM_EMPTY;
        }
    }

    pthread_exit(NULL);

    return NULL;
}


status_code_t channel_init(uint16_t client_id, uint16_t dst_client_id,
                           uint8_t shmem_path_len, char *shmem_path,
                           reader_func_t reader_cb)
{
    status_code_t     status = SC_OK;
    int               res = 0, fd = 0;
    channel_header_t *ch_header_p = NULL;
    client_header_t  *client_p = NULL;


    /* Parameters validation */
    if ((shmem_path_len == 0) || (shmem_path == NULL)) {
        status = SC_PARAM_NULL;
        g_printf("Shared memory size is invalid or path is NULL.\n");
        goto exit;
    }

    if (reader_cb == NULL) {
        status = SC_PARAM_NULL;
        g_printf("Reader function callback is NULL.\n");
        goto exit;
    }

    /* TODO: Check if client ID is not too big. May cause an SEGFAULT */

    /* Initialize channel metadata */
    channel_g.shmem_size = CHANNEL_SIZE;
    channel_g.reader_cb = reader_cb;
	channel_g.pid = getpid();
    channel_g.client_id = client_id;
    channel_g.dst_client_id = dst_client_id;

    /* Copy shared memory path */
    channel_g.shmem_path = calloc(1, shmem_path_len + 1);
    strncpy(channel_g.shmem_path, shmem_path, shmem_path_len);
    channel_g.shmem_path[shmem_path_len] = '\0';

	/* Get shared memory file descriptor (NOT a file) */
	fd = shm_open(channel_g.shmem_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("Не вдалось відкрити спільну пам'ять\n");
		status = SC_UNKNOWN_ERROR;
        goto exit;
	}

	/* Extend shared memory object as by default it's initialized with size 0 */
	res = ftruncate(fd, channel_g.shmem_size);
	if (res == -1) {
		perror("Не вдалось змінити розмір спільної пам'яті\n");
        status = SC_UNKNOWN_ERROR;
        goto exit;
	}

	/* Map shared memory to process address space */
	channel_g.stream = (uint8_t *)mmap(NULL, channel_g.shmem_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if (channel_g.stream == MAP_FAILED) {
		perror("Помилка прив'язки спільної пам'яті\n");
		status = SC_UNKNOWN_ERROR;
        goto exit;
	}

    /* Update channel header */
    g_printf("stream_p = %p\n", channel_g.stream);
    ch_header_p = (channel_header_t *)channel_g.stream;
    g_printf("ch_header_p = %p\n", ch_header_p);
    ch_header_p->clients_attached++;


    /* Initialize client header */
    client_p = CLIENT_HEADER(ch_header_p, channel_g.client_id);
    g_printf("client_p = %p\n", client_p);
    client_p->client_id = channel_g.client_id;
    client_p->mode = CM_EMPTY;
    client_p->buffer_size = 0;

    /* Start polling thread */
    channel_g.polling = TRUE;
    pthread_create(&channel_g.polling_thread, NULL, __channel_polling, &channel_g);

exit:
    return status;
}


status_code_t channel_deinit()
{
    status_code_t     status = SC_OK;
    int               res = 0, fd = 0;
    channel_header_t *ch_header_p = NULL;
    client_header_t  *client_p = NULL;

    /* Stop polling thread */
    channel_g.polling = FALSE;
    pthread_join(channel_g.polling_thread, NULL);

    /* Update channel header */
    ch_header_p = (channel_header_t *)channel_g.stream;
    ch_header_p->clients_attached--;

    /* Deinitialize client header */
    client_p = CLIENT_HEADER(ch_header_p, channel_g.client_id);
    client_p->client_id = 0;
    client_p->mode = CM_INVALID;
    client_p->buffer_size = 0;


	/* Mmap cleanup */
	res = munmap(channel_g.stream, channel_g.shmem_size);
	if (res == -1) {
		perror("Помилка відв'язування спільної пам'яті\n");
		status = SC_UNKNOWN_ERROR;
        goto exit;
	}

	/* shm_open cleanup */
	fd = shm_unlink(channel_g.shmem_path);
	if (fd == -1) {
		perror("Помилка завершення зв'язку із спільною пам'яттю\n");
		status = SC_UNKNOWN_ERROR;
        goto exit;
	}

    /* De-initialize channel metadata */
    channel_g.shmem_size = 0;
    channel_g.reader_cb = NULL;
	channel_g.pid = 0;
    channel_g.client_id = 0;

    free(channel_g.shmem_path);

exit:
    return status;
}


status_code_t channel_write(int8_t dst_client_id, uint8_t buffer_size, uint8_t *buffer_p)
{
    status_code_t     status = SC_OK;
    channel_header_t *ch_header_p = NULL;
    client_header_t  *dst_client_p = NULL;
    uint8_t          *stream_p = NULL;
    uint8_t           send_attempt = 0;

    /* Validate parameters */
    if ((buffer_size == 0) || (buffer_p == NULL)) {
        perror("Помилка запису в канал. Розмір буфера 0 чи буфер не існує.\n");
        status = SC_PARAM_NULL;
        goto exit;
    }

    if (dst_client_id == -1) {
        /* Використання отримувача за умовчанням */
        dst_client_id = channel_g.dst_client_id;
    }

    /* Access required buffer */
    g_printf("stream_p = %p\n", channel_g.stream);
    ch_header_p = (channel_header_t *)channel_g.stream;
    g_printf("ch_header_p = %p\n", ch_header_p);
    dst_client_p = CLIENT_HEADER(ch_header_p, dst_client_id);
    g_printf("dst_client_p = %p\n", dst_client_p);
    stream_p = (uint8_t *)dst_client_p + sizeof(client_header_t);
    g_printf("stream_p = %p\n", stream_p);

    if (dst_client_p->mode == CM_INVALID) {
        perror("Помилка запису в канал. Отримувач не ініціалізований.\n");
        status = SC_UNKNOWN_ERROR;
        goto exit;
    }

	/* Place data into the memory in 10 attempts */
    for (send_attempt = 0; send_attempt < 10; send_attempt++) {
        if (dst_client_p->mode == CM_EMPTY) {
            /* Write to the buffer */
            dst_client_p->mode = CM_WRITING;
            memcpy(stream_p, buffer_p, buffer_size);
            dst_client_p->buffer_size = buffer_size;

            dst_client_p->mode = CM_READY;
            break;
        }

        /* Sleep 100 miliseconds */
        usleep(100000);
    }

exit:
    return status;
}

