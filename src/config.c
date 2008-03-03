#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"

struct config *
config_alloc (void)
{
    struct config * config = (struct config *)malloc (sizeof (struct config));
    if (!config)
        goto e_no_mem;
    memset (config, 0, sizeof (struct config));

    config->foreground_mode = 0;
    config->user_port       = DEF_USER_PORT;
    config->ctl_port        = DEF_CONTROL_PORT;
    config->s2s_port        = DEF_S2S_PORT;

    if (!(config->s2s_password  = (char *)strdup ((const char *)DEF_SERVER_PASSWORD)))
        goto e_no_mem;
    
    if (!(config->log_file  = (char *)strdup ((const char *)DEF_LOG_FILE)))
        goto e_no_mem;
    config->log_level       = NOTE_LEVEL;
    if (!(config->pid_file  = strdup (DEF_PID_FILE)))
        goto e_no_mem;
    config->null_clients    = 1;
    config->max_msg_size    = 8192;

    return config;

  e_no_mem:
    config_destroy (config);
    return 0;
}

void
config_destroy (struct config * config)
{
    if (!config)
        return;

    free (config->log_file);
    free (config->pid_file);
    free (config->s2s_password);
    free (config);
}

enum CONFIG_STATUS
parse_config (struct config * config, const char * fname)
{
    DEBUG (__func__);
    assert (config);
    assert (fname);
    //FIXME: config currently unimplemented
    WARN ("I'm unimplemented. Please config me via config.h :]");
    return CONFIG_OK;
}
