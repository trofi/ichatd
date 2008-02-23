#ifndef __ICC2S_H__
#define __ICC2S_H__

struct icc2s;
struct buffer;

struct icc2s * ichat_buffer_to_icc2s (struct buffer * msg);
void icc2s_unref (struct icc2s * c2s_msg);

// here can be on-demand init (currently uniplemented)
struct buffer * icc2s_sender (struct icc2s * c2s_msg);
struct buffer * icc2s_command (struct icc2s * c2s_msg);
struct buffer * icc2s_receiver (struct icc2s * c2s_msg);
struct buffer * icc2s_data (struct icc2s * c2s_msg);

#endif // __ICC2S_H__
