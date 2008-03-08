#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "log.h"

static struct s2s_block *
s2s_block_alloc()
{
    struct s2s_block * s2s_b = (struct s2s_block *)malloc (sizeof (struct s2s_block));
    if (!s2s_b)
        return 0;
    memset (s2s_b, 0, sizeof (struct s2s_block));
    return s2s_b;
}

void
s2s_block_destroy (struct s2s_block * s2s_b)
{
    if (!s2s_b)
        return;

    free (s2s_b->host);
    free (s2s_b->pass);
    free (s2s_b);
}

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


    if (!(config->server_name  = (char *)strdup ((const char *)DEF_SERVER_NAME)))
        goto e_no_mem;
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
    free (config->server_name);
    free (config->s2s_password);
    struct s2s_block * b = config->s2s_queue;
    struct s2s_block * b_next = 0;
    while (b)
    {
        b_next = b->next;
        s2s_block_destroy (b);
        b = b_next;
    }
    free (config);
}

static int is_port (long port)
{
    if (port == -1 // closed
        || (port > 0 && port < 65536)) // 16 bit positive value
        return 1;
    return 0;
}

enum BOOL_PARSE_RESULT {
    BOOL_ERROR,
    BOOL_FALSE,
    BOOL_TRUE,
};

static enum BOOL_PARSE_RESULT
parse_bool (const char * value)
{
    if (strcmp (value, "no") == 0
        || strcmp (value, "0") == 0)
        return BOOL_FALSE;
    if (strcmp (value, "yes") == 0
        || strcmp (value, "1") == 0)
        return BOOL_TRUE;
    return BOOL_ERROR;
}

// parses ip port pass -> structure or NULL
static struct s2s_block *
parse_s2s (char * value)
{
    struct s2s_block * s2s_b = s2s_block_alloc();
    if (!s2s_b)
        goto e_no_mem;

    char * ip = value;
    char * ip_end = ip;
    while (*ip_end && *ip_end != ' ' && *ip_end != '\t') ++ip_end;
    if (*ip_end == '\0')
        goto e_bad_value;

    *ip_end = '\0';
    char * port = ip_end + 1;
    while (*port == ' ' && *port == '\t') ++port;
    if (*port == '\0')
        goto  e_bad_value;
    char * port_end = port;
    while (*port_end && *port_end != ' ' && *port_end != '\t') ++port_end;
    if (*port_end == '\0')
        goto e_bad_value;

    *port_end = '\0';
    char * pass = port_end + 1;
    while (*pass == ' ' && *pass == '\t') ++pass;

    // store parsed values

    int port_num = strtol (port, NULL, 10);
    if (!(is_port(port_num)))
        goto e_bad_value;

    s2s_b->port = port_num;
    if (!(s2s_b->host = strdup (ip)))
        goto e_bad_value;
    if (!(s2s_b->pass = strdup (pass)))
        goto e_bad_value;

    return s2s_b;

  e_bad_value:
    s2s_block_destroy (s2s_b);
  e_no_mem:
    return 0;
}

enum OPT_PARSE_RESULT {
    OPT_PARSED,
    OPT_UNKNOWN,
    OPT_BAD_VAL,
};

static enum OPT_PARSE_RESULT
parse_option (struct config * config, const char * name, char * value)
{
    assert (config);
    assert (name);
    assert (value);
#define OPT_IS_A(opt) (strcmp (name, opt) == 0)
#define VAL_IS_A(opt) (strcmp (value, opt) == 0)

    // TODO: remove bruteforce comparison (migrate to tree?)
    if (OPT_IS_A("port")) // port <num>
    {
        int port = strtol (value, NULL, 10);
        if (is_port (port))
        {
            config->user_port = port;
            return OPT_PARSED;
        }
        return OPT_BAD_VAL;
    }

    if (OPT_IS_A("serverport")) // serverport <num>
    {
        int port = strtol (value, NULL, 10);
        if (is_port (port))
        {
            config->s2s_port = port;
            return OPT_PARSED;
        }
        return OPT_BAD_VAL;
    }

    if (OPT_IS_A("controlport")) // controlport <num>
    {
        int port = strtol (value, NULL, 10);
        if (is_port (port))
        {
            config->ctl_port = port;
            return OPT_PARSED;
        }
        return OPT_BAD_VAL;
    }

    if (OPT_IS_A("servername")) // servername [name]
    {
        free (config->server_name);
        config->server_name = strdup (value);
        return OPT_PARSED;
    }

    if (OPT_IS_A("serverpassword")) // serverpassword [pass]
    {
        free (config->s2s_password);
        config->s2s_password = strdup (value);
        return OPT_PARSED;
    }

    if (OPT_IS_A("logfile")) // logfile path
    {
        // TODO: check path writeability
        free (config->log_file);
        config->log_file = strdup (value);
        return OPT_PARSED;
    }

    if (OPT_IS_A("pidfile")) // pidfile path
    {
        // TODO: check path writeability
        free (config->pid_file);
        config->pid_file = strdup (value);
        return OPT_PARSED;
    }

    if (OPT_IS_A("loglevel")) // loglevel { fatal | warn | note | debug }
    {
        if (VAL_IS_A("fatal"))
        {
            config->log_level = FATAL_LEVEL;
            return OPT_PARSED;
        }
        if (VAL_IS_A("warn"))
        {
            config->log_level = WARN_LEVEL;
            return OPT_PARSED;
        }
        if (VAL_IS_A("note"))
        {
            config->log_level = NOTE_LEVEL;
            return OPT_PARSED;
        }
        if (VAL_IS_A("debug"))
        {
            config->log_level = DEBUG_LEVEL;
            return OPT_PARSED;
        }
        return OPT_BAD_VAL;
    }

    if (OPT_IS_A("nullclients")) // nullclients { 0 | 1 }
    {
        switch (parse_bool (value))
        {
            case BOOL_TRUE:
                config->null_clients = 1;
                return OPT_PARSED;
            case BOOL_FALSE:
                config->null_clients = 0;
                return OPT_PARSED;
            case BOOL_ERROR:
                return OPT_BAD_VAL;
        }
    }

    if (OPT_IS_A("server")) // server ip port [pass]
    {
        struct s2s_block * b = parse_s2s (value);
        if (b)
        {
            b->next = config->s2s_queue;
            config->s2s_queue = b;
            return OPT_PARSED;
        }
        return OPT_BAD_VAL;
    }
#undef VAL_IS_A
#undef OPT_IS_A
    return OPT_UNKNOWN;
}

enum CONFIG_STATUS
parse_config (struct config * config, const char * fname)
{
    assert (config);
    assert (fname);

    FILE * conf = fopen (fname, "r");
    if (!conf)
        goto bad_file;

    int line_no = 0;
    char line[1024];
    // for each line
    while (fgets (line, sizeof (line), conf))
    {
        ++line_no;
        // zero trailing \r\n's
        char * end = line + strlen (line) - 1;
        while (end > line && (*end == '\n' || *end == '\r')) *(end--) = '\0';

        char * start = line;
        // skip spaces
        while (*start == ' ' || *start == '\t') ++start;
        // blank line or comment
        if (*start == '\0' || *start == '#') continue;

        char * opt_name = start;

        char * opt_value = opt_name;
        // skip first word
        while (*opt_value && *opt_value != ' ' && *opt_value != '\t') ++opt_value;

        // separate name from value by '\0'
        if (*opt_value) *(opt_value++) = '\0';

        // skip spaces
        while (*opt_value && (*opt_value == ' ' || *opt_value == '\t')) ++opt_value;

        switch (parse_option (config, opt_name, opt_value))
        {
            case OPT_BAD_VAL:
                WARN ("%d: option %s has bad value", line_no, opt_name);
                break;
            case OPT_UNKNOWN:
                WARN ("%d: unknown option %s", line_no, opt_name);
                break;
            case OPT_PARSED:
                // all ok
                break;
        }
    }
    fclose (conf);
    return CONFIG_OK;

  bad_file:
    return CONFIG_FILE_NOT_FOUND;
}
