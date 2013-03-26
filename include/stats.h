#ifndef STATS_H
#define STATS_H

/* Structure used by 'get_stats' function */
struct stats
{
  unsigned long user_ticks;
  unsigned long system_ticks;
  unsigned long blocked_ticks;
  unsigned long ready_ticks;
  unsigned long elapsed_total_ticks;
  /* Number of times the process has got the CPU: READY->RUN transitions */
  unsigned long total_trans; 
  unsigned long remaining_ticks;
};

void stats_update_user_to_system(struct stats * st);
void stats_update_system_to_user(struct stats * st);
void stats_update_system_to_ready(struct stats * st);
void stats_update_ready_to_system(struct stats * st);

void stats_init(struct stats * st);

#endif /* !STATS_H */
