#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdint.h>
#include <unistd.h>

#include "common.h"
#include "status.h"
#include "logger.h"

/*
 * Channel communication mechanism
 * ----------------------------------------------------------
 *
 * Channel - is an shared memory used for data transmission
 * between two clients.
 *
 * Channel has its own communication protocol. More details
 * below.
 *
 * ----------------------------------------------------------
 *
 * Shared memory has the following structure:
 *  - CH header - contains general information about the
 *                channel.
 *  - C0 header - Some information about client #0.
 *  - Line-0    - buffer used for sending data to the
 *                client #0.
 *  - C2 header - Some information about client #1.
 *  - Line-1    - buffer used for sending data to the
 *                client #1.
 * ----------------------------------------------------------
 */


#define CHANNEL_ID "/physical_channel"

#define LINE_SIZE       (1*1024) /* 1K bytes */
#define MAX_CLIENTS_NUM 2         /* Maximum number of the clients */
#define CHANNEL_SIZE    (sizeof(channel_header_t) + (sizeof(client_header_t) + LINE_SIZE) * MAX_CLIENTS_NUM)

// 4 + (10 + 1024) * 2 = 4 + 1034 * 2 = 4 + 2068 = 2072

#define CLIENT_HEADER(channel_p, client_id) \
    (client_header_t *)((uint8_t *)channel_p + sizeof(channel_header_t) + (sizeof(client_header_t) + LINE_SIZE) * client_id)


typedef enum {
    CM_INVALID = 0, /* Client is not initialized here */
    CM_EMPTY,       /* Buffer may be used for writing */
    CM_READING,     /* Client is reading data from the buffer */
    CM_WRITING,     /* Someone is writing to the buffer */
    CM_READY        /* Buffer is ready for reading data */
} client_mode_t;

/* General information about channel */
typedef struct channel_header {
    uint32_t clients_attached; /* Used to count clients attached to the channel */
} channel_header_t;

/* Client-specific data */
typedef struct client_header {
    uint16_t      client_id;    /* Client ID */
    uint16_t      buffer_size;  /* Size of valid data in the buffer */
    client_mode_t mode;         /* Buffer status mode */
} client_header_t;

/* Buffer reader callback */
typedef void (*reader_func_t)(uint16_t buffer_size, uint8_t *buffer);

/* Channel object used for data transmission */
typedef struct channel {
    pid_t          pid;
    uint16_t       client_id;
    uint16_t       dst_client_id;
    uint32_t       shmem_size;
    char          *shmem_path;
    bool_t         polling;
    pthread_t      polling_thread;
    reader_func_t  reader_cb;
    uint8_t       *stream;
} channel_t;


/* Functions prototypes */
status_code_t channel_init(uint16_t client_id, uint16_t dst_client_id,
                           uint8_t shmem_path_len, char *shmem_path,
                           reader_func_t reader_cb);
status_code_t channel_deinit();
status_code_t channel_write(int8_t dst_client_id, uint8_t buffer_size, uint8_t *buffer_p);


#endif /* _CHANNEL_H_ */