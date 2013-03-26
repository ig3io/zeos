#include <stats.h>
#include <utils.h>

void stats_update_user_to_system(struct stats * st)
{
  st->user_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_system_to_user(struct stats * st)
{
  st->system_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_system_to_ready(struct stats * st)
{
  st->system_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_ready_to_system(struct stats * st)
{
  st->ready_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_init(struct stats * st)
{
  st->user_ticks = 0;
  st->system_ticks = 0;
  st->blocked_ticks = 0;
  st->ready_ticks = 0;
  st->elapsed_total_ticks = get_ticks();
  st->total_trans = 0;
  st->remaining_ticks = 0;
}
