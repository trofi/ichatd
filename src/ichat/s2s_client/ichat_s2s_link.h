#ifndef __ICHAT_S2S_LINK_H__
#define __ICHAT_S2S_LINK_H__

struct timed_task;
struct s2s_block;
struct timed_task * make_s2s_link_task (long long delay, const struct s2s_block * b);

int start_ichat_s2s_link (const struct s2s_block * b);

#endif // __ICHAT_S2S_LINK_H__
