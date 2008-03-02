#ifndef __ICS2S_H__
#define __ICS2S_H__

struct ics2s;
struct buffer;

struct ics2s * ichat_buffer_to_ics2s (struct buffer * msg);
void ics2s_unref (struct ics2s * s2s_msg);

// here can be on-demand init (currently uniplemented)
struct buffer * ics2s_sender (struct ics2s * s2s_msg);
struct buffer * ics2s_timestamp (struct ics2s * s2s_msg);
struct buffer * ics2s_command (struct ics2s * s2s_msg);
struct buffer * ics2s_data (struct ics2s * s2s_msg);

// some buffer making stuff
struct buffer * s2s_make_login_msg (const char * server_name, const char * password);

#endif // __ICS2S_H__
