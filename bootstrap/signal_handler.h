#ifndef GUAGE_SIGNAL_HANDLER_H
#define GUAGE_SIGNAL_HANDLER_H

#include <stdbool.h>
#include <signal.h>

/* Max registered signal handlers */
#define SIGNAL_MAX_HANDLERS 32

/* Signal→actor mapping */
typedef struct {
    int signum;
    int actor_id;
    bool active;
} SignalRegistration;

/* Initialize self-pipe signal infrastructure */
void signal_init(void);

/* Shutdown signal subsystem */
void signal_shutdown(void);

/* Register actor to receive POSIX signal as message */
bool signal_register(int signum, int actor_id);

/* Unregister signal handler */
bool signal_unregister(int signum);

/* Poll for pending signals, dispatch as actor messages.
 * Called from scheduler idle loop — zero overhead when no signals pending. */
void signal_poll(void);

/* Get list of registered handlers (for ⚡?) */
int signal_list_handlers(SignalRegistration* out, int max);

/* Map signal name to number: "sigusr1" → SIGUSR1 */
int signal_name_to_num(const char* name);

/* Map signal number to name: SIGUSR1 → "sigusr1" */
const char* signal_num_to_name(int signum);

#endif /* GUAGE_SIGNAL_HANDLER_H */
