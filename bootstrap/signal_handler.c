#include "signal_handler.h"
#include "cell.h"
#include "actor.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

/* Self-pipe for async-signal-safe signal delivery */
static int signal_pipe[2] = {-1, -1};

/* Registration table */
static SignalRegistration registrations[SIGNAL_MAX_HANDLERS] = {{0}};

/* Previous signal actions for restoration */
static struct sigaction prev_actions[SIGNAL_MAX_HANDLERS] = {{0}};

/* Async-signal-safe handler — writes signal number to pipe */
static void signal_handler(int signum) {
    uint8_t sig = (uint8_t)signum;
    /* write() is async-signal-safe per POSIX */
    (void)write(signal_pipe[1], &sig, 1);
}

void signal_init(void) {
    if (signal_pipe[0] >= 0) return; /* Already initialized */

    if (pipe(signal_pipe) < 0) return;

    /* Set non-blocking on both ends */
    int flags0 = fcntl(signal_pipe[0], F_GETFL, 0);
    fcntl(signal_pipe[0], F_SETFL, flags0 | O_NONBLOCK);
    int flags1 = fcntl(signal_pipe[1], F_GETFL, 0);
    fcntl(signal_pipe[1], F_SETFL, flags1 | O_NONBLOCK);

    /* Set close-on-exec */
    fcntl(signal_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(signal_pipe[1], F_SETFD, FD_CLOEXEC);
}

void signal_shutdown(void) {
    /* Restore all previous handlers */
    for (int i = 0; i < SIGNAL_MAX_HANDLERS; i++) {
        if (registrations[i].active) {
            sigaction(registrations[i].signum, &prev_actions[i], NULL);
            registrations[i].active = false;
        }
    }

    if (signal_pipe[0] >= 0) { close(signal_pipe[0]); signal_pipe[0] = -1; }
    if (signal_pipe[1] >= 0) { close(signal_pipe[1]); signal_pipe[1] = -1; }
}

bool signal_register(int signum, int actor_id) {
    /* Find free slot or existing registration for this signal */
    int slot = -1;
    for (int i = 0; i < SIGNAL_MAX_HANDLERS; i++) {
        if (registrations[i].active && registrations[i].signum == signum) {
            /* Update existing */
            registrations[i].actor_id = actor_id;
            return true;
        }
        if (!registrations[i].active && slot < 0) {
            slot = i;
        }
    }

    if (slot < 0) return false; /* No free slots */

    /* Install signal handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(signum, &sa, &prev_actions[slot]) < 0) return false;

    registrations[slot].signum = signum;
    registrations[slot].actor_id = actor_id;
    registrations[slot].active = true;
    return true;
}

bool signal_unregister(int signum) {
    for (int i = 0; i < SIGNAL_MAX_HANDLERS; i++) {
        if (registrations[i].active && registrations[i].signum == signum) {
            sigaction(signum, &prev_actions[i], NULL);
            registrations[i].active = false;
            return true;
        }
    }
    return false;
}

void signal_poll(void) {
    if (signal_pipe[0] < 0) return;

    uint8_t buf[64];
    ssize_t n;
    while ((n = read(signal_pipe[0], buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            int signum = buf[i];
            /* Find registered actor for this signal */
            for (int j = 0; j < SIGNAL_MAX_HANDLERS; j++) {
                if (registrations[j].active && registrations[j].signum == signum) {
                    /* Send (:signal :signame) message to actor */
                    const char* name = signal_num_to_name(signum);
                    Cell* msg = cell_cons(
                        cell_symbol("signal"),
                        cell_cons(cell_symbol(name ? name : "unknown"), cell_nil())
                    );
                    Actor* actor = actor_lookup(registrations[j].actor_id);
                    if (actor) actor_send(actor, msg);
                    break;
                }
            }
        }
    }
}

int signal_list_handlers(SignalRegistration* out, int max) {
    int count = 0;
    for (int i = 0; i < SIGNAL_MAX_HANDLERS && count < max; i++) {
        if (registrations[i].active) {
            out[count++] = registrations[i];
        }
    }
    return count;
}

int signal_name_to_num(const char* name) {
    if (!name) return -1;
    if (name[0] == ':') name++; /* Skip Guage symbol prefix */

    if (strcasecmp(name, "sighup") == 0)    return SIGHUP;
    if (strcasecmp(name, "sigint") == 0)     return SIGINT;
    if (strcasecmp(name, "sigquit") == 0)    return SIGQUIT;
    if (strcasecmp(name, "sigterm") == 0)    return SIGTERM;
    if (strcasecmp(name, "sigusr1") == 0)    return SIGUSR1;
    if (strcasecmp(name, "sigusr2") == 0)    return SIGUSR2;
    if (strcasecmp(name, "sigchld") == 0)    return SIGCHLD;
    if (strcasecmp(name, "sigalrm") == 0)    return SIGALRM;
    if (strcasecmp(name, "sigpipe") == 0)    return SIGPIPE;
    if (strcasecmp(name, "sigwinch") == 0)   return SIGWINCH;
    if (strcasecmp(name, "sigcont") == 0)    return SIGCONT;
    if (strcasecmp(name, "sigtstp") == 0)    return SIGTSTP;
    if (strcasecmp(name, "sigttin") == 0)    return SIGTTIN;
    if (strcasecmp(name, "sigttou") == 0)    return SIGTTOU;
    return -1;
}

const char* signal_num_to_name(int signum) {
    switch (signum) {
        case SIGHUP:    return "sighup";
        case SIGINT:    return "sigint";
        case SIGQUIT:   return "sigquit";
        case SIGTERM:   return "sigterm";
        case SIGUSR1:   return "sigusr1";
        case SIGUSR2:   return "sigusr2";
        case SIGCHLD:   return "sigchld";
        case SIGALRM:   return "sigalrm";
        case SIGPIPE:   return "sigpipe";
        case SIGWINCH:  return "sigwinch";
        case SIGCONT:   return "sigcont";
        case SIGTSTP:   return "sigtstp";
        case SIGTTIN:   return "sigttin";
        case SIGTTOU:   return "sigttou";
        default:        return NULL;
    }
}
