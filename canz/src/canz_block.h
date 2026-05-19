#ifndef CANZ_BLOCK_H
#define CANZ_BLOCK_H

#include "canz_internal.h"

int canz_block_flush(FILE                    *out,
                     const canz_stream_msg_t *msgs,
                     int                      count,
                     const canz_schema_t     *schema,
                     int                      comp_level,
                     size_t                  *bytes_out);


int canz_write_file_header(FILE *out, const canz_schema_t *schema);

#endif /* CANZ_BLOCK_H */
