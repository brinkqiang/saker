#include "luaworking.h"
#include <unistd.h>
#include "common/common.h"
#include "utils/ulist.h"
#include "utils/path.h"
#include "utils/file.h"
#include "utils/xsds.h"
#include "utils/logger.h"
#include "utils/debug.h"

static void luaworkSetError(char *err, const char *fmt, ...) {
    va_list ap;

    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, LUAWORK_ERR_LEN, fmt, ap);
    va_end(ap);
}

lua_State *luaworkOpen(void) {
    lua_State *L = L = luaL_newstate();
    luaL_openlibs(L);
    return L;
}

void luaworkClose( lua_State *L ) {
    if (L) {
        lua_close(L);
    }
}

void luaworkSetEnv(lua_State *L, const char *key, const char *value) {
    lua_getglobal(L, key);
    ugAssert(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_pushstring(L, value);
    lua_setglobal(L, key);
}

const char *luaworkGetEnv(lua_State *L, const char *key) {
    const char *result;
    lua_getglobal(L, key);
    result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
}

int luaworkDoString( lua_State *L,const char *code,  char *err ) {
    if (0 == luaL_dostring(L,code)) {
        return UGOK;
    }
    luaworkSetError(err, "do lua code failed :%s", lua_tostring(L,1));
    lua_pop(L, 1);
    return UGERR;
}

int luaworkDoFile(lua_State *L,const char *path , char *err) {
    int ret = UGERR;

    FILE *fp = fopen(path,"rb");
    if (NULL == fp) {
        luaworkSetError(err, "open file %s failed", path);
        return UGERR;
    }
    fclose(fp);

    if (0 == luaL_dofile(L, path)) return UGOK;

    luaworkSetError(err, "do lua code failed :%s", lua_tostring(L,1));
    lua_pop(L, 1);

    return ret;
}


int luaworkDoDir(lua_State *L,const char *path, char *err) {
    int ret = UGOK;
    sds relpath = sdsempty();
    list *queue = listCreate();
    listNode *node=NULL;
    listIter *iter = NULL;

    if (xfilelistdir(path, "*.lua", queue) == UGERR) {
        luaworkSetError(err, "list files for %s failed", path);
        ret = UGERR;
        goto END;
    }

    iter = listGetIterator(queue, AL_START_HEAD);

    while ((node=listNext(iter))!=NULL) {
        const char *filename = (char *) (node->value);
        sdsclear(relpath);
        relpath = sdscatprintf(relpath, "%s/%s", path, filename);
        ret = luaworkDoFile(L, relpath, err);
        if (ret != UGOK) break;
    }

    listReleaseIterator(iter);
END:
    listRelease(queue);
    sdsfree(relpath);
    return ret;

}


LUA_HANDLE luaworkRefFunction(lua_State *L,const char *func,char *err) {
    UG_NOTUSED(err);
    lua_getglobal(L, func);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}


void luaworkUnrefFunction(lua_State *L, LUA_HANDLE handle,char *err) {
    UG_NOTUSED(err);
    luaL_unref(L, LUA_REGISTRYINDEX, handle);
}

int luaworkInnerCall(lua_State *L, char *err, const char *sig, va_list vl) {
    int narg, nres=0;
    int ret = UGOK;
    //push param
    for (narg=0; *sig; narg++) {
        switch(*sig++) {
        case 'd':
            lua_pushnumber(L, va_arg(vl,double));
            break;
        case 'i':
            lua_pushinteger(L, va_arg(vl,int));
            break;
        case 's':
            lua_pushstring(L, va_arg(vl,char *));
            break;
        case '>':
            goto ENDARGS;
        default:
            luaworkSetError(err, "%s-%c", "invalid option", *(sig-1));
            return UGERR;
        }
    }

ENDARGS:
    nres = strlen(sig);
    if (lua_pcall(L, narg, nres, 0) != 0) {
        luaworkSetError(err, "%s", lua_tostring(L,-1));
        lua_pop(L, 1);
        return UGERR;
    }
    lua_gc(L, LUA_GCSTEP, 1);
    switch(lua_type(L, -1)) {
      case LUA_TTABLE:
          lua_pushstring(L, "err");
          lua_gettable(L, -2);
          if (lua_type(L, -1) == LUA_TSTRING) {
              ret = UGERR;
              luaworkSetError(err, "lua function return err:'%s'", lua_tostring(L, -1));
              lua_pop(L, 2);
          }
          lua_pop(L, 1);
          break;
      default:
          break;
    }
    lua_pop(L, 1);
    return ret;
}

int luaworkCallByName(lua_State *L,const char *func,char *err, const char *sig, ...) {
    va_list vl;
    int ret = UGOK;
    int top = lua_gettop(L);
    va_start(vl,sig);
    lua_getglobal(L, func);
    ret = luaworkInnerCall(L, err, sig, vl);
    va_end(vl);
    lua_settop(L, top);
    return ret;
}

int luaworkCallByRef(lua_State *L, LUA_HANDLE handle,char *err, const char *sig,... ) {
    va_list vl;
    int ret = UGOK;
    int top = lua_gettop(L);
    va_start(vl,sig);
    lua_rawgeti(L, LUA_REGISTRYINDEX, handle);
    ret = luaworkInnerCall(L, err, sig, vl);
    va_end(vl);
    lua_settop(L, top);
    return ret;
}

int  luaworkRefLib(lua_State *L,const luaL_Reg *reg, char *err) {
    const luaL_Reg *lib;

    /* call open functions from 'loadedlibs' and set results to global table */
    for (lib = reg; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* emove lib */
    }
    return UGOK;
}

void luaworkTraceStack(lua_State *L, int n, char *msg, size_t msglen) {
    lua_Debug ar;
    char buf[MAX_STRING_LEN]= {0};
    while( lua_getstack(L, n++, &ar) ) {
        lua_getinfo(L, "Sln", &ar);
        buf[0] = 0;
        if(ar.name) {
            sprintf(buf, "    stack[%d] -> line %d : %s()[%s : line %d]\n", n, ar.currentline, ar.name, ar.short_src, ar.linedefined);
        } else {
            sprintf(buf, "    stack[%d] -> line %d : unknown[%s : line %d]\n", n, ar.currentline, ar.short_src, ar.linedefined);
        }
        if (strlen(msg) + strlen(buf) < msglen) {
            strcat(msg, buf);
        } else {
            break;
        }
    }
}
