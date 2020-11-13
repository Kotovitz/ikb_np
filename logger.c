#include <time.h>
#include <stdint.h>

#include "logger.h"


log_level_t global_log_level_g = LVL_NONE;

/**
 * \brief Configure global log level
 *
 * \param[in] new_level - new global logging level.
 *
 * \return status_code_t of operation result.
 */
status_code_t logger_set_glog_level(log_level_t new_level)
{
    if ((new_level < LVL_MIN) || (new_level > LVL_MAX)) {
        LOG_WRN("Invalid log level '%d' configured.", new_level);
    }

    global_log_level_g = new_level;

    return SC_OK;
}

/**
 * \brief Get global log level
 *
 * \param[in] existing_level - pointer to logging level variable.
 *
 * \return status_code_t of operation result.
 */
status_code_t logger_get_glog_level(log_level_t * existing_level)
{
    if (existing_level == NULL) {
        MSG_ERR("Level pointer is NULL");
        return SC_PARAM_NULL;
    }

    *existing_level = global_log_level_g;

    return SC_OK;
}

/**
 * \brief Returns the local date/time formatted as 2014-03-19 11:11:52
 *
 * \returns char * - String with formatted time
 */
char * logger_get_time_str(void)
{
    /* Must be static, otherwise won't work */
    static char _retval[20];
    time_t      rawtime;
    struct tm*  timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(_retval, sizeof(_retval), "%d.%H.%M.%S", timeinfo);

    return _retval;
}