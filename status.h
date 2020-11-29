#ifndef STATUS_H
#define STATUS_H

#define CHECK_STATUS_FAIL(status) (status != SC_OK)

#define STATUS_CHECK_OK(status, error_msg) \
    {                                      \
        if (CHECK_STATUS_FAIL(status)) {   \
            MSG_ERR(error_msg);            \
            return status;                 \
        }                                  \
    }

/** Status codes of operation */
typedef enum status_code {
    SC_OK = 0,                          /**< Operation has been done successfully */
    SC_UNKNOWN_ERROR,                   /**< Operation has failed for unknown reason */
    SC_FLOW_UNSUPPORTED,                /**< Operation has failed because flow is unsupported */
    SC_FLOW_DEPRECATED,                 /**< Operation has failed because flow is deprecated */
    SC_FLOW_FAILED,                     /**< Operation has failed because flow cannot be executed */
    SC_MODULE_UNINITIALIZED,            /**< Operation has failed because module is uninitialized */
    SC_MODULE_ALREADY_INITIALIZED,      /**< Operation has failed because module is already initialized */
    SC_PARAM_NULL,                      /**< Operation has failed because parameter is NULL pointer */
    SC_PARAM_VALUE_INVALID,             /**< Operation has failed because parameter value is invalid */
    SC_OBJECT_EXIST,                    /**< Operation has failed because object exist */
    SC_OBJECT_NOT_EXIST,                /**< Operation has failed because object not exist */
    SC_OBJECT_EQUAL,                    /**< Operation has failed because object is equal to another by some property */
    SC_OBJECT_NOT_EQUAL,                /**< Operation has failed because object is not equal to another by some property */
    SC_OBJECT_NOT_FOUND,                /**< Operation has failed because object was not found */
    SC_OBJECT_INITIALIZED,              /**< Operation has failed because object is already initialized */
    SC_OBJECT_UNINITIALIZED,            /**< Operation has failed because object is not initialized */
    SC_OBJECT_REFERENCED,               /**< Operation has failed because object used by other objects */
    SC_OBJECT_UNREFERENCED,             /**< Operation has failed because object is not used by other objects */
    SC_OBJECT_NOT_EMPTY,                /**< Operation has failed because object contain other objects/references */
    SC_OBJECT_EMPTY,                    /**< Operation has failed because object doesn`t contain other objects/references */
    SC_NO_FREE_MEMORY,                  /**< Operation has failed because function cannot allocate memory */

    SC_MIN = SC_OK,
    SC_MAX = SC_NO_FREE_MEMORY,
} status_code_t;

char * status_get_str(status_code_t status);


#endif /* STATUS_H */