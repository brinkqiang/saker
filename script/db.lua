local sakerdb = {}
local sakerdb_id = 0
local sakerdb_size = 16

function db_ping() 
    return true, "PONG"
end

function db_dbsize()
    local dbsize = 0
    for _, _ in pairs(sakerdb[sakerdb_id])
    do
        dbsize = dbsize + 1
    end
    return dbsize
end

function db_select(args) 
    if not args or #args ~= 1 then
        return false, "wrong params"
    end
    local dbid = tonumber(args[1]) 
    if dbid >= sakerdb_size or dbid < 0 then
        return false, "out of dbsize"
    end
    sakerdb_id = dbid
    local rt = {}
    rt["ok"] = "OK"
    return rt
end

function db_keys()
    local keys = {}
    for k, _ in pairs(sakerdb[sakerdb_id])
    do
        if k ~= "_dbsize" then
            table.insert(keys, k)
        end
    end
    return keys 
end

function db_set(args) 
    if not args or #args ~= 2 then
        return false, "wrong params"
    end
    local key = args[1]
    local value = args[2]
    sakerdb[sakerdb_id][key] = value
    local rt = {}
    rt["ok"] = "OK"
    return rt
end

function db_get(args) 
    if not args or #args ~= 1 then
        return false, "wrong params"
    end
    local key = args[1]
    local value = sakerdb[sakerdb_id][key]
    if value then
        return value
    end
    return false
end

local function main()
    for i=0, sakerdb_size-1, 1 do
        sakerdb[i] = {}
    end
    
    saker.register("ping",   "db_ping",   PROP_PASSIVITY)
    saker.register("PING",   "db_ping",   PROP_PASSIVITY)  
    saker.register("dbsize", "db_dbsize", PROP_PASSIVITY)
    saker.register("select", "db_select", PROP_PASSIVITY)
    saker.register("keys",   "db_keys",   PROP_PASSIVITY)
    saker.register("set",    "db_set",    PROP_PASSIVITY)
    saker.register("SET",    "db_set",    PROP_PASSIVITY)
    saker.register("get",    "db_get",    PROP_PASSIVITY)
    saker.register("GET",    "db_get",    PROP_PASSIVITY)
end

main()
