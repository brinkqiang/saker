#include "core_declarer.h"
#include <assert.h>
#include <fcntl.h>
/* umask */
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>

#include "common/types.h"
#include "config.h"
#include "utils/xstring.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/thread.h"
#include "utils/process.h"
#include "utils/xsds.h"
#include "saker.h"

static sds get_pidfile(const char *execfile) {
    sds   pidfile = sdsempty();
    char  buff[MAX_STRING_LEN] = {0};
    char *p, *q;
    strcat(buff, execfile);
    xstrlrtrim_spaces(buff);
    p = (p = strrchr(buff, ' ')) != NULL ? p+1 : buff ;
    if ((q = strrchr(p, '/')) != NULL) {
        q = q+1;
    } else if ((q=strrchr(p, '\\')) != NULL) {
        q = q+1;
    } else {
        q = p;
    }
    return sdscatprintf(pidfile, "%s/%s.pid" ,server.config->pidfile_dir, q);
}

/*
@params lua table
    cmd : string
    args : table [ string, string,...]
    pidfile: string
    umask : string(0222)
    user : string(root)
    stdout: string
    stderr: string
*/
int core_adopt(lua_State *L) {
    pid_t pid;
    int ret = 2;
    int status = 0;
    int idx = 0;
    int t_argc = 0;
    char **t_argv = NULL;
    sds cmd = NULL;
    sds pidfile = NULL;
    sds umask_str = NULL; 
    sds user = NULL;
    sds stdout_logfile = NULL;
    sds stderr_logfile = NULL;
    const char *k, *v;
    int top = lua_gettop(L);
    if (top == 0 || lua_type(L, 1) != LUA_TTABLE) {
        lua_pushnil(L);
        lua_pushstring(L, "wrong number of arguments for adopt");
        goto End;
    }

    lua_pushnil(L);
    while (lua_next(L, -2)) {
        k = lua_tostring(L, -2);
        if (k == NULL)
            continue;
        LOG_DEBUG("key %s", k);
        if (strcmp(k, "args") == 0) {
             if (lua_istable(L, -1)) {
                 t_argc = 0;
                 lua_pushnil(L);
                 while(lua_next(L,-2)) {
                     lua_pop(L,1); /* remove value, keep key for next iteration. */
                     ++t_argc;
                 }
                 t_argc += 2;
                 t_argv = (char **)zmalloc(sizeof(char *)*(t_argc)); /* last null */
                 idx = 1;
                 lua_pushnil(L);
                 while (lua_next(L, -2) != 0) {
                    /* value at -1, key is at -2 which we ignore */
                    const char *value = lua_tostring(L, -1);
                    LOG_DEBUG("args value %s", value);
                    t_argv[idx++] = zstrdup(value);
                    /* removes 'value'; keeps 'key' for next iteration */
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            continue;
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            continue;
        }
        v = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (v == NULL) {
            continue;
        }
        LOG_DEBUG("value %s", v);
        if (strcmp(k, "cmd") == 0) {
            cmd = sdsnew(v);
        } else if (strcmp(k, "umask") == 0) {
            umask_str = sdsnew(v);
        } else if (strcmp(k, "user") == 0) {
            user = sdsnew(v);
        } else if (strcmp(k, "stdout") == 0) {
            stdout_logfile = sdsnew(v);
        } else if (strcmp(k, "stderr") == 0) {
            stderr_logfile = sdsnew(v);
        } else if (strcmp(k, "pidfile") == 0) {
            pidfile = sdsnew(v);
        }
    }

    if (t_argv == NULL) {
        t_argc = 2;
        t_argv = (char **)zmalloc(sizeof(char *)*(t_argc));
    }

    t_argv[0] = zstrdup(cmd);
    t_argv[t_argc - 1] = NULL;

    if (pidfile == NULL) {
        pidfile = get_pidfile(cmd);
    }

    if (pidfile_verify(pidfile) == UGOK) {
        lua_pushnil(L);
        lua_pushfstring(L, "pidfile '%s' is exists", pidfile);
        goto End;
    }
    pid = fork();
    if (pid < 0 ) {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot fork process for  '%s'", t_argv[0]);
        goto End;
    } else if (pid == 0) {
        int fd;
        int maxFd = (int)sysconf(_SC_OPEN_MAX);
        unsigned int flags = 0;
        pid_t newpid = 0;
        int stdout_fd = STDOUT_FILENO, stderr_fd = STDERR_FILENO;
        daemonize(0);
        /* Close all file descriptors, except for standard input,
                  standard output, standard error    and listen fd
        */
        for(fd = 3; fd < maxFd; ++fd) {
            if (fd != server.ipfd) {
                close(fd);
            }
        }

        /* FD_CLOEXEC, the close-on-exec flag.  If the FD_CLOEXEC bit is 0, the
                   file descriptor will remain open across an execve(2), otherwise it will be closed.
         */
        flags = fcntl(server.ipfd, F_GETFD);
        flags |= 1; /* FD_CLOEXEC */
        if(fcntl(server.ipfd, F_SETFD, flags) == -1) {
            close(server.ipfd);
            server.ipfd = -1;
        }
        if (close(STDIN_FILENO) == -1 || open("/dev/null", O_RDWR) != STDIN_FILENO) {
            LOG_ERROR("Cannot reopen standard file descriptor (%d) -- \n", 0);
        }

        if (stdout_logfile) {
            stdout_fd = open(stdout_logfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (stdout_fd == -1) {
                LOG_ERROR("open file '%s' for stdout failed. err:'%s'", stdout_logfile, xerrmsg());
            }
        }
        if (stderr_logfile) {
            stderr_fd = open(stderr_logfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (stderr_fd == -1) {
                LOG_ERROR("open file '%s' for stderr failed. err:'%s'", stderr_logfile, xerrmsg());
            }
        }
        if (stdout_fd <= STDOUT_FILENO) {
            if (close(STDOUT_FILENO) == -1 || open("/dev/null", O_RDWR) != STDOUT_FILENO) {
                LOG_ERROR("Cannot reopen standard file descriptor (%d) -- \n", STDOUT_FILENO);
            }
        }        
        if (stderr_fd <= STDERR_FILENO) {
            if (close(STDERR_FILENO) == -1 || open("/dev/null", O_RDWR) != STDERR_FILENO) {
                LOG_ERROR("Cannot reopen standard file descriptor (%d) -- \n", STDERR_FILENO);
            }
        }
        
        if (stdout_fd > 1 && dup2(stdout_fd, STDOUT_FILENO) == -1) {
            LOG_ERROR("dup2 stdout to file '%s' failed. err:'%s'", stdout_logfile, xerrmsg());
        }        
        if (stderr_fd > 2 && dup2(stderr_fd, STDERR_FILENO) == -1) {
            LOG_ERROR("dup2 stderr to file '%s' failed. err:'%s'", stderr_logfile, xerrmsg());
        }
        newpid = getpid();
        if (pidfile_create(pidfile, cmd, newpid) == UGERR) {
            LOG_ERROR("cannot create pid for '%s' ,exit process. err: %s , mypid is '%d'", pidfile , xerrmsg(), newpid);
            _exit(1);
        }
        if (user != NULL) {
            struct passwd *pwd = getpwnam(user);
            if (pwd) {
                if (setuid(pwd->pw_uid) != 0) {
                    LOG_ERROR("cannot setuid for '%s'", cmd);
                    _exit(1);
                }
            } else {
                LOG_ERROR("cannot getpwnam for '%s'", user);
                _exit(1);
            }
        }
        
        LOG_TRACE("exec %s", cmd);
        execvp(cmd, (char * const *)t_argv);
        LOG_TRACE("exec process exit, mypid is '%d', err:'%s'.", newpid, xerrmsg());
        xfiledel(pidfile);
        close(stdout_fd);
        close(stderr_fd);
        _exit(72);
    }
    /* avoid <defunct> Wait blocking */
    waitpid(pid, &status, 0);
    xsleep(10);
    if (pidfile_verify(pidfile) == UGOK) {
        lua_pushnumber(L, pid);
        lua_pushstring(L, pidfile);
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "child exited for '%s' ,please cat log by 'mypid is '%d''", pidfile, pid);
    }
End:
    sdsfree(cmd);
    sdsfree(pidfile);
    sdsfree(umask_str);
    sdsfree(user);
    sdsfree(stdout_logfile);
    sdsfree(stderr_logfile);
    if(t_argv) {
        for (idx=0; t_argv[idx]; ++idx) {
            zfree(t_argv[idx]);
        }
        zfree((char **)t_argv);
    }
    return ret;
}

