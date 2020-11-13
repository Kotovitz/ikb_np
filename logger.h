#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <string.h>
#include <locale.h>

#include <glib/gprintf.h>

#include "status.h"


#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define FONT_NRM "\033[0m"
#define FONT_PNK "\033[1;35m"
#define FONT_RED "\033[0;31m"
#define FONT_CYA "\033[0;36m"
#define FONT_YLW "\033[1;33m"
#define FONT_GRY "\033[0;37m"
#define FONT_BLE "\033[0;34m"
#define FONT_GRN "\033[0;32m"

#define LOG_CLI(format, ...) g_printf(" [%s] %s:%-4d %-20s " format, logger_get_time_str(), __FILENAME__, __LINE__, __FUNCTION__, ## __VA_ARGS__)

#define CRT_HEADER "[" FONT_PNK "СТРАШНЕ"      FONT_NRM "]"
#define ERR_HEADER "[" FONT_RED "ПОМИЛКА"      FONT_NRM "]"
#define WRN_HEADER "[" FONT_CYA "ЗАСТЕРЕЖЕННЯ" FONT_NRM "]"
#define NTF_HEADER "[" FONT_YLW "Повідомлення" FONT_NRM "]"
#define INF_HEADER "[" FONT_GRY "Інформація"   FONT_NRM "]"
#define DBG_HEADER "[" FONT_BLE "налагодження" FONT_NRM "]"
#define SUC_HEADER "[" FONT_GRN "успіх"        FONT_NRM "]"

#define LOG_CLI_FILTER(level, format, ...)  if (LVL_##level <= global_log_level_g) {                     \
                                                LOG_CLI(level##_HEADER " " format ".\n", __VA_ARGS__); \
                                            }                                                           \

#define LOG_CRT(format, ...) LOG_CLI_FILTER(CRT, format, __VA_ARGS__)
#define LOG_ERR(format, ...) LOG_CLI_FILTER(ERR, format, __VA_ARGS__)
#define LOG_WRN(format, ...) LOG_CLI_FILTER(WRN, format, __VA_ARGS__)
#define LOG_NTF(format, ...) LOG_CLI_FILTER(NTF, format, __VA_ARGS__)
#define LOG_INF(format, ...) LOG_CLI_FILTER(INF, format, __VA_ARGS__)
#define LOG_DBG(format, ...) LOG_CLI_FILTER(DBG, format, __VA_ARGS__)
#define LOG_SUC(format, ...) LOG_CLI_FILTER(SUC, format, __VA_ARGS__)

#define MSG_CRT(text) LOG_CRT("%s", text)
#define MSG_ERR(text) LOG_ERR("%s", text)
#define MSG_WRN(text) LOG_WRN("%s", text)
#define MSG_NTF(text) LOG_NTF("%s", text)
#define MSG_INF(text) LOG_INF("%s", text)
#define MSG_DBG(text) LOG_DBG("%s", text)
#define MSG_SUC(text) LOG_SUC("%s", text)

/**
 * Logging levels enumaration
 */
typedef enum {
    LVL_NONE = 0,       /**< Logs are completely disabled */
    LVL_CRT,            /**< Only critical messages will be printed */
    LVL_ERR,            /**< Error and higher level messages will be printed */
    LVL_WRN,            /**< Warning and higher level messages will be printed */
    LVL_NTF,            /**< Notification and higher level messages will be printed */
    LVL_INF,            /**< Information and higher level messages will be printed */
    LVL_DBG,            /**< Debug and higher level messages will be printed */
    LVL_SUC,            /**< Successand higher level messages will be printed */
    LVL_ALL,            /**< All logs will be printed */
    LVL_MIN = LVL_CRT,  /**< Minimal log level - important logs are printed */
    LVL_MAX = LVL_SUC,  /**< Maximum log level - most of logs are printed */
} log_level_t;

extern log_level_t global_log_level_g;

/**
 * \brief Configure global log level
 *
 * \param[in] new_level - new global logging level.
 *
 * \return status_code_t of operation result.
 */
status_code_t logger_set_glog_level(log_level_t new_level);

/**
 * \brief Get global log level
 *
 * \param[in] existing_level - pointer to logging level variable.
 *
 * \return status_code_t of operation result.
 */
status_code_t logger_get_glog_level(log_level_t * existing_level);

/**
 * \brief Returns the local date/time formatted as 2014-03-19 11:11:52
 *
 * \returns char * - String with formatted time
 */
char * logger_get_time_str(void);


#endif /* LOGGER_H */