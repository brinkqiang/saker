
function sysinfo_example()
    saker.log(LOG_INFO, "This is sysinfo_example")
    -- get pid 
    local AppName = ""
    if saker.uname() == "WIN" then
        AppName = "saker.exe" 
    else 
        AppName = "saker"
    end
    local pidstr,err = saker.sysinfo("system.proc.id", AppName)
    if err ~= nil then
        saker.log(LOG_ERROR, "can not find the pid "..err)
        return true
    end

    print(pidstr)
    local pidobj = json.decode(pidstr) 

    for k,v in pairs(pidobj)
    do
        print(v)
    end

    local ctx,err = saker.sysinfo("system.proc.all", pidobj[1])

    if err ~= nil then
        saker.log(LOG_ERROR , err)
        return false, err
    end

    saker.log(LOG_INFO, ctx)
    local myt = json.decode(ctx)
    for k,v in pairs(myt)
    do
        saker.log(LOG_INFO, k..":"..v)
    end
    return true
end
    
saker.register("sysinfo_example", "sysinfo_example", PROP_CYCLE)
